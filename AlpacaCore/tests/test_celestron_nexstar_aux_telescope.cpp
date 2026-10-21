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

#include <alpacacore/vendor/celestron/celestron_aux_link.h>
#include <alpacacore/vendor/celestron/celestron_nexstar_aux_telescope_driver.h>
#include <alpacacore/version.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>

#include "catch2_compat.h"
#include "fake_celestron_aux_server.h"
#include "fake_task_clock.h"

namespace {
using namespace std::chrono_literals;
using namespace alpacacore;
using namespace alpacacore::vendor::celestron;
NexStarAuxConnectionInfo info(int port = 2000) {
    NexStarAuxConnectionInfo value;
    value.host = "127.0.0.1";
    value.tcp_port = port;
    value.site_latitude = 45;
    value.site_longitude = -75;
    value.aperture_diameter = 0.2;
    value.focal_length = 1;
    return value;
}
template <typename F>
void error_code(F&& action, int code) {
    try {
        action();
        FAIL("Expected AlpacaException");
    } catch (const AlpacaException& error) {
        CHECK(error.error_code() == code);
    }
}
void refused_identity(test::FakeCelestronAuxServer& fake) {
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    error_code([&] { driver->set_connected(true); }, AlpacaError::DriverException);
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
    CHECK(driver->get_device_state().empty());
    const auto requests = fake.requests();
    CHECK(std::all_of(requests.begin(), requests.end(), [](const auto& frame) {
        return frame.source == 0x20 && (frame.destination == 0x10 || frame.destination == 0x11) &&
               (frame.command == 0xfe || frame.command == 0x05) && frame.payload.empty();
    }));
    // A new live exchange proves the refused driver closed the socket and released its claim.
    CelestronAuxLink replacement;
    CHECK_NOTHROW(replacement.connect_tcp("127.0.0.1", fake.port(), 500ms));
    CHECK_NOTHROW(replacement.exchange({0x20, 0x10, 0xfe, {}}, 500ms));
}
void transition(TelescopeDriver& driver, test::FakeTaskClock& clock, bool connected) {
    const auto waits = clock.wait_count();
    auto result = std::async(std::launch::async, [&] { driver.set_connected(connected); });
    const bool waiting = clock.wait_for_wait_count(waits + 1, 2s);
    clock.advance(500ms);
    // Do not unwind while a connection is asleep on virtual time.
    REQUIRE(waiting);
    REQUIRE(result.wait_for(2s) == std::future_status::ready);
    result.get();
    if (connected) REQUIRE(clock.wait_for_wait_count(waits + 2, 2s));
}
// A failing REQUIRE must not strand the destructor's verification on virtual time.
struct VirtualDisconnect {
    TelescopeDriver& driver;
    test::FakeTaskClock& clock;
    ~VirtualDisconnect() {
        if (!driver.get_connected()) return;
        const auto waits = clock.wait_count();
        auto result = std::async(std::launch::async, [&] { driver.set_connected(false); });
        if (clock.wait_for_wait_count(waits + 1, 2s)) clock.advance(500ms);
        try {
            result.get();
        } catch (const AlpacaException&) {
            // Fault teardown still closes the transport.
        }
    }
};
void home_for_sync(TelescopeDriver& driver) {
    driver.set_connected(true);
    driver.find_home();
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (driver.get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    REQUIRE(driver.get_at_home());
}
std::size_t label_writes(const test::FakeCelestronAuxServer& fake, std::uint8_t axis = 0) {
    const auto requests = fake.requests();
    return std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
        return frame.command == 0x04 && (axis == 0 || frame.destination == axis);
    });
}
}  // namespace

TEST_CASE("Celestron SkyPortal AUX Driver - Native guide directions duration and ownership",
          "[celestron][nexstar-aux][telescope][guide]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    error_code([&] { driver->pulse_guide(4, 0); }, AlpacaError::InvalidValue);
    error_code([&] { driver->pulse_guide(0, -1); }, AlpacaError::InvalidValue);
    error_code([&] { driver->pulse_guide(0, 600001); }, AlpacaError::InvalidValue);
    error_code([&] { driver->pulse_guide(0, 0); }, AlpacaError::NotConnected);
    home_for_sync(*driver);
    error_code([&] { driver->pulse_guide(0, 0); }, AlpacaError::InvalidOperation);
    driver->sync_to_coordinates(6, 20);
    driver->set_tracking(true);
    const auto guides = [&] {
        auto frames = fake.requests();
        std::erase_if(frames, [](const auto& f) { return f.command != 0x26; });
        return frames;
    };
    driver->pulse_guide(0, 0);
    CHECK(guides().empty());
    CHECK_FALSE(driver->get_is_pulse_guiding());
    CHECK_FALSE(driver->get_can_set_guide_rates());
    CHECK(driver->get_guide_rate().ra == 15.0411 / 7200.0);
    CHECK(driver->get_guide_rate().dec == 15.0411 / 7200.0);
    for (int direction = 0; direction < 4; ++direction) {
        const auto before = driver->get_declination();
        const auto before_ra = driver->get_right_ascension();
        const auto count = guides().size();
        driver->pulse_guide(direction, 100);
        CHECK(driver->get_is_pulse_guiding());
        CHECK_FALSE(driver->get_slewing());
        error_code([&] { driver->pulse_guide((direction + 2) % 4, 100); }, AlpacaError::InvalidOperation);
        error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::InvalidOperation);
        error_code([&] { driver->set_tracking(false); }, AlpacaError::InvalidOperation);
        const auto deadline = std::chrono::steady_clock::now() + 3s;
        while (driver->get_is_pulse_guiding() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(10ms);
        CHECK_FALSE(driver->get_is_pulse_guiding());
        REQUIRE(guides().size() == count + 1);
        const auto frame = guides().back();
        CHECK(frame.destination == (direction < 2 ? 0x11 : 0x10));
        CHECK(frame.payload ==
              std::vector<std::uint8_t>{static_cast<std::uint8_t>(direction == 1 || direction == 3 ? 50 : 206), 10});
        if (direction < 2)
            CHECK((driver->get_declination() - before) * (direction == 0 ? 1 : -1) > 0);
        else
            CHECK(std::remainder(driver->get_right_ascension() - before_ra, 24.0) * (direction == 2 ? 1 : -1) > 0);
        CHECK(driver->get_tracking());
    }
    for (const int duration : {1, 5, 15, 2549, 2550, 2551, 2555, 5115}) {
        INFO("duration=" << duration);
        const auto count = guides().size();
        driver->pulse_guide(1, duration);
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        while (driver->get_is_pulse_guiding() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(10ms);
        REQUIRE_FALSE(driver->get_is_pulse_guiding());
        auto remaining = std::max(1, (duration + 5) / 10);
        const auto frames = guides();
        REQUIRE(frames.size() - count == static_cast<std::size_t>((remaining + 254) / 255));
        for (auto i = count; i < frames.size(); ++i) {
            CHECK(frames[i].payload ==
                  std::vector<std::uint8_t>{50, static_cast<std::uint8_t>(std::min(remaining, 255))});
            remaining -= std::min(remaining, 255);
        }
    }
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
    CHECK_FALSE(driver->get_slewing());
    CHECK(label_writes(fake) == 2);  // Guiding never fabricates a coordinate correction.
}

TEST_CASE("Celestron SkyPortal AUX Driver - Guide Abort cancels remaining segments and verifies stopping",
          "[celestron][nexstar-aux][telescope][guide]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    driver->pulse_guide(1, 10000);
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    bool sent = false;
    while (!sent && std::chrono::steady_clock::now() < deadline) {
        const auto frames = fake.requests();
        sent = std::any_of(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; });
        std::this_thread::sleep_for(10ms);
    }
    REQUIRE(sent);
    driver->abort_slew();
    CHECK(driver->get_slewing());
    const auto stopped = std::chrono::steady_clock::now() + 6s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < stopped) std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_is_pulse_guiding());
    CHECK_FALSE(driver->get_tracking());
    CHECK_FALSE(driver->get_at_home());
    const auto frames = fake.requests();
    CHECK(std::count_if(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; }) == 1);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Lost guide acknowledgement retains failure without retry",
          "[celestron][nexstar-aux][telescope][guide]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    driver->set_tracking(true);
    fake.lose_guide_ack(0x11);
    driver->pulse_guide(1, 10000);
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (driver->get_link_fault().empty() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_is_pulse_guiding(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
    error_code([&] { driver->pulse_guide(1, 100); }, AlpacaError::DriverException);
    const auto frames = fake.requests();
    CHECK(std::count_if(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; }) == 1);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Disconnect drains the current native guide packet",
          "[celestron][nexstar-aux][telescope][guide]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    driver->pulse_guide(1, 10000);
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    bool sent = false;
    while (!sent && std::chrono::steady_clock::now() < deadline) {
        const auto frames = fake.requests();
        sent = std::any_of(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; });
        std::this_thread::sleep_for(1ms);
    }
    REQUIRE(sent);
    CHECK_NOTHROW(driver->set_connected(false));
    CHECK_FALSE(driver->get_connected());
    const auto frames = fake.requests();
    CHECK(std::count_if(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; }) == 1);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Guide busy timeout never chains another segment",
          "[celestron][nexstar-aux][telescope][guide]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    fake.set_guide_stuck(true);
    driver->pulse_guide(1, 3000);
    const auto deadline = std::chrono::steady_clock::now() + 9s;
    while (driver->get_link_fault().empty() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    fake.set_guide_stuck(false);
    REQUIRE_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_is_pulse_guiding(); }, AlpacaError::DriverException);
    const auto frames = fake.requests();
    CHECK(std::count_if(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; }) == 1);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Sync settles labels and publishes coordinates and targets",
          "[celestron][nexstar-aux][telescope][sync]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_home_position(0, 16763171);
    fake.set_home_position(1, 16148955);
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    for (const auto axis : {0, 1}) {
        fake.script_position_offsets(axis, {8, 4, 0, 0, 0, 0});
        fake.script_label_offsets(axis, {8, -7, 4, 0, 0, 0});
    }
    CHECK_NOTHROW(driver->sync_to_coordinates(6, 20));
    CHECK(label_writes(fake) == 2);
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - 6.0, 24.0)) < 0.001);
    CHECK(std::abs(driver->get_declination() - 20.0) < 0.00003);
    CHECK(driver->get_target_right_ascension() == 6);
    CHECK(driver->get_target_declination() == 20);
    CHECK_FALSE(driver->get_at_home());
    CHECK(driver->get_side_of_pier() == 0);
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_tracking());
    // Subsequent idle polls must not mistake the verified new labels for uncommanded motion.
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
    driver->set_target_right_ascension(7);
    driver->set_target_declination(-10);
    CHECK_NOTHROW(driver->sync_to_target());
    CHECK(label_writes(fake) == 4);
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - 7.0, 24.0)) < 0.001);
    CHECK(std::abs(driver->get_declination() + 10.0) < 0.00003);
}

