// AlpacaCore
// Copyright (c) 2025-2026 Joey Troy and contributors
//
// This file is part of AlpacaCore.
//
// AlpacaCore is licensed under the GNU Affero General Public License,
// version 3 or (at your option) any later version (AGPL-3.0-or-later),
// with an additional permission allowing combination with proprietary
// device-vendor SDKs. See the LICENSE file in this repository for the full
// license text and the vendor-SDK linking exception, or the license online at:
// https://www.gnu.org/licenses/agpl-3.0.html
//
// Park is an asynchronous initiator (issue #208): it must return well inside
// the ConformU 4.5 STANDARD 1 s target while the park slew runs in the
// background, Slewing stays true until the mount arrives, and AtPark flips
// true in the same step Slewing drops. A FakeMountServer plays a Celestron
// NexStar handset whose GOTO takes ~1.5 s, so the whole lifecycle runs hardware-free.
#ifndef _WIN32

#include <alpacacore/telescope_driver.h>
#include <alpacacore/util/error_handling.h>
#include <alpacacore/util/logging.h>
#include <alpacacore/vendor/celestron/celestron_protocol_wrapper.h>
#include <alpacacore/vendor/celestron/celestron_telescope_driver.h>

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "catch2_compat.h"
#include "concurrency_stress.h"
#include "fake_mount_server.h"

namespace {

using Clock = std::chrono::steady_clock;

struct FakeCelestronState {
    std::atomic<bool> goto_seen{false};
    std::atomic<int> goto_count{0};
    std::atomic<Clock::rep> goto_started{0};
    std::atomic<bool> hold_goto{false};
    std::atomic<bool> muted{false};
    std::atomic<bool> fail_dispatch{false};
    std::atomic<bool> fail_axis_stop{false};
    std::atomic<int> failed_stop{-1};
    std::array<std::atomic<int>, 3> stop_attempts{};
    std::atomic<int> status_polls{0};
    static constexpr auto kGotoDuration = std::chrono::milliseconds(1500);
    bool goto_in_progress() const {
        if (!goto_seen.load()) return false;
        if (hold_goto.load()) return true;
        const auto started = Clock::time_point(Clock::duration(goto_started.load()));
        return Clock::now() - started < kGotoDuration;
    }
};

alpacacore::test::FakeMountServer::Responder celestron_responder(std::shared_ptr<FakeCelestronState> st) {
    return [st](const std::string& chunk) -> std::string {
        if (chunk.empty()) return "0#";
        const bool passthrough = chunk.size() == 8 && chunk[0] == 'P';
        const unsigned char op = passthrough ? static_cast<unsigned char>(chunk[3]) : 0;
        if (chunk[0] == 'L' || (passthrough && op == 0x13)) ++st->status_polls;
        if (passthrough && (op == 6 || op == 7) && chunk[4] == 0 && chunk[5] == 0 && st->fail_axis_stop.load()) {
            st->muted.store(true);
        }
        int stop = -1;
        if (chunk == "M") stop = 0;
        if (passthrough && (op == 36 || op == 37) && chunk[4] == 0) {
            stop = chunk[2] == 16 ? 1 : 2;
        }
        if (stop >= 0) {
            if (chunk == "M" || op == 36) ++st->stop_attempts[stop];
            if (st->failed_stop.load() == stop) return "";
        }
        const bool goto_command = chunk[0] == 'r' || chunk[0] == 'R' || chunk[0] == 'b' || chunk[0] == 'B' ||
                                  (passthrough && (op == 0x02 || op == 0x17));
        if (goto_command && st->fail_dispatch.load()) {
            st->goto_seen.store(true);
            st->muted.store(true);
        }
        if (st->muted.load()) return "";
        switch (chunk[0]) {
            case 'e':
            case 'E':
            case 'z':
            case 'Z':
                return "12AB0500,20000500#";  // parseable 16/24-bit position pair
            case 'r':
            case 'R':
            case 'b':
            case 'B':
                st->goto_started.store(Clock::now().time_since_epoch().count());
                st->goto_seen.store(true);
                st->goto_count.fetch_add(1);
                return "#";
            case 'L':
                return st->goto_in_progress() ? "1#" : "0#";
            case 'M':  // cancel goto
                st->goto_seen.store(false);
                return "#";
            case 'J':  // alignment complete (the slew-safety gate requires it)
                return "1#";
            case 'T':  // tracking mode write
                return "#";
            case 'P': {  // AUX passthrough: P len dev op ...
                const unsigned char op = chunk.size() > 3 ? static_cast<unsigned char>(chunk[3]) : 0;
                if (op == 0x02 || op == 0x17) {  // MC_GOTO_FAST / MC_GOTO_SLOW
                    st->goto_started.store(Clock::now().time_since_epoch().count());
                    st->goto_seen.store(true);
                    st->goto_count.fetch_add(1);
                    return "#";
                }
                if (op == 0x13) {  // MC_SLEW_DONE: 0x00 = still slewing, 0xFF = done
                    return st->goto_in_progress() ? std::string("\x00#", 2) : std::string("\xFF#");
                }
                if (op == 0xFE) return std::string("\x01\x00#", 3);  // GET_VER: two binary bytes, then '#'
                return std::string("\xFF#");
            }
            default:
                return "0#";
        }
    };
}

alpacacore::vendor::celestron::ConnectionInfo endpoint(int port) {
    alpacacore::vendor::celestron::ConnectionInfo info;
    info.type = alpacacore::vendor::celestron::ConnectionType::Network;
    info.host = "127.0.0.1";
    info.tcp_port = port;
    info.response_timeout_ms = 200;
    return info;
}

bool wait_until(const std::function<bool()>& pred, int timeout_ms) {
    const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_ms);
    while (Clock::now() < deadline) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return pred();
}

}  // namespace

