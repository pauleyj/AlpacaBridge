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
#include <alpacacore/vendor/celestron/celestron_protocol_wrapper.h>

#include <algorithm>
#include <future>

#include "catch2_compat.h"
#include "fake_celestron_aux_server.h"
#include "fake_celestron_cgx_mount.h"

using namespace std::chrono_literals;
namespace AlpacaError = alpacacore::AlpacaError;
using alpacacore::AlpacaException;
using alpacacore::test::FakeCelestronAuxServer;
using namespace alpacacore::vendor::celestron;

TEST_CASE("Celestron AUX TCP - typed commands select the live source while raw exchanges retain their address",
          "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
    const auto response = link.exchange(AuxFrame{0x21, 0x11, 0x01, {}}, 500ms);
    CHECK(response.destination == 0x21);
    const auto requests = server.requests();
    REQUIRE(requests.size() == 2);
    CHECK(requests[0].source == 0x20);
    CHECK(requests[1].source == 0x21);
    alpacacore::test::FakeCelestronCgxMount usb;
    REQUIRE(usb.ok());
    link.connect_serial(usb.slave_path());
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0);
    const auto serial_requests = usb.requests();
    REQUIRE(serial_requests.size() == 1);
    CHECK(serial_requests[0].source == 0x00);
}

TEST_CASE("Celestron AUX TCP - an incomplete frame closes the link before any stop can append bytes",
          "[celestron][aux][tcp]") {
    for (const std::uint8_t command : {0x02, 0x50, 0x24}) {
        INFO("Partial command " << static_cast<int>(command));
        FakeCelestronAuxServer server;
        REQUIRE(server.ok());
        std::size_t remaining = command == 0x02 ? 6 : 3;
        std::size_t transmitted = 0;
        bool partial = false;
        CelestronAuxLink link([&](int fd, const void* data, std::size_t size, int flags) -> std::ptrdiff_t {
            const auto* bytes = static_cast<const std::uint8_t*>(data);
            if (!partial && size >= 5 && bytes[4] == command) partial = true;
            if (partial && remaining == 0) {
                errno = EAGAIN;
                return -1;
            }
            const auto count = ::send(fd, data, partial ? std::min(size, remaining) : size, flags);
            if (count > 0) {
                transmitted += static_cast<std::size_t>(count);
                if (partial) remaining -= static_cast<std::size_t>(count);
            }
            return count;
        });
        link.connect_tcp("127.0.0.1", server.port(), 500ms);
        const auto generation = link.session_generation();
        if (command == 0x02) {
            CHECK_THROWS_AS(link.goto_axis_fast(CgxAxis::RightAscension, 0x6a0000, 20ms), AlpacaException);
        } else if (command == 0x50) {
            server.report_before_reply();
            CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension, 20ms), AlpacaException);
        } else {
            CHECK_THROWS_AS(link.send_stop(CgxAxis::RightAscension, 20ms), AlpacaException);
        }
        REQUIRE(partial);
        REQUIRE(remaining == 0);
        CHECK_FALSE(link.is_connected());
        CHECK_FALSE(link.is_healthy());
        CHECK(link.session_generation() > generation);
        const auto before_stops = transmitted;
        CHECK_THROWS_AS(link.send_stop(CgxAxis::RightAscension, 20ms), AlpacaException);
        CHECK_THROWS_AS(link.send_stop(CgxAxis::Declination, 20ms), AlpacaException);
        CHECK(transmitted == before_stops);
        CelestronAuxLink replacement;
        CHECK_NOTHROW(replacement.connect_tcp("127.0.0.1", server.port(), 500ms));
        CHECK(replacement.get_axis_position(CgxAxis::RightAscension) == 0x123456);
    }
}

TEST_CASE("Celestron AUX TCP - fragmented replies use the WiFi application address", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    server.fragment_next_reply();
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    auto result =
        std::async(std::launch::async, [&] { return link.get_axis_position(CgxAxis::RightAscension, 2500ms); });
    const bool held = server.wait_for_fragment();
    const auto before_tail = result.wait_for(0ms);
    server.release_fragment();
    REQUIRE(held);
    CHECK(before_tail == std::future_status::timeout);
    CHECK(result.get() == 0x123456);
    const auto requests = server.requests();
    REQUIRE(requests.size() == 1);
    CHECK(requests[0].source == 0x20);
    CHECK(requests[0].destination == 0x10);
}