TEST_CASE("Celestron SkyPortal AUX Driver - GOTO refuses off tracking and branch-crossing approaches before dispatch",
          "[celestron][nexstar-aux][telescope][goto]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    error_code([&] { driver->slew_to_coordinates_async(-1, 20); }, AlpacaError::InvalidValue);
    error_code([&] { driver->slew_to_coordinates_async(6, 91); }, AlpacaError::InvalidValue);
    error_code([&] { driver->slew_to_coordinates_async(6, 20); }, AlpacaError::NotConnected);
    home_for_sync(*driver);
    const auto ra = driver->get_right_ascension();
    error_code([&] { driver->slew_to_coordinates_async(ra, 88); }, AlpacaError::InvalidOperation);
    driver->set_tracking(true);
    const auto opposite = std::fmod(ra + 12.0, 24.0);
    CHECK(driver->get_destination_side_of_pier(ra, 88) == 0);
    CHECK(driver->get_destination_side_of_pier(opposite, 88) == 1);
    error_code([&] { driver->slew_to_coordinates_async(ra, 89.9); }, AlpacaError::InvalidOperation);
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin(), requests.end(),
                       [](const auto& frame) { return frame.command == 0x02 || frame.command == 0x17; }));
    CHECK(driver->get_tracking());
}

TEST_CASE("Celestron SkyPortal AUX Driver - GOTO uses slow approach selective corrections and settling",
          "[celestron][nexstar-aux][telescope][goto]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    const auto target = std::fmod(driver->get_right_ascension() + 0.01, 24.0);
    fake.script_slow_offsets(0, {1000, 0});
    driver->set_slew_settle_time(2);
    driver->set_target_right_ascension(target);
    driver->set_target_declination(88.5);
    const auto started = std::chrono::steady_clock::now();
    driver->slew_to_target_async();
    CHECK(std::chrono::steady_clock::now() - started < 100ms);
    CHECK(driver->get_slewing());
    const auto deadline = started + 8s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK(std::chrono::steady_clock::now() - started >= 2s);
    CHECK(driver->get_tracking());
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - target, 24.0)) * 15 < 3.0 / 3600);
    CHECK(std::abs(driver->get_declination() - 88.5) < 3.0 / 3600);
    CHECK(driver->get_target_right_ascension() == target);
    CHECK(driver->get_target_declination() == 88.5);
    const auto requests = fake.requests();
    for (const auto axis : {0x10, 0x11}) {
        CHECK(std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
                  return frame.destination == axis && frame.command == 0x02;
              }) == 1);
        CHECK(std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
                  return frame.destination == axis && frame.command == 0x17;
              }) == (axis == 0x10 ? 2 : 1));
    }
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Branch-changing GOTO homes and preserves Sync calibration",
          "[celestron][nexstar-aux][telescope][goto][flip]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    const double home_ra = driver->get_right_ascension();
    driver->sync_to_coordinates(home_ra, 89.5);
    const auto synced = fake.requests();
    driver->set_tracking(true);
    const auto opposite = std::fmod(home_ra + 12.0, 24.0);
    const auto before = fake.requests().size();
    const auto started = std::chrono::steady_clock::now();
    driver->slew_to_coordinates_async(opposite, 88.5);
    CHECK(std::chrono::steady_clock::now() - started < 100ms);
    CHECK(driver->get_slewing());
    const auto deadline = started + 12s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK(driver->get_tracking());
    CHECK(driver->get_side_of_pier() == 1);
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - opposite, 24.0)) * 15 < 3.0 / 3600);
    CHECK(std::abs(driver->get_declination() - 88.5) < 3.0 / 3600);
    const auto requests = fake.requests();
    for (const auto axis : {0x10, 0x11}) {
        const auto first = requests.begin() + before;
        const auto home = std::find_if(first, requests.end(),
                                       [axis](const auto& f) { return f.destination == axis && f.command == 0x0b; });
        const auto label = std::find_if(first, requests.end(),
                                        [axis](const auto& f) { return f.destination == axis && f.command == 0x04; });
        const auto fast = std::find_if(first, requests.end(),
                                       [axis](const auto& f) { return f.destination == axis && f.command == 0x02; });
        REQUIRE(home != requests.end());
        REQUIRE(label != requests.end());
        REQUIRE(fast != requests.end());
        CHECK(home < label);
        CHECK(label < fast);
        const auto sync_label = std::find_if(
            synced.begin(), synced.end(), [axis](const auto& f) { return f.destination == axis && f.command == 0x04; });
        REQUIRE(sync_label != synced.end());
        CHECK(label->payload == sync_label->payload);
    }
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
    driver->slew_to_coordinates_async(home_ra, 88.5);
    const auto return_deadline = std::chrono::steady_clock::now() + 12s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < return_deadline)
        std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK(driver->get_side_of_pier() == 0);
    CHECK(driver->get_tracking());
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - home_ra, 24.0)) * 15 < 3.0 / 3600);
    CHECK(std::abs(driver->get_declination() - 88.5) < 3.0 / 3600);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Branch-changing GOTO refuses ambiguous home or label completion",
          "[celestron][nexstar-aux][telescope][goto][flip]") {
    bool home_failure = false;
    SECTION("Lost home acknowledgement") { home_failure = true; }
    SECTION("Lost restored label acknowledgement") {}
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    driver->set_tracking(true);
    if (home_failure)
        fake.lose_home_reply(0x10);
    else
        fake.lose_label_reply(0x11);
    const auto before = fake.requests().size();
    driver->slew_to_coordinates_async(std::fmod(driver->get_right_ascension() + 12.0, 24.0), 88.5);
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (driver->get_link_fault().empty() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    REQUIRE(fake.wait_for_command_since(before, 0x11, 0x24));
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin() + before, requests.end(),
                       [](const auto& f) { return f.command == 0x02 || f.command == 0x17; }));
    CHECK(std::count_if(requests.begin() + before, requests.end(), [home_failure](const auto& f) {
              return f.destination == (home_failure ? 0x10 : 0x11) && f.command == (home_failure ? 0x0b : 0x04);
          }) == 1);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort during meridian home never restores an unverified reference",
          "[celestron][nexstar-aux][telescope][goto][flip][abort]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    fake.set_home_polls(0, -1);
    fake.set_home_polls(1, -1);
    const auto before = fake.requests().size();
    driver->slew_to_coordinates_async(std::fmod(driver->get_right_ascension() + 12.0, 24.0), 88.5);
    REQUIRE(fake.wait_for_command_since(before, 0x10, 0x0b));
    driver->abort_slew();
    const auto deadline = std::chrono::steady_clock::now() + 6s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_tracking());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin() + before, requests.end(), [](const auto& f) {
        return f.command == 0x02 || f.command == 0x17 ||
               (f.command == 0x06 && f.payload == std::vector<std::uint8_t>{0xff, 0xff});
    }));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Meridian home guards stop before labels or GOTO",
          "[celestron][nexstar-aux][telescope][goto][flip]") {
    bool stalled = false;
    SECTION("Cumulative travel limit") {}
    SECTION("No encoder progress") { stalled = true; }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    if (stalled) {
        fake.set_home_step(0);
        fake.set_home_polls(0, -1);
        fake.set_home_polls(1, -1);
    } else {
        fake.set_home_position(1, (1U << 23) + 1);
    }
    const auto before = fake.requests().size();
    driver->slew_to_coordinates_async(std::fmod(driver->get_right_ascension() + 12.0, 24.0), 88.5);
    bool failed = false;
    const auto deadline = std::chrono::steady_clock::now() + 16s;
    while (!failed && std::chrono::steady_clock::now() < deadline) {
        try {
            static_cast<void>(driver->get_slewing());
        } catch (const AlpacaException& error) {
            CHECK(error.error_code() == AlpacaError::DriverException);
            CHECK(std::string(error.what()).find(stalled ? "stalled" : "180 degrees") != std::string::npos);
            failed = true;
        }
        std::this_thread::sleep_for(10ms);
    }
    REQUIRE(failed);
    CHECK(driver->get_link_fault().empty());
    CHECK_FALSE(driver->get_tracking());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    REQUIRE(fake.wait_for_command_since(before, 0x11, 0x24));
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin() + before, requests.end(),
                       [](const auto& f) { return f.command == 0x04 || f.command == 0x02 || f.command == 0x17; }));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort fences a pending GOTO slow approach",
          "[celestron][nexstar-aux][telescope][goto]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_goto_stuck(true);
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    driver->slew_to_coordinates_async(driver->get_right_ascension(), 88.5);
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    bool sent = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto requests = fake.requests();
        sent = std::count_if(requests.begin(), requests.end(),
                             [](const auto& frame) { return frame.command == 0x02; }) == 2;
        if (sent) break;
        std::this_thread::sleep_for(10ms);
    }
    const auto started = std::chrono::steady_clock::now();
    driver->abort_slew();
    CHECK(std::chrono::steady_clock::now() - started < 100ms);
    const auto stopped = std::chrono::steady_clock::now() + 6s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < stopped) std::this_thread::sleep_for(10ms);
    REQUIRE(sent);
    CHECK_FALSE(driver->get_slewing());
    CHECK(driver->get_tracking());
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin(), requests.end(), [](const auto& frame) { return frame.command == 0x17; }));
    CHECK(std::isfinite(driver->get_right_ascension()));
    driver->set_tracking(false);
    CHECK_FALSE(driver->get_tracking());
    CHECK(std::isfinite(driver->get_right_ascension()));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort never hides stop or tracking restoration failure",
          "[celestron][nexstar-aux][telescope][goto][abort]") {
    bool stop_failure = false;
    SECTION("Stop acknowledgement lost") { stop_failure = true; }
    SECTION("Restored tracking acknowledgement lost") {}
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_goto_stuck(true);
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    driver->set_tracking(true);
    driver->slew_to_coordinates_async(driver->get_right_ascension(), 88.5);
    REQUIRE(fake.wait_for_command_since(0, 0x11, 0x02));
    const auto before = fake.requests().size();
    if (stop_failure)
        fake.lose_stop_reply(0x10);
    else
        fake.lose_tracking_reply(0x10, 2);  // Zero-rate stop, then restored sidereal rate.
    driver->abort_slew();
    const auto deadline = std::chrono::steady_clock::now() + 6s;
    while (driver->get_link_fault().empty() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    REQUIRE(fake.wait_for_command_since(before, 0x11, 0x24));
    const auto requests = fake.requests();
    CHECK(std::count_if(requests.begin() + before, requests.end(), [](const auto& frame) {
              return frame.command == 0x06 && frame.payload == std::vector<std::uint8_t>{0xff, 0xff};
          }) == (stop_failure ? 0 : 1));
}

TEST_CASE("Celestron SkyPortal AUX Driver - GOTO arrival survives WiFi reply latency",
          "[celestron][nexstar-aux][telescope][goto]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    fake.set_reply_delay(25ms);
    fake.set_goto_busy_reads(1);
    const auto ra = driver->get_right_ascension();
    driver->set_slew_settle_time(1);
    driver->slew_to_coordinates_async(ra, 88.5);
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_slewing());
    CHECK(driver->get_tracking());
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - ra, 24.0)) * 15.0 <= 3.0 / 3600.0);
    CHECK(std::abs(driver->get_declination() - 88.5) <= 3.0 / 3600.0);
    CHECK(driver->get_link_fault().empty());
    driver->set_tracking(false);
    CHECK_FALSE(driver->get_tracking());
}