TEST_CASE("Celestron async - Park returns immediately, AtPark flips when the slew ends",
          "[celestron][telescope][async]") {
    auto st = std::make_shared<FakeCelestronState>();
    alpacacore::test::FakeMountServer server(celestron_responder(st));
    REQUIRE(server.ok());
    auto driver = alpacacore::vendor::celestron::create_celestron_telescope(0, endpoint(server.port()));
    REQUIRE(alpacacore::test::settle_connected(*driver, true, std::chrono::seconds(10)));
    REQUIRE_FALSE(driver->get_at_park());

    const auto t0 = Clock::now();
    driver->park();
    const auto park_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count();
    CHECK(park_ms < 1000);           // ConformU 4.5 STANDARD target for an async initiator
    REQUIRE(driver->get_slewing());  // parking reports Slewing until AtPark
    REQUIRE_FALSE(driver->get_at_park());
    REQUIRE(wait_until([&] { return st->goto_seen.load(); }, 5000));  // the GOTO was dispatched
    // Park while a park is in flight is a no-op: the running slew is neither
    // cancelled nor restarted (still exactly one GOTO on the wire).
    driver->park();
    REQUIRE(driver->get_slewing());
    REQUIRE_FALSE(driver->get_at_park());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    CHECK(st->goto_count.load() == 1);

    REQUIRE(wait_until([&] { return driver->get_at_park(); }, 20000));
    REQUIRE_FALSE(driver->get_slewing());
    REQUIRE_FALSE(driver->get_tracking());  // park stops tracking

    driver->park();  // second Park on a parked mount is harmless
    REQUIRE(driver->get_at_park());
    REQUIRE_FALSE(driver->get_slewing());

    driver->unpark();
    REQUIRE_FALSE(driver->get_at_park());
    driver->set_connected(false);
}

TEST_CASE("Celestron async - Unpark during a park cancels it", "[celestron][telescope][async]") {
    auto st = std::make_shared<FakeCelestronState>();
    alpacacore::test::FakeMountServer server(celestron_responder(st));
    REQUIRE(server.ok());
    auto driver = alpacacore::vendor::celestron::create_celestron_telescope(0, endpoint(server.port()));
    REQUIRE(alpacacore::test::settle_connected(*driver, true, std::chrono::seconds(10)));

    driver->park();
    REQUIRE(driver->get_slewing());
    driver->unpark();
    REQUIRE_FALSE(driver->get_at_park());
    REQUIRE(wait_until([&] { return !driver->get_slewing(); }, 5000));
    // The cancelled park task must never flip AtPark afterwards.
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    REQUIRE_FALSE(driver->get_at_park());

    // A park in flight gates motion members like a completed park does, so a
    // slew or axis jog cannot silently clobber it (ParkedException, 0x408).
    driver->park();
    REQUIRE(driver->get_slewing());
    CHECK_THROWS_AS(driver->move_axis(0, 0.5), alpacacore::AlpacaException);
    CHECK_THROWS_AS(driver->slew_to_coordinates_async(5.0, 20.0), alpacacore::AlpacaException);
    CHECK_THROWS_AS(driver->sync_to_coordinates(5.0, 20.0), alpacacore::AlpacaException);
    try {
        driver->move_axis(0, 0.5);
    } catch (const alpacacore::AlpacaException& ex) {
        CHECK(ex.error_code() == alpacacore::AlpacaError::InvalidWhileParked);
    }
    REQUIRE(driver->get_slewing());  // the park is still in flight
    driver->abort_slew();            // ...but AbortSlew may cancel it
    REQUIRE_FALSE(driver->get_at_park());
    REQUIRE(wait_until([&] { return !driver->get_slewing(); }, 5000));

    // Disconnect with a park in flight joins the task cleanly.
    driver->park();
    driver->set_connected(false);
    REQUIRE_FALSE(driver->get_connected());
}