TEST_CASE("Celestron AUX TCP - queued I/O expires without dispatching or poisoning the active exchange",
          "[celestron][aux][tcp]") {
    for (const int operation : {0, 1, 2}) {
        CAPTURE(operation);
        FakeCelestronAuxServer server;
        REQUIRE(server.ok());
        CelestronAuxLink link;
        link.connect_tcp("127.0.0.1", server.port(), 500ms);
        const auto generation = link.session_generation();
        server.fragment_next_reply();
        auto active =
            std::async(std::launch::async, [&] { return link.get_axis_position(CgxAxis::RightAscension, 2500ms); });
        const bool held = server.wait_for_fragment();
        auto queued = std::async(std::launch::async, [&] {
            try {
                if (operation == 0)
                    link.send_stop(CgxAxis::Declination, 20ms);
                else if (operation == 1)
                    static_cast<void>(link.get_axis_position(CgxAxis::Declination, 20ms));
                else
                    link.connect_tcp("127.0.0.1", server.port(), 20ms);
                return false;
            } catch (const AlpacaException& error) {
                return error.error_code() == AlpacaError::DriverException;
            }
        });
        const auto status = queued.wait_for(250ms);
        server.release_fragment();
        REQUIRE(held);
        CHECK(status == std::future_status::ready);
        CHECK(queued.get());
        CHECK(active.get() == 0x123456);
        CHECK(link.is_healthy());
        CHECK_FALSE(link.is_desynchronized());
        CHECK(link.session_generation() == generation);
        REQUIRE(server.requests().size() == 1);
        CHECK(link.get_axis_position(CgxAxis::Declination) == 0x123456);
    }
}

TEST_CASE("Celestron AUX serial - queued exchange and stop retain their post-lock I/O budget",
          "[celestron][aux][cgx][serial]") {
    for (const bool stop : {false, true}) {
        CAPTURE(stop);
        alpacacore::test::FakeCelestronCgxMount usb;
        REQUIRE(usb.ok());
        CelestronAuxLink link;
        link.connect_serial(usb.slave_path());
        const auto generation = link.session_generation();
        usb.fragment_next_reply();
        auto active =
            std::async(std::launch::async, [&] { return link.get_axis_position(CgxAxis::RightAscension, 2500ms); });
        const bool held = usb.wait_for_reply_fragment(2s);
        auto queued = std::async(std::launch::async, [&] {
            if (stop) {
                link.send_stop(CgxAxis::Declination, 20ms);
                return std::uint32_t{0};
            }
            return link.get_axis_position(CgxAxis::Declination, 20ms);
        });
        const auto before_release = queued.wait_for(100ms);
        usb.release_reply_tail();
        REQUIRE(held);
        CHECK(before_release == std::future_status::timeout);
        CHECK(active.get() == 0);
        CHECK(queued.get() == 0);
        CHECK(link.is_desynchronized() == stop);
        CHECK(link.is_healthy() == !stop);
        CHECK((link.session_generation() != generation) == stop);
    }
}

TEST_CASE("Celestron AUX TCP - acknowledges coalesced controller reports without losing the reply",
          "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    server.report_before_reply();
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
    REQUIRE(server.wait_for_requests(2));
    const auto requests = server.requests();
    CHECK(requests[1].source == 0x20);
    CHECK(requests[1].destination == 0x10);
    CHECK(requests[1].command == 0x50);
    CHECK(requests[1].payload.empty());
    const auto reports = link.take_controller_reports();
    REQUIRE(reports.size() == 1);
    CHECK(reports[0].payload == std::vector<std::uint8_t>{1});
    CHECK(link.take_controller_reports().empty());
}

TEST_CASE("Celestron AUX TCP - timeout poisons exchanges but transmits both axis stops", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    server.set_muted(true);
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension, 20ms), AlpacaException);
    CHECK(link.is_desynchronized());
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    link.send_stop(CgxAxis::RightAscension, 100ms);
    link.send_stop(CgxAxis::Declination, 100ms);
    REQUIRE(server.wait_for_requests(3));
    const auto requests = server.requests();
    REQUIRE(requests.size() == 3);
    CHECK(requests[1].destination == 0x10);
    CHECK(requests[2].destination == 0x11);
    CHECK(requests[1].command == 0x24);
    CHECK(requests[2].command == 0x24);
    CHECK(requests[1].payload == std::vector<std::uint8_t>{0});
    CHECK(requests[2].payload == std::vector<std::uint8_t>{0});
    CHECK(link.is_desynchronized());
    link.disconnect();
    server.set_muted(false);
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - report overflow poisons the link without discarding pending reports",
          "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto generation = link.session_generation();
    for (int i = 0; i < 16; ++i) {
        server.report_before_reply();
        REQUIRE(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
    }
    server.report_before_reply();
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK(link.is_desynchronized());
    CHECK_FALSE(link.is_healthy());
    CHECK(link.session_generation() != generation);
    const auto reports = link.take_controller_reports();
    REQUIRE(reports.size() == 16);
    for (const auto& report : reports) {
        CHECK(report.command == 0x50);
        CHECK(report.payload == std::vector<std::uint8_t>{1});
    }
    // Consuming reports cannot make a delayed reply safe to match again.
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    link.send_stop(CgxAxis::RightAscension, 100ms);
    link.send_stop(CgxAxis::Declination, 100ms);
    REQUIRE(server.wait_for_requests(36));
    const auto requests = server.requests();
    REQUIRE(requests.size() == 36);
    CHECK(requests[34].destination == 0x10);
    CHECK(requests[35].destination == 0x11);
    CHECK(requests[34].command == 0x24);
    CHECK(requests[35].command == 0x24);
    CHECK(requests[34].payload == std::vector<std::uint8_t>{0});
    CHECK(requests[35].payload == std::vector<std::uint8_t>{0});
}