TEST_CASE("Celestron SkyPortal AUX Driver - GOTO failures retain faults instead of idle success",
          "[celestron][nexstar-aux][telescope][goto]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    bool restoration_drift = false;
    bool transport_failure = false;
    std::uint8_t cleanup_lost_ack = 0;
    std::uint8_t cleanup_lost_rate_ack = 0;
    SECTION("Lost RA command acknowledgement") {
        fake.lose_goto_reply(0x10);
        transport_failure = true;
    }
    SECTION("Lost Dec command acknowledgement") {
        fake.lose_goto_reply(0x11);
        transport_failure = true;
    }
    SECTION("Exhausted corrections") { fake.script_slow_offsets(0, {1000, 1000, 1000}); }
    SECTION("Stalled axis") { fake.set_goto_stuck(true); }
    SECTION("Restoration moved outside arrival tolerance") { restoration_drift = true; }
    SECTION("Restoration failure followed by lost RA tracking-off acknowledgement") {
        restoration_drift = true;
        cleanup_lost_rate_ack = 0x10;
        transport_failure = true;
    }
    SECTION("Restoration failure followed by lost Dec tracking-off acknowledgement") {
        restoration_drift = true;
        cleanup_lost_rate_ack = 0x11;
        transport_failure = true;
    }
    SECTION("Arrival failure followed by lost RA stop acknowledgement") {
        fake.script_slow_offsets(0, {1000, 1000, 1000});
        cleanup_lost_ack = 0x10;
        transport_failure = true;
    }
    SECTION("Arrival failure followed by lost Dec stop acknowledgement") {
        fake.script_slow_offsets(0, {1000, 1000, 1000});
        cleanup_lost_ack = 0x11;
        transport_failure = true;
    }
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    driver->set_tracking(true);
    if (restoration_drift) fake.set_tracking_start_offset(1000);
    if (cleanup_lost_ack) fake.lose_stop_reply(cleanup_lost_ack);
    // Three measured, tracked arrivals fail before cleanup clears the rates.
    if (cleanup_lost_rate_ack) fake.lose_tracking_reply(cleanup_lost_rate_ack, cleanup_lost_rate_ack == 0x10 ? 7 : 4);
    driver->slew_to_coordinates_async(driver->get_right_ascension(), 88.5);
    const auto deadline = std::chrono::steady_clock::now() + 16s;
    bool failed = false;
    bool premature_idle = false;
    while (!failed && std::chrono::steady_clock::now() < deadline) {
        try {
            premature_idle = premature_idle || !driver->get_slewing();
        } catch (const AlpacaException& error) {
            CHECK(error.error_code() == AlpacaError::DriverException);
            failed = true;
        }
        std::this_thread::sleep_for(10ms);
    }
    REQUIRE(failed);
    CHECK_FALSE(premature_idle);
    error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    if (transport_failure) {
        REQUIRE_FALSE(driver->get_link_fault().empty());
        error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    } else {
        REQUIRE(driver->get_link_fault().empty());
        CHECK_FALSE(driver->get_tracking());
        const auto requests = fake.requests();
        const auto stop =
            std::find_if(requests.rbegin(), requests.rend(), [](const auto& frame) { return frame.command == 0x24; });
        REQUIRE(stop != requests.rend());
        CHECK(std::count_if(stop.base(), requests.end(), [](const auto& frame) { return frame.command == 0x01; }) >=
              8);  // Three stable comparisons require four position pairs after the stop.
        driver->abort_slew();
        const auto stopped = std::chrono::steady_clock::now() + 5s;
        while (driver->get_slewing() && std::chrono::steady_clock::now() < stopped) std::this_thread::sleep_for(10ms);
        REQUIRE_FALSE(driver->get_slewing());
        CHECK_FALSE(driver->get_tracking());
        CHECK(driver->get_connected());
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Sidereal tracking owns RA and preserves its reference on stop",
          "[celestron][nexstar-aux][telescope][tracking]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    error_code([&] { driver->set_tracking(true); }, AlpacaError::NotConnected);
    error_code([&] { driver->set_tracking_rate(1); }, AlpacaError::InvalidValue);
    driver->set_connected(true);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::InvalidOperation);
    driver->set_connected(false);
    home_for_sync(*driver);
    driver->set_site_latitude(-30);
    CHECK_NOTHROW(driver->set_tracking_rate(0));
    CHECK_NOTHROW(driver->set_tracking(true));
    CHECK(driver->get_tracking());
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_at_home());
    error_code([&] { driver->find_home(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->set_site_latitude(45); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->set_site_longitude(0); }, AlpacaError::InvalidOperation);
    std::this_thread::sleep_for(1200ms);
    const auto ra = driver->get_right_ascension();
    std::this_thread::sleep_for(1200ms);
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - ra, 24.0)) < 0.00005);
    CHECK(driver->get_declination() == 90);
    CHECK(driver->get_link_fault().empty());
    const auto before = fake.requests();
    CHECK(std::count_if(before.begin(), before.end(), [](const auto& frame) {
              return frame.destination == 0x10 && frame.command == 0x06 &&
                     frame.payload == std::vector<std::uint8_t>{0xff, 0xff};
          }) == 1);
    CHECK_NOTHROW(driver->set_tracking(false));
    CHECK_FALSE(driver->get_tracking());
    CHECK_NOTHROW(driver->get_right_ascension());
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
    CHECK_NOTHROW(driver->set_tracking(true));
    const auto idle_abort_start = fake.requests().size();
    driver->abort_slew();
    CHECK_FALSE(driver->get_slewing());
    CHECK(driver->get_tracking());
    CHECK_NOTHROW(driver->get_right_ascension());
    const auto after_idle_abort = fake.requests();
    CHECK(std::none_of(after_idle_abort.begin() + idle_abort_start, after_idle_abort.end(),
                       [](const auto& frame) { return frame.command == 0x06 || frame.command == 0x24; }));
    driver->set_tracking(false);
    CHECK_FALSE(driver->get_tracking());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Sync restores tracking only after verified labels",
          "[celestron][nexstar-aux][telescope][tracking]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    driver->sync_to_coordinates(6, 20);
    CHECK(driver->get_tracking());
    const auto writes = fake.requests();
    CHECK(std::count_if(writes.begin(), writes.end(), [](const auto& frame) {
              return frame.destination == 0x10 && frame.command == 0x06 &&
                     frame.payload == std::vector<std::uint8_t>{0xff, 0xff};
          }) == 2);
    CHECK_FALSE(driver->get_slewing());
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - 6.0, 24.0)) < 0.001);
    CHECK(std::abs(driver->get_declination() - 20.0) < 0.00003);
    std::this_thread::sleep_for(1200ms);
    CHECK(driver->get_link_fault().empty());
    driver->set_tracking(false);
    CHECK_FALSE(driver->get_tracking());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Ambiguous tracking writes fault and stop both axes",
          "[celestron][nexstar-aux][telescope][tracking]") {
    std::uint8_t axis = 0x10;
    bool restoring = false;
    SECTION("RA enable") {}
    SECTION("Dec enable") { axis = 0x11; }
    SECTION("Sync RA restoration") { restoring = true; }
    SECTION("Sync Dec restoration") {
        axis = 0x11;
        restoring = true;
    }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    if (restoring) driver->set_tracking(true);
    const auto before = fake.requests().size();
    fake.set_reply_delay(10ms);  // Keep server consumption observably behind send-only cleanup.
    fake.lose_tracking_reply(axis, restoring ? 2 : 1);
    error_code(
        [&] {
            if (restoring)
                driver->sync_to_coordinates(6, 20);
            else
                driver->set_tracking(true);
        },
        AlpacaError::DriverException);
    error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    REQUIRE(fake.wait_for_command_since(before, 0x11, 0x24));  // Send-only cleanup is not a receive barrier.
    const auto requests = fake.requests();
    for (auto motor : {0x10, 0x11}) {
        CHECK(std::any_of(requests.begin() + before, requests.end(), [motor](const auto& frame) {
            return frame.destination == motor && frame.command == 0x24 && frame.payload == std::vector<std::uint8_t>{0};
        }));
    }
    CHECK_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_target_right_ascension(); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort cancels a pending tracking transition",
          "[celestron][nexstar-aux][telescope][tracking]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    fake.fragment_next_reply();
    auto tracking = std::async(std::launch::async, [&] { driver->set_tracking(true); });
    const bool held = fake.wait_for_fragment();
    driver->abort_slew();
    fake.release_fragment();
    REQUIRE(held);
    REQUIRE(tracking.wait_for(6s) == std::future_status::ready);
    error_code([&] { tracking.get(); }, AlpacaError::InvalidOperation);
    const auto deadline = std::chrono::steady_clock::now() + 6s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(10ms);
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_tracking());
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin(), requests.end(), [](const auto& frame) {
        return frame.command == 0x06 && frame.payload == std::vector<std::uint8_t>{0xff, 0xff};
    }));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Tracking refuses unexpected Dec motion",
          "[celestron][nexstar-aux][telescope][tracking]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_tracking(true);
    fake.script_position_offsets(1, {100});
    const auto deadline = std::chrono::steady_clock::now() + 4s;
    while (driver->get_link_fault().empty() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Unsettled Sync preserves its reference without label writes",
          "[celestron][nexstar-aux][telescope][sync]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    std::vector<std::int32_t> offsets;
    for (int i = 0; i < 30; ++i) offsets.push_back(i % 2 == 0 ? 8 : -8);
    fake.script_position_offsets(1, offsets);
    error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::DriverException);
    CHECK(label_writes(fake) == 0);
    CHECK(driver->get_link_fault().empty());
    CHECK(driver->get_at_home());
    CHECK(std::abs(driver->get_declination() - 90.0) < 1e-9);
    error_code([&] { driver->get_target_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_target_declination(); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Failed label verification or acknowledgement invalidates Sync",
          "[celestron][nexstar-aux][telescope][sync]") {
    std::uint8_t axis = 0x10;
    bool lost_ack = false;
    SECTION("RA retained offset") {}
    SECTION("Dec retained offset") { axis = 0x11; }
    SECTION("RA lost acknowledgement") { lost_ack = true; }
    SECTION("Dec lost acknowledgement") {
        axis = 0x11;
        lost_ack = true;
    }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    home_for_sync(*driver);
    driver->set_target_right_ascension(2);
    driver->set_target_declination(10);
    if (lost_ack)
        fake.lose_label_reply(axis);
    else
        fake.script_label_offsets(axis == 0x10 ? 0 : 1, std::vector<std::int32_t>(30, 8));
    error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::DriverException);
    CHECK(label_writes(fake, 0x10) == 1);
    CHECK(label_writes(fake, 0x11) == (lost_ack && axis == 0x10 ? 0 : 1));
    CHECK(driver->get_target_right_ascension() == 2);
    CHECK(driver->get_target_declination() == 10);
    error_code([&] { driver->get_declination(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_at_home(); }, AlpacaError::DriverException);
    CHECK_FALSE(driver->get_link_fault().empty());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort and disconnect cancel Sync before label writes",
          "[celestron][nexstar-aux][telescope][sync]") {
    bool disconnect = false;
    SECTION("AbortSlew") {}
    SECTION("Disconnect") { disconnect = true; }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    fake.fragment_next_reply();
    auto sync = std::async(std::launch::async, [&] { driver->sync_to_coordinates(6, 20); });
    const bool held = fake.wait_for_fragment();
    if (disconnect)
        driver->disconnect();
    else
        driver->abort_slew();
    fake.release_fragment();
    REQUIRE(held);
    REQUIRE(sync.wait_for(6s) == std::future_status::ready);
    error_code([&] { sync.get(); }, disconnect ? AlpacaError::NotConnected : AlpacaError::InvalidOperation);
    const auto deadline = std::chrono::steady_clock::now() + 6s;
    while ((disconnect ? driver->get_connecting() : driver->get_slewing()) &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    if (disconnect)
        CHECK_FALSE(driver->get_connected());
    else {
        CHECK_FALSE(driver->get_slewing());
        CHECK_FALSE(driver->get_at_home());
    }
    CHECK(label_writes(fake) == 0);
}

TEST_CASE("Celestron SkyPortal AUX Driver - An older failing poll resolves a queued Sync",
          "[celestron][nexstar-aux][telescope][sync]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    fake.truncate_next_reply();
    fake.fragment_next_reply();
    const bool held = fake.wait_for_fragment();
    auto sync = std::async(std::launch::async, [&] { driver->sync_to_coordinates(6, 20); });
    const auto deadline = std::chrono::steady_clock::now() + 500ms;
    while (driver->get_at_home() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    const bool queued = !driver->get_at_home();
    fake.release_fragment();
    REQUIRE(held);
    REQUIRE(queued);
    const auto completed = sync.wait_for(3s);
    if (completed != std::future_status::ready) {
        // Resolve the waiter even under the falsifying mutation, before destroying its future.
        driver->set_connected(false);
    }
    REQUIRE(completed == std::future_status::ready);
    try {
        sync.get();
        FAIL("Expected the poll failure in the Sync caller");
    } catch (const AlpacaException& error) {
        CHECK(error.error_code() == AlpacaError::DriverException);
        CHECK(std::string(error.what()).find("AUX transport returned end-of-file") != std::string::npos);
    }
    CHECK(label_writes(fake) == 0);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Home captures measured labels and healthy logical reconnect retains them",
          "[celestron][nexstar-aux][telescope][home]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_home_position(0, 16763171);
    fake.set_home_position(1, 16148955);
    fake.set_home_polls(1, 5);
    test::FakeTaskClock clock;
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    CHECK_FALSE(driver->get_at_home());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_declination(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_altitude(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_azimuth(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_side_of_pier(); }, AlpacaError::InvalidOperation);
    driver->find_home();
    CHECK(driver->get_slewing());
    CHECK_FALSE(driver->get_at_home());
    error_code([&] { driver->set_site_latitude(40); }, AlpacaError::InvalidOperation);
    for (int i = 0; i < 14; ++i) {
        clock.advance(250ms);
        REQUIRE(clock.wait_for_woken_settled(2s));
        if (i < 6) CHECK_FALSE(driver->get_at_home());
    }
    CHECK_FALSE(driver->get_slewing());
    CHECK(driver->get_at_home());
    const auto before = fake.requests().size();
    // Independent home fixture: north pole, HA +6h, altitude equals site latitude.
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - driver->get_sidereal_time() + 6.0, 24.0)) < 0.001);
    CHECK(std::abs(driver->get_declination() - 90.0) < 1e-9);
    CHECK(std::abs(driver->get_altitude() - 45.0) < 1e-9);
    CHECK(std::abs(std::remainder(driver->get_azimuth(), 360.0)) < 1e-9);
    CHECK(driver->get_azimuth() >= 0.0);
    CHECK(driver->get_azimuth() < 360.0);
    CHECK(driver->get_side_of_pier() == 0);
    driver->set_site_latitude(-30);
    CHECK(std::abs(driver->get_declination() - 90.0) < 1e-9);
    CHECK(std::abs(driver->get_altitude() + 30.0) < 1e-9);
    driver->set_site_latitude(45);
    CHECK(fake.requests().size() == before);
    driver->find_home();
    CHECK(fake.requests().size() == before);
    transition(*driver, clock, false);
    transition(*driver, clock, true);
    CHECK(driver->get_at_home());
    CHECK(std::isfinite(driver->get_right_ascension()));
    CHECK(driver->get_declination() == 90);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Logical reconnect holds ownership without implicit calibration writes",
          "[celestron][nexstar-aux][telescope][continuity]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->sync_to_coordinates(6, 20);
    const auto before = fake.requests().size();
    driver->set_connected(false);
    CHECK_FALSE(driver->get_connected());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::NotConnected);
    CelestronAuxLink competitor;
    error_code([&] { competitor.connect_tcp("127.0.0.1", fake.port(), 100ms); }, AlpacaError::InvalidOperation);
    driver->set_connected(false);
    const auto before_reconnect = fake.requests().size();
    driver->set_connected(true);
    CHECK(driver->get_connected());
    CHECK_FALSE(driver->get_tracking());
    CHECK(std::abs(std::remainder(driver->get_right_ascension() - 6.0, 24.0)) < 0.001);
    CHECK(std::abs(driver->get_declination() - 20.0) < 0.00003);
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin() + before_reconnect, requests.end(),
                       [](const auto& f) { return f.command == 0x24 || f.command == 0x06; }));
    CHECK(std::none_of(requests.begin() + before, requests.end(), [](const auto& f) {
        return f.command == 0x0b || f.command == 0x04 || f.command == 0x02 || f.command == 0x17;
    }));
    driver->set_connected(false);
    driver.reset();
    CHECK_NOTHROW(competitor.connect_tcp("127.0.0.1", fake.port(), 500ms));
    CHECK_NOTHROW(competitor.exchange({0x20, 0x10, 0xfe, {}}, 500ms));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Failed logical continuity never restores calibration",
          "[celestron][nexstar-aux][telescope][continuity]") {
    bool firmware_change = false;
    SECTION("Encoder checkpoint mismatch") {}
    SECTION("Changed motor firmware") { firmware_change = true; }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    driver->set_connected(false);
    const auto before = fake.requests().size();
    if (firmware_change) {
        for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) fake.set_firmware(axis, {7, 18, 8, 0});
    } else {
        fake.script_position_offsets(0, {10, 10, 10, 10});
    }
    driver->set_connected(true);
    CHECK(driver->get_connected());
    CHECK_FALSE(driver->get_at_home());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::InvalidOperation);
    const auto requests = fake.requests();
    CHECK(std::none_of(requests.begin() + before, requests.end(),
                       [](const auto& f) { return f.command == 0x0b || f.command == 0x04; }));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Disconnect checkpoint deadline cannot retain calibration",
          "[celestron][nexstar-aux][telescope][continuity]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    fake.set_reply_delay(50ms);
    error_code([&] { driver->set_connected(false); }, AlpacaError::DriverException);
    CHECK_FALSE(driver->get_connected());
    fake.set_reply_delay(0ms);
    driver->set_connected(true);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    CHECK_FALSE(driver->get_tracking());
}