TEST_CASE("Celestron - Unpark attempts every stop and reports failures truthfully", "[celestron][telescope][stop]") {
    // -1 = success, 0/1/2 = a selected stop fails, 3 = the whole link is silent.
    for (int failed_stop : {-1, 0, 1, 2, 3}) {
        CAPTURE(failed_stop);
        auto st = std::make_shared<FakeCelestronState>();
        st->hold_goto.store(true);
        alpacacore::test::FakeMountServer server(celestron_responder(st));
        REQUIRE(server.ok());
        auto driver = alpacacore::vendor::celestron::create_celestron_telescope(0, endpoint(server.port()));
        driver->set_connected(true);
        driver->park();
        REQUIRE(wait_until([&] { return st->goto_seen.load(); }, 5000));
        st->failed_stop.store(failed_stop);
        st->muted.store(failed_stop == 3);
        if (failed_stop < 0) {
            REQUIRE_NOTHROW(driver->unpark());
        } else {
            try {
                driver->unpark();
                FAIL_CHECK("Expected failed park stop to throw");
            } catch (const alpacacore::AlpacaException& ex) {
                CHECK(ex.error_code() == alpacacore::AlpacaError::DriverException);
                CHECK(std::string(ex.what()) ==
                      "Unpark could not stop the park slew: Timeout waiting for Celestron response");
            }
        }
        for (const auto& attempts : st->stop_attempts) CHECK(attempts.load() == 1);
        REQUIRE_FALSE(driver->get_at_park());
        st->muted.store(true);
        const int polls = st->status_polls.load();
        CHECK(driver->get_slewing() == (failed_stop >= 0));
        // Prove the real polling path ran, rather than a timed slew grace
        // window or a still-running park task making Slewing true by accident.
        CHECK(st->status_polls.load() > polls);

        st->muted.store(false);
        st->failed_stop.store(-1);
        st->hold_goto.store(false);
        // Retry is allowed: the failed Unpark cleared parking_ and joined its
        // task. The new park must not be cancelled by the old worker's tail.
        driver->park();
        REQUIRE(wait_until([&] { return driver->get_at_park(); }, 20000));
        CHECK_FALSE(driver->get_slewing());
    }
}

TEST_CASE("Celestron - failed park cleanup logs failed stops and retains motion", "[celestron][telescope][stop]") {
    auto st = std::make_shared<FakeCelestronState>();
    std::atomic<bool> error_seen{false};
    struct SinkGuard {
        alpacacore::logging::LogSink previous = alpacacore::logging::get_log_sink();
        ~SinkGuard() { alpacacore::logging::set_log_sink(previous); }
    } sink_guard;
    alpacacore::logging::set_log_sink(
        [&](alpacacore::logging::LogLevel level, std::string_view component, std::string_view message) {
            if (level == alpacacore::logging::LogLevel::Error && component == "Celestron" &&
                message ==
                    "Celestron: stop after park failure failed: Timeout waiting for Celestron response; "
                    "the mount may still be moving") {
                error_seen.store(true);
            }
        });
    alpacacore::test::FakeMountServer server(celestron_responder(st));
    REQUIRE(server.ok());
    auto driver = alpacacore::vendor::celestron::create_celestron_telescope(0, endpoint(server.port()));
    driver->set_connected(true);
    st->fail_dispatch.store(true);
    driver->park();
    REQUIRE(wait_until([&] { return st->stop_attempts[0].load() > 0; }, 5000));
    CHECK(wait_until([&] { return error_seen.load(); }, 5000));
    for (const auto& attempts : st->stop_attempts) CHECK(attempts.load() == 1);
    REQUIRE_FALSE(driver->get_at_park());
    const int polls = st->status_polls.load();
    CHECK(driver->get_slewing());
    CHECK(st->status_polls.load() > polls);

    st->fail_dispatch.store(false);
    st->muted.store(false);
    driver->park();
    REQUIRE(wait_until([&] { return driver->get_at_park(); }, 20000));
    CHECK_FALSE(driver->get_slewing());
}

TEST_CASE("Celestron - failed MoveAxis stop retains the manual motion flag", "[celestron][telescope][stop]") {
    for (int axis : {0, 1}) {
        CAPTURE(axis);
        auto st = std::make_shared<FakeCelestronState>();
        alpacacore::test::FakeMountServer server(celestron_responder(st));
        REQUIRE(server.ok());
        auto driver = alpacacore::vendor::celestron::create_celestron_telescope(0, endpoint(server.port()));
        driver->set_connected(true);
        REQUIRE_FALSE(driver->get_slewing());
        driver->move_axis(axis, 0.5);
        REQUIRE(driver->get_slewing());
        st->fail_axis_stop.store(true);
        CHECK_THROWS_AS(driver->move_axis(axis, 0.0), alpacacore::AlpacaException);
        CHECK(driver->get_slewing());
        st->fail_axis_stop.store(false);
        st->muted.store(false);
        REQUIRE_NOTHROW(driver->move_axis(axis, 0.0));
        CHECK_FALSE(driver->get_slewing());
    }
}

#endif  // !_WIN32