TEST_CASE("Celestron AUX TCP - HC and AUX exclude the same endpoint and release claims", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink first, second;
    first.connect_tcp("127.0.0.1", server.port(), 500ms);
    try {
        second.connect_tcp("::ffff:127.0.0.1", server.port(), 500ms);
        FAIL("IPv4-mapped IPv6 must not bypass ownership");
    } catch (const AlpacaException& error) {
        CHECK(error.error_code() == AlpacaError::InvalidOperation);
    }
    ConnectionInfo info;
    info.type = ConnectionType::Network;
    info.host = "127.0.0.1";
    info.tcp_port = server.port();
    auto& hc = CelestronProtocolWrapper::instance();
    CHECK_THROWS_AS(hc.connect(info), AlpacaException);
    first.disconnect();
    REQUIRE(hc.connect(info));
    CHECK_THROWS_AS(second.connect_tcp(info.host, info.tcp_port, 500ms), AlpacaException);
    hc.disconnect();
    second.connect_tcp(info.host, info.tcp_port, 500ms);
    CHECK(second.get_axis_position(CgxAxis::RightAscension) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - checksum failure poisons and EOF closes the link", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    server.corrupt_next_reply();
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK(link.is_desynchronized());
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    link.disconnect();
    server.close_next_connection();
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK_FALSE(link.is_connected());
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - continuous unrelated traffic cannot extend the deadline", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    server.unrelated_until_deadline();
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto start = std::chrono::steady_clock::now();
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension, 20ms), AlpacaException);
    CHECK(std::chrono::steady_clock::now() - start < 500ms);
    CHECK(link.is_desynchronized());
}

TEST_CASE("Celestron AUX TCP - validates numeric endpoints before connecting", "[celestron][aux][tcp]") {
    CelestronAuxLink link;
    for (const auto port : {0, 65536}) {
        try {
            link.connect_tcp("127.0.0.1", port, 500ms);
            FAIL("Expected invalid port refusal");
        } catch (const AlpacaException& error) {
            CHECK(error.error_code() == AlpacaError::InvalidValue);
        }
    }
    CHECK_THROWS_AS(link.connect_tcp("localhost", 2000, 500ms), AlpacaException);
    CHECK_THROWS_AS(link.connect_tcp(std::string("127.0.0.1\0ignored", 17), 2000, 500ms), AlpacaException);
    CHECK_THROWS_AS(link.connect_tcp("127.0.0.1", 2000, 0ms), AlpacaException);
    CHECK_THROWS_AS(link.connect_tcp("127.0.0.1", 2000, 5001ms), AlpacaException);
    CHECK_FALSE(link.is_connected());
}

TEST_CASE("Celestron AUX TCP - a late reply cannot acknowledge another exchange", "[celestron][aux][tcp]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    server.fragment_next_reply();
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    auto result =
        std::async(std::launch::async, [&] { return link.get_axis_position(CgxAxis::RightAscension, 200ms); });
    const bool held = server.wait_for_fragment();
    // Always release the fake before asserting, including on an unexpected return.
    bool failed = false;
    try {
        static_cast<void>(result.get());
    } catch (const AlpacaException&) {
        failed = true;
    }
    server.release_fragment();
    REQUIRE(held);
    REQUIRE(failed);
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK(server.requests().size() == 1);
    link.disconnect();
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - a refused connect releases endpoint ownership", "[celestron][aux][tcp]") {
    // Reserve an ephemeral port without listening: connects are refused, not timed out.
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    REQUIRE(fd >= 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const int bound = ::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    socklen_t size = sizeof(address);
    const int named = ::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &size);
    ::close(fd);
    REQUIRE(bound == 0);
    REQUIRE(named == 0);
    const int port = ntohs(address.sin_port);
    CelestronAuxLink link;
    CHECK_THROWS_AS(link.connect_tcp("127.0.0.1", port, 500ms), AlpacaException);
    CHECK_FALSE(link.is_connected());
    CelestronEndpointClaim claim;
    CHECK_NOTHROW(claim.claim(reinterpret_cast<const sockaddr*>(&address), port));
}