TEST_CASE("Celestron SkyPortal AUX Driver - A poll fault during disconnect cannot preserve calibration",
          "[celestron][nexstar-aux][telescope][continuity]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    home_for_sync(*driver);
    fake.report_before_reply({0x10, 0x20, 0x50, {2}});
    fake.fragment_next_reply();
    const bool held = fake.wait_for_fragment();
    auto disconnect = std::async(std::launch::async, [&] { driver->set_connected(false); });
    const auto deadline = std::chrono::steady_clock::now() + 1s;
    while (driver->get_connected() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    const bool cleared = !driver->get_connected();
    fake.release_fragment();
    REQUIRE(held);
    REQUIRE(cleared);
    REQUIRE(disconnect.wait_for(5s) == std::future_status::ready);
    disconnect.get();
    driver->set_connected(true);
    CHECK_FALSE(driver->get_at_home());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Abort and deadline stop home without adopting a reference",
          "[celestron][nexstar-aux][telescope][home]") {
    bool timeout = false;
    SECTION("abort") {}
    SECTION("deadline") { timeout = true; }
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_home_polls(0, -1);
    fake.set_home_polls(1, -1);
    test::FakeTaskClock clock;
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    driver->find_home();
    clock.advance(250ms);
    REQUIRE(clock.wait_for_woken_settled(2s));
    if (timeout) {
        clock.advance(250ms);
        REQUIRE(clock.wait_for_woken_settled(2s));
        clock.advance(60s);
        REQUIRE(clock.wait_for_woken_settled(2s));
    } else {
        driver->abort_slew();
    }
    CHECK(driver->get_slewing());
    for (int i = 0; i < 6; ++i) {
        clock.advance(250ms);
        REQUIRE(clock.wait_for_woken_settled(2s));
    }
    if (timeout) {
        try {
            driver->get_slewing();
            FAIL("Expected retained home timeout");
        } catch (const AlpacaException& error) {
            CHECK(error.error_code() == AlpacaError::DriverException);
            const std::string message = error.what();
            CHECK(message.find("RA position=") != std::string::npos);
            CHECK(message.find("; Dec position=") != std::string::npos);
            CHECK(message.find("home_done=0") != std::string::npos);
            CHECK(message.find("moving=1 guiding=0 stable=0") != std::string::npos);
        }
        error_code([&] { driver->get_at_home(); }, AlpacaError::DriverException);
    } else {
        CHECK_FALSE(driver->get_slewing());
        CHECK_FALSE(driver->get_at_home());
        error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    }
    const auto requests = fake.requests();
    for (const auto axis : {0x10, 0x11}) {
        CHECK(std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
                  return frame.destination == axis && frame.command == 0x24;
              }) == 2);
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Excessive home travel stops both axes and retains the failure",
          "[celestron][nexstar-aux][telescope][home]") {
    for (const std::size_t axis : {0U, 1U}) {
        for (const auto& offsets :
             {std::vector<std::int32_t>{400000, 400000, 800000}, std::vector<std::int32_t>{-400000, -400000, -800000},
              std::vector<std::int32_t>{400000, 400000, 0}}) {
            CAPTURE(axis, offsets);
            test::FakeCelestronAuxServer fake;
            fake.enable_cgx_profile();
            fake.set_home_polls(0, -1);
            fake.set_home_polls(1, -1);
            test::FakeTaskClock clock;
            auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
            VirtualDisconnect cleanup{*driver, clock};
            transition(*driver, clock, true);
            const auto tick = [&] {
                clock.advance(250ms);
                REQUIRE(clock.wait_for_woken_settled(2s));
            };
            driver->find_home();
            tick();
            fake.script_position_offsets(axis, offsets);
            tick();
            CHECK(driver->get_slewing());
            const auto below_limit = fake.requests().size();
            tick();
            REQUIRE(fake.requests().size() == below_limit + 8);
            const auto before_limit = fake.requests().size();
            tick();
            tick();
            const auto stopped = fake.requests();
            REQUIRE(std::count_if(stopped.begin() + static_cast<std::ptrdiff_t>(before_limit), stopped.end(),
                                  [](const auto& frame) { return frame.command == 0x24; }) == 2);
            for (int i = 0; i < 6; ++i) tick();
            error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
            error_code([&] { driver->get_at_home(); }, AlpacaError::DriverException);
            error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
            driver->abort_slew();
            for (int i = 0; i < 6; ++i) tick();
            CHECK_FALSE(driver->get_slewing());
            CHECK_FALSE(driver->get_at_home());
            error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
            fake.set_home_polls(0, 1);
            fake.set_home_polls(1, 1);
            driver->find_home();
            for (int i = 0; i < 14; ++i) tick();
            CHECK(driver->get_at_home());
        }
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Defaults", "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(0, info());
    CHECK(driver->get_device_type() == DeviceType::Telescope);
    CHECK(driver->get_device_number() == 0);
    CHECK(driver->get_name() == "Celestron SkyPortal AUX Telescope");
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_connecting());
    CHECK_FALSE(driver->get_device_firmware());
    CHECK(driver->get_link_fault().empty());
    CHECK(driver->get_device_state().empty());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Ambiguous home acknowledgement stops both axes without retry",
          "[celestron][nexstar-aux][telescope][home]") {
    for (const std::uint8_t axis : {0x10, 0x11}) {
        test::FakeCelestronAuxServer fake;
        fake.enable_cgx_profile();
        test::FakeTaskClock clock;
        auto config = info(fake.port());
        config.response_timeout = 100ms;
        auto driver = create_celestron_nexstar_aux_telescope(0, config, clock);
        VirtualDisconnect cleanup{*driver, clock};
        transition(*driver, clock, true);
        fake.set_reply_delay(10ms);
        fake.lose_home_reply(axis);
        const auto before = fake.requests().size();
        driver->find_home();
        clock.advance(250ms);
        REQUIRE(clock.wait_for_woken_settled(2s));
        REQUIRE_FALSE(driver->get_link_fault().empty());
        error_code([&] { driver->get_at_home(); }, AlpacaError::DriverException);
        error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
        // Let the fake consume the send-only stops without assuming send means receipt.
        REQUIRE(fake.wait_for_command_since(before, 0x11, 0x24));
        const auto requests = fake.requests();
        CHECK(std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
                  return frame.destination == axis && frame.command == 0x0b;
              }) == 1);
        for (const auto stopped_axis : {0x10, 0x11}) {
            CHECK(std::count_if(requests.begin(), requests.end(), [stopped_axis](const auto& frame) {
                      return frame.destination == stopped_axis && frame.command == 0x24;
                  }) == 2);
        }
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Disconnect joins home before stopping and closing",
          "[celestron][nexstar-aux][telescope][home]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_home_polls(0, -1);
    fake.set_home_polls(1, -1);
    test::FakeTaskClock clock;
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    driver->find_home();
    clock.advance(250ms);
    REQUIRE(clock.wait_for_woken_settled(2s));
    transition(*driver, clock, false);
    const auto requests = fake.requests();
    CHECK_FALSE(driver->get_connected());
    clock.advance(60s);
    REQUIRE(clock.wait_for_woken_settled(2s));
    CHECK(fake.requests().size() == requests.size());
    for (const auto axis : {0x10, 0x11}) {
        CHECK(std::count_if(requests.begin(), requests.end(), [axis](const auto& frame) {
                  return frame.destination == axis && frame.command == 0x24;
              }) == 2);
    }
    transition(*driver, clock, true);
    CHECK_FALSE(driver->get_at_home());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Device metadata", "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(3, info());
    CHECK(driver->get_device_number() == 3);
    CHECK(driver->get_unique_id() == "Celestron_NexStar_AUX_3");
    CHECK(driver->get_description() == "Celestron SkyPortal AUX mount (home-reference profile)");
    CHECK(driver->get_driver_info() == "AlpacaCore Celestron SkyPortal AUX Driver");
    CHECK(driver->get_driver_version() == kVersion);
    CHECK(driver->get_interface_version() == 4);
    CHECK(driver->get_alignment_mode() == AlignmentMode::GermanPolar);
    CHECK(driver->get_equatorial_system() == EquatorialSystem::Topocentric);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Disconnected behavior", "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(0, info());
    error_code([&] { driver->get_tracking(); }, AlpacaError::NotConnected);
    error_code([&] { driver->get_slewing(); }, AlpacaError::NotConnected);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::NotConnected);
    error_code([&] { driver->get_target_declination(); }, AlpacaError::NotConnected);
    error_code([&] { driver->set_target_declination(12); }, AlpacaError::NotConnected);
    error_code([&] { driver->get_site_latitude(); }, AlpacaError::NotConnected);
    error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::NotConnected);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Unsupported actions", "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(0, info());
    CHECK(driver->get_supported_actions().empty());
    CHECK_FALSE(driver->can_action("anything"));
    error_code([&] { driver->action("anything", ""); }, AlpacaError::ActionNotImplemented);
    error_code([&] { driver->command_blind("x"); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->command_bool("x"); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->command_string("x"); }, AlpacaError::MethodNotImplemented);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Range validation precedes connection",
          "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(0, info());
    error_code([&] { driver->set_target_declination(91); }, AlpacaError::InvalidValue);
    error_code([&] { driver->set_target_right_ascension(-1); }, AlpacaError::InvalidValue);
    error_code([&] { driver->set_site_latitude(std::numeric_limits<double>::quiet_NaN()); }, AlpacaError::InvalidValue);
    error_code([&] { driver->set_site_longitude(181); }, AlpacaError::InvalidValue);
    error_code([&] { driver->set_site_elevation(-301); }, AlpacaError::InvalidValue);
    error_code([&] { driver->get_can_move_axis(3); }, AlpacaError::InvalidValue);
    error_code([&] { driver->move_axis(-1, 0); }, AlpacaError::InvalidValue);
    error_code([&] { driver->pulse_guide(4, 10); }, AlpacaError::InvalidValue);
    error_code([&] { driver->sync_to_coordinates(-1, 20); }, AlpacaError::InvalidValue);
    error_code([&] { driver->sync_to_coordinates(6, 91); }, AlpacaError::InvalidValue);
    error_code([&] { driver->sync_to_coordinates(std::numeric_limits<double>::quiet_NaN(), 20); },
               AlpacaError::InvalidValue);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Connect stops both axes and getters use the snapshot",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_moving(true);
    test::FakeTaskClock clock;
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    CHECK(driver->get_connected());
    CHECK(driver->get_device_firmware() == std::optional<std::string>{"RA 7.17.25, Dec 7.17.25"});
    const auto requests = fake.requests();
    REQUIRE(requests.size() >= 20);
    CHECK(requests[2].command == 0x05);
    CHECK(requests[2].destination == 0x10);
    CHECK(requests[3].command == 0x05);
    CHECK(requests[3].destination == 0x11);
    CHECK(requests[6].command == 0x24);
    CHECK(requests[6].destination == 0x10);
    CHECK(requests[7].command == 0x06);
    CHECK(requests[7].destination == 0x10);
    CHECK(requests[7].payload == std::vector<std::uint8_t>{0, 0, 0});
    CHECK(requests[8].command == 0x24);
    CHECK(requests[8].destination == 0x11);
    CHECK(requests[9].command == 0x06);
    CHECK(requests[9].destination == 0x11);
    CHECK(requests[9].payload == std::vector<std::uint8_t>{0, 0, 0});
    CHECK_FALSE(driver->get_tracking());
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_is_pulse_guiding());
    CHECK_FALSE(driver->get_at_park());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_side_of_pier(); }, AlpacaError::InvalidOperation);
    CHECK_FALSE(driver->get_device_state().empty());
    error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::InvalidOperation);
    CHECK(fake.requests().size() == requests.size());
    transition(*driver, clock, false);
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Stale telemetry stays readable without admitting control writes",
          "[celestron][nexstar-aux][telescope][snapshot]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    test::FakeTaskClock clock;
    auto config = info(fake.port());
    config.response_timeout = 5000ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config, clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    driver->find_home();
    for (int i = 0; i < 20 && driver->get_slewing(); ++i) {
        clock.advance(250ms);
        REQUIRE(clock.wait_for_woken_settled(2s));
    }
    REQUIRE(driver->get_at_home());

    // Hold an in-flight poll while its previous frame ages, without faulting the link.
    fake.fragment_next_reply();
    struct ReleaseReply {
        test::FakeCelestronAuxServer& fake;
        ~ReleaseReply() { fake.release_fragment(); }
    } release{fake};
    clock.advance(1s);
    REQUIRE(fake.wait_for_fragment());
    clock.advance(2100ms);
    const auto before = fake.requests().size();
    CHECK(driver->get_connected());
    CHECK(driver->get_link_fault().empty());
    CHECK(driver->get_at_home());
    CHECK_FALSE(driver->get_at_park());
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_tracking());
    CHECK_FALSE(driver->get_is_pulse_guiding());
    CHECK(std::isfinite(driver->get_right_ascension()));
    CHECK(std::abs(driver->get_declination() - 90.0) < 1e-9);
    CHECK(std::abs(driver->get_altitude() - 45.0) < 1e-9);
    CHECK(std::isfinite(driver->get_azimuth()));
    CHECK(driver->get_side_of_pier() == 0);
    CHECK_NOTHROW(driver->get_destination_side_of_pier(6, 20));
    CHECK(driver->get_right_ascension_rate() == 0);
    CHECK(driver->get_declination_rate() == 0);
    CHECK(driver->get_tracking_rate() == 0);
    const auto earliest = device_state_timestamp(std::chrono::system_clock::now() - 3100ms);
    const auto state = driver->get_device_state();
    const auto latest = device_state_timestamp(std::chrono::system_clock::now() - 3100ms);
    for (const auto name : {"RightAscension", "Declination", "Altitude", "Azimuth", "Slewing", "Tracking", "SideOfPier",
                            "AtHome", "AtPark", "IsPulseGuiding", "TimeStamp"}) {
        CHECK(std::any_of(state.begin(), state.end(), [&](const auto& entry) { return entry.name == name; }));
    }
    for (const auto& entry : state) {
        if (entry.name == "TimeStamp") {
            const auto& stamp = std::get<std::string>(entry.value);
            CHECK(stamp >= earliest);
            CHECK(stamp <= latest);
        }
    }
    error_code([&] { driver->find_home(); }, AlpacaError::DriverException);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::DriverException);
    error_code([&] { driver->set_tracking_rate(0); }, AlpacaError::DriverException);
    error_code([&] { driver->pulse_guide(0, 0); }, AlpacaError::DriverException);
    error_code([&] { driver->slew_to_coordinates_async(6, 20); }, AlpacaError::DriverException);
    error_code([&] { driver->sync_to_coordinates(6, 20); }, AlpacaError::DriverException);
    CHECK(fake.requests().size() == before);

    fake.release_fragment();
    REQUIRE(clock.wait_for_woken_settled(2s));
    CHECK(driver->get_link_fault().empty());
    CHECK_NOTHROW(driver->find_home());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Targets are independent and reset on reconnect",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    driver->set_connected(true);
    error_code([&] { driver->get_target_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_target_declination(); }, AlpacaError::InvalidOperation);
    driver->set_target_right_ascension(6);
    CHECK(driver->get_target_right_ascension() == 6);
    error_code([&] { driver->get_target_declination(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->slew_to_target_async(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->sync_to_target(); }, AlpacaError::InvalidOperation);
    driver->set_target_declination(20);
    CHECK(driver->get_target_declination() == 20);
    driver->set_connected(false);
    driver->set_connected(true);
    error_code([&] { driver->get_target_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_target_declination(); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Unqualified capabilities have exact unsupported errors",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    driver->set_connected(true);
    CHECK_FALSE(driver->get_can_park());
    CHECK_FALSE(driver->get_can_unpark());
    CHECK_FALSE(driver->get_can_set_park());
    CHECK(driver->get_can_find_home());
    CHECK(driver->get_can_slew_async());
    CHECK(driver->get_can_sync());
    CHECK(driver->get_can_pulse_guide());
    CHECK(driver->get_can_set_tracking());
    CHECK(driver->get_axis_rate_ranges(0).empty());
    error_code([&] { driver->park(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->unpark(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->set_park(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->pulse_guide(0, 10); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::InvalidOperation);
}

TEST_CASE("Celestron SkyPortal AUX Driver - Refused identity leaves no endpoint claim",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.set_model(24);
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    error_code([&] { driver->set_connected(true); }, AlpacaError::DriverException);
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
    fake.set_model(0x1788);
    auto replacement = create_celestron_nexstar_aux_telescope(1, info(fake.port()));
    CHECK_NOTHROW(replacement->set_connected(true));
    CHECK(replacement->get_connected());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Modern firmware decodes the big-endian build",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    REQUIRE(fake.ok());
    fake.enable_cgx_profile();
    for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination})
        fake.set_firmware(axis, {7, 17, 0x12, 0x34});
    test::FakeTaskClock clock;
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    CHECK(driver->get_device_firmware() == std::optional<std::string>{"RA 7.17.4660, Dec 7.17.4660"});
    const auto requests = fake.requests();
    REQUIRE(requests.size() >= 6);
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(requests[i].source == 0x20);
        CHECK(requests[i].destination == (i % 2 == 0 ? 0x10 : 0x11));
        CHECK(requests[i].command == (i < 2 ? 0xfe : 0x05));
        CHECK(requests[i].payload.empty());
    }
    CHECK(std::none_of(requests.begin(), requests.end(),
                       [](const auto& frame) { return frame.destination == 0x30 || frame.destination == 0x31; }));
    CHECK(driver->get_can_slew_async());
    CHECK(driver->get_can_set_tracking());
    CHECK(driver->get_can_pulse_guide());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Malformed and mixed firmware identities refuse before takeover",
          "[celestron][nexstar-aux][telescope]") {
    for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) {
        for (const auto& reply :
             std::vector<std::vector<std::uint8_t>>{{}, {7}, {7, 17}, {7, 17, 0}, {7, 17, 0, 25, 0}}) {
            INFO("axis=" << static_cast<int>(axis) << ", firmware length=" << reply.size());
            test::FakeCelestronAuxServer fake;
            REQUIRE(fake.ok());
            fake.enable_cgx_profile();
            fake.set_firmware(axis, reply);
            refused_identity(fake);
        }
    }
    SECTION("both motors using the legacy shape remain unqualified") {
        test::FakeCelestronAuxServer fake;
        REQUIRE(fake.ok());
        fake.enable_cgx_profile();
        for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) {
            fake.set_firmware(axis, {7, 18});
            fake.set_model(axis, {23});
        }
        refused_identity(fake);
    }
    SECTION("matching oversized firmware is malformed, not a new profile") {
        test::FakeCelestronAuxServer fake;
        REQUIRE(fake.ok());
        fake.enable_cgx_profile();
        for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination})
            fake.set_firmware(axis, {7, 17, 0, 25, 0});
        refused_identity(fake);
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Mismatched and old firmware identities refuse before takeover",
          "[celestron][nexstar-aux][telescope]") {
    for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) {
        for (const auto& reply : std::vector<std::vector<std::uint8_t>>{
                 {8, 17, 0, 25}, {7, 18, 0, 25}, {7, 17, 1, 25}, {7, 17, 0, 26}, {6, 49, 0, 25}}) {
            INFO("axis=" << static_cast<int>(axis) << ", firmware=" << static_cast<int>(reply[0]) << "."
                         << static_cast<int>(reply[1]) << "." << static_cast<int>(reply[2]) << "."
                         << static_cast<int>(reply[3]));
            test::FakeCelestronAuxServer fake;
            REQUIRE(fake.ok());
            fake.enable_cgx_profile();
            fake.set_firmware(axis, reply);
            refused_identity(fake);
        }
    }
    SECTION("matching old firmware is still refused") {
        test::FakeCelestronAuxServer fake;
        REQUIRE(fake.ok());
        fake.enable_cgx_profile();
        for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) fake.set_firmware(axis, {6, 49, 0, 25});
        refused_identity(fake);
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Both motor models require the exact modern CGX identity",
          "[celestron][nexstar-aux][telescope]") {
    for (const auto axis : {CgxAxis::RightAscension, CgxAxis::Declination}) {
        for (const auto& reply : std::vector<std::vector<std::uint8_t>>{
                 {}, {23}, {24}, {0x17}, {0x17, 0x88, 0}, {0x17, 0x89}, {0x18, 0x88}, {0x88, 0x17}}) {
            INFO("axis=" << static_cast<int>(axis) << ", model length=" << reply.size());
            test::FakeCelestronAuxServer fake;
            REQUIRE(fake.ok());
            fake.enable_cgx_profile();
            fake.set_model(axis, reply);
            refused_identity(fake);
        }
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Lost stop acknowledgement still sends the sibling stop",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.lose_stop_reply(0x10);
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    CHECK_THROWS_AS(driver->set_connected(true), AlpacaException);
    REQUIRE(fake.wait_for_requests(9));
    const auto requests = fake.requests();
    CHECK(std::any_of(requests.begin(), requests.end(),
                      [](const auto& frame) { return frame.destination == 0x11 && frame.command == 0x24; }));
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Faulted telemetry becomes lost without autonomous reconnect",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    test::FakeTaskClock clock;
    auto config = info(fake.port());
    config.response_timeout = 100ms;  // Exercise fault ageing, not a multi-second wire timeout.
    auto driver = create_celestron_nexstar_aux_telescope(0, config, clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    REQUIRE(clock.wait_for_wait_count(2, 2s));
    SECTION("unreferenced idle connection") {}
    SECTION("completed home does not survive an idle communication failure") {
        driver->find_home();
        for (int i = 0; i < 12; ++i) {
            clock.advance(250ms);
            REQUIRE(clock.wait_for_woken_settled(2s));
        }
        REQUIRE(driver->get_at_home());
        REQUIRE_FALSE(driver->get_slewing());
    }
    fake.set_muted(true);
    clock.advance(1s);
    REQUIRE(clock.wait_for_woken_settled(5s));
    CHECK(driver->get_connected());
    CHECK_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_at_home(); }, AlpacaError::DriverException);
    error_code([&] { driver->find_home(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_declination(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_altitude(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_azimuth(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_side_of_pier(); }, AlpacaError::DriverException);
    const auto requests = fake.requests().size();
    clock.advance(30s);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (driver->get_connected() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    CHECK_FALSE(driver->get_connected());
    CHECK(driver->get_device_state().empty());
    CHECK_FALSE(driver->get_device_firmware());
    CHECK_FALSE(driver->get_link_fault().empty());
    CHECK(fake.requests().size() == requests);
    fake.set_muted(false);
    transition(*driver, clock, true);
    CHECK_FALSE(driver->get_at_home());
    CHECK_FALSE(driver->get_slewing());
    CHECK(driver->get_link_fault().empty());
    const auto reconnected = fake.requests();
    CHECK(std::none_of(reconnected.begin() + requests, reconnected.end(),
                       [](const auto& frame) { return frame.command == 0x0b || frame.command == 0x04; }));
}

TEST_CASE("Celestron SkyPortal AUX Driver - Low battery reports remain diagnostic-only",
          "[celestron][nexstar-aux][telescope]") {
    for (const std::uint8_t axis : {0x10, 0x11}) {
        INFO("report axis=" << static_cast<int>(axis));
        test::FakeCelestronAuxServer fake;
        REQUIRE(fake.ok());
        fake.enable_cgx_profile();
        test::FakeTaskClock clock;
        auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
        VirtualDisconnect cleanup{*driver, clock};
        transition(*driver, clock, true);
        const auto before = fake.requests().size();
        fake.report_before_reply({axis, 0x20, 0x50, {0}});
        clock.advance(1s);
        REQUIRE(clock.wait_for_woken_settled(2s));
        CHECK(driver->get_connected());
        CHECK(driver->get_link_fault().empty());
        CHECK_FALSE(driver->get_tracking());
        CHECK_FALSE(driver->get_slewing());
        const auto requests = fake.requests();
        REQUIRE(requests.size() == before + 7);  // Six status queries and the report ACK.
        CHECK(std::count_if(requests.begin() + before, requests.end(), [axis](const auto& frame) {
                  return frame.source == 0x20 && frame.destination == axis && frame.command == 0x50 &&
                         frame.payload.empty();
              }) == 1);
        clock.advance(1s);
        REQUIRE(clock.wait_for_woken_settled(2s));
        CHECK(driver->get_link_fault().empty());
        CHECK_FALSE(driver->get_tracking());
        CHECK_FALSE(driver->get_slewing());
        CHECK(fake.requests().size() == requests.size() + 6);
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Controller motion reports latch until virtual-time link loss",
          "[celestron][nexstar-aux][telescope]") {
    std::uint8_t command = 0x50;
    std::uint8_t code = 1;
    SECTION("motion-limit warning") {}
    SECTION("controller error including code zero") {
        command = 0x51;
        code = 0;
    }
    for (const std::uint8_t axis : {0x10, 0x11}) {
        INFO("report axis=" << static_cast<int>(axis) << ", command=" << static_cast<int>(command));
        test::FakeCelestronAuxServer fake;
        REQUIRE(fake.ok());
        fake.enable_cgx_profile();
        test::FakeTaskClock clock;
        auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
        VirtualDisconnect cleanup{*driver, clock};
        transition(*driver, clock, true);
        CHECK_FALSE(driver->get_tracking());
        CHECK_FALSE(driver->get_slewing());
        const auto before = fake.requests().size();
        fake.report_before_reply({axis, 0x20, command, {code}});
        clock.advance(1s);
        REQUIRE(clock.wait_for_woken_settled(2s));
        CHECK(driver->get_connected());
        const auto fault = driver->get_link_fault();
        CHECK(fault.find("controller reports a motion limit or error") != std::string::npos);
        // The previous snapshot is only one second old: its TTL must not hide a known fault.
        error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
        error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
        const auto requests = fake.requests();
        REQUIRE(requests.size() == before + 7);
        CHECK(std::count_if(requests.begin() + before, requests.end(), [axis, command](const auto& frame) {
                  return frame.source == 0x20 && frame.destination == axis && frame.command == command &&
                         frame.payload.empty();
              }) == 1);

        // The fake remains responsive, but a report fault cannot clear itself by polling.
        for (const auto step : {1s, 28s}) {
            clock.advance(step);
            REQUIRE(clock.wait_for_woken_settled(2s));
            CHECK(driver->get_connected());
            CHECK_FALSE(driver->get_connecting());
            CHECK(driver->get_link_fault() == fault);
            error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
            error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
            CHECK(fake.requests().size() == requests.size());
        }
        clock.advance(1s);  // Exactly 30 seconds since the fault latched.
        REQUIRE(clock.wait_for_woken_settled(2s));
        CHECK_FALSE(driver->get_connected());
        CHECK_FALSE(driver->get_connecting());
        CHECK_FALSE(driver->get_device_firmware());
        CHECK(driver->get_device_state().empty());
        CHECK(driver->get_link_fault() == fault);
        error_code([&] { driver->get_tracking(); }, AlpacaError::NotConnected);
        error_code([&] { driver->get_slewing(); }, AlpacaError::NotConnected);
        clock.advance(60s);
        REQUIRE(clock.wait_for_woken_settled(2s));
        CHECK_FALSE(driver->get_connected());
        CHECK(driver->get_link_fault() == fault);
        CHECK(fake.requests().size() == requests.size());
    }
}

TEST_CASE("Celestron SkyPortal AUX Driver - Sync disconnect cannot disappear during a sync connect",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.fragment_next_reply();
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()));
    auto connecting = std::async(std::launch::async, [&] { driver->set_connected(true); });
    const bool held = fake.wait_for_fragment();
    std::promise<void> started;
    auto disconnecting = std::async(std::launch::async, [&] {
        started.set_value();
        driver->set_connected(false);
    });
    started.get_future().wait();
    fake.release_fragment();
    REQUIRE(held);
    connecting.get();
    disconnecting.get();
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Slow fragmented WiFi replies use the configured budget",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    fake.fragment_next_reply();
    auto config = info(fake.port());
    config.response_timeout = 1500ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    auto connecting = std::async(std::launch::async, [&] { driver->set_connected(true); });
    const bool held = fake.wait_for_fragment();
    // This is a held wire response, not a driver timer: the old fixed 250 ms
    // exchange would already have failed while this packet was still arriving.
    const auto before_tail = connecting.wait_for(300ms);
    fake.release_fragment();
    REQUIRE(held);
    CHECK(before_tail == std::future_status::timeout);
    CHECK_NOTHROW(connecting.get());
    CHECK(driver->get_connected());
}

TEST_CASE("Celestron SkyPortal AUX Driver - Async teardown retains an unverified stop reason",
          "[celestron][nexstar-aux][telescope]") {
    test::FakeCelestronAuxServer fake;
    fake.enable_cgx_profile();
    auto config = info(fake.port());
    config.response_timeout = 100ms;
    auto driver = create_celestron_nexstar_aux_telescope(0, config);
    driver->set_connected(true);
    fake.set_muted(true);
    driver->disconnect();
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (driver->get_connecting() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    CHECK_FALSE(driver->get_connecting());
    CHECK_FALSE(driver->get_connected());
    CHECK(driver->get_link_fault().find("Stop verification failed") != std::string::npos);
    CHECK_FALSE(driver->get_device_firmware());
}
