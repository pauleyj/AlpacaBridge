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

#include <array>
#include <cerrno>
#include <vector>

#include "catch2_compat.h"
#include "fake_celestron_aux_server.h"

using namespace std::chrono_literals;
using alpacacore::AlpacaException;
using alpacacore::test::FakeCelestronAuxServer;
using namespace alpacacore::vendor::celestron;

TEST_CASE("Celestron AUX TCP - Dec errors and broadcast reports receive exact reversed-address ACKs",
          "[celestron][aux][tcp][reports]") {
    const std::array<AuxFrame, 3> reports{
        {{0x11, 0x20, 0x51, {0x82}}, {0x11, 0x00, 0x51, {0x7f}}, {0x10, 0x00, 0x50, {1}}}};
    const std::array<std::vector<std::uint8_t>, 3> acknowledgements{
        {{0x3b, 3, 0x20, 0x11, 0x51, 0x7b}, {0x3b, 3, 0x00, 0x11, 0x51, 0x9b}, {0x3b, 3, 0x00, 0x10, 0x50, 0x9d}}};
    for (std::size_t i = 0; i < reports.size(); ++i) {
        CAPTURE(i);
        FakeCelestronAuxServer server;
        REQUIRE(server.ok());
        std::vector<std::uint8_t> transmitted;
        CelestronAuxLink link([&](int fd, const void* data, std::size_t size, int flags) -> std::ptrdiff_t {
            const auto count = ::send(fd, data, size, flags);
            if (count > 0) {
                const auto* bytes = static_cast<const std::uint8_t*>(data);
                transmitted.insert(transmitted.end(), bytes, bytes + count);
            }
            return count;
        });
        link.connect_tcp("127.0.0.1", server.port(), 500ms);
        const auto generation = link.session_generation();
        server.report_before_reply(reports[i]);
        REQUIRE(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
        auto expected = std::vector<std::uint8_t>{0x3b, 3, 0x20, 0x10, 0x01, 0xcc};
        expected.insert(expected.end(), acknowledgements[i].begin(), acknowledgements[i].end());
        CHECK(transmitted == expected);
        REQUIRE(server.wait_for_requests(2));
        CHECK(server.requests().size() == 2);
        const auto queued = link.take_controller_reports();
        REQUIRE(queued.size() == 1);
        CHECK(queued[0].source == reports[i].source);
        CHECK(queued[0].destination == reports[i].destination);
        CHECK(queued[0].command == reports[i].command);
        CHECK(queued[0].payload == reports[i].payload);
        CHECK(link.take_controller_reports().empty());
        CHECK(link.is_healthy());
        CHECK(link.session_generation() == generation);
    }
}

TEST_CASE("Celestron AUX TCP - reports from other sources or destinations are ignored without ACKs",
          "[celestron][aux][tcp][reports]") {
    for (const auto& report : std::array<AuxFrame, 4>{
             {{0x12, 0x20, 0x50, {1}}, {0x20, 0x20, 0x51, {1}}, {0x10, 0x21, 0x50, {1}}, {0x11, 0x10, 0x51, {1}}}}) {
        CAPTURE(report.source, report.destination);
        FakeCelestronAuxServer server;
        REQUIRE(server.ok());
        CelestronAuxLink link;
        link.connect_tcp("127.0.0.1", server.port(), 500ms);
        const auto generation = link.session_generation();
        server.report_before_reply(report);
        REQUIRE(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
        CHECK(link.take_controller_reports().empty());
        REQUIRE(link.get_axis_position(CgxAxis::Declination) == 0x123456);
        REQUIRE(server.wait_for_requests(2));
        const auto requests = server.requests();
        REQUIRE(requests.size() == 2);
        CHECK(requests[0].command == 0x01);
        CHECK(requests[1].command == 0x01);
        CHECK(link.is_healthy());
        CHECK_FALSE(link.is_desynchronized());
        CHECK(link.session_generation() == generation);
    }
}

TEST_CASE("Celestron AUX TCP - malformed controller report payloads poison the exchange before ACK",
          "[celestron][aux][tcp][reports]") {
    for (const std::uint8_t command : {0x50, 0x51}) {
        for (const std::uint8_t source : {0x10, 0x11}) {
            for (const std::uint8_t destination : {0x20, 0x00}) {
                for (const auto& payload : {std::vector<std::uint8_t>{}, std::vector<std::uint8_t>{1, 2}}) {
                    CAPTURE(command, source, destination, payload.size());
                    FakeCelestronAuxServer server;
                    REQUIRE(server.ok());
                    std::size_t sends = 0;
                    CelestronAuxLink link([&](int fd, const void* data, std::size_t size, int flags) -> std::ptrdiff_t {
                        ++sends;
                        return ::send(fd, data, size, flags);
                    });
                    link.connect_tcp("127.0.0.1", server.port(), 500ms);
                    const auto generation = link.session_generation();
                    server.report_before_reply(AuxFrame{source, destination, command, payload});
                    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
                    CHECK(sends == 1);
                    CHECK(link.is_connected());
                    CHECK(link.is_desynchronized());
                    CHECK_FALSE(link.is_healthy());
                    CHECK(link.session_generation() > generation);
                    CHECK(link.take_controller_reports().empty());
                    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::Declination), AlpacaException);
                    CHECK(sends == 1);
                }
            }
        }
    }
}

TEST_CASE("Celestron AUX TCP - a report after the reply is acknowledged and queued by the next exchange",
          "[celestron][aux][tcp][reports]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto generation = link.session_generation();
    server.report_after_reply(AuxFrame{0x11, 0x00, 0x51, {2}});
    REQUIRE(link.get_axis_position(CgxAxis::RightAscension) == 0x123456);
    CHECK(link.take_controller_reports().empty());
    REQUIRE(link.get_axis_position(CgxAxis::Declination) == 0x123456);
    REQUIRE(server.wait_for_requests(3));
    const auto requests = server.requests();
    REQUIRE(requests.size() == 3);
    CHECK(requests[1].command == 0x01);
    CHECK(requests[2].source == 0x00);
    CHECK(requests[2].destination == 0x11);
    CHECK(requests[2].command == 0x51);
    CHECK(requests[2].payload.empty());
    const auto reports = link.take_controller_reports();
    REQUIRE(reports.size() == 1);
    CHECK(reports[0].source == 0x11);
    CHECK(reports[0].destination == 0x00);
    CHECK(reports[0].command == 0x51);
    CHECK(reports[0].payload == std::vector<std::uint8_t>{2});
    CHECK(link.take_controller_reports().empty());
    CHECK(link.is_healthy());
    CHECK(link.session_generation() == generation);
}

TEST_CASE("Celestron AUX TCP - fatal report ACK send failure closes the link and releases endpoint ownership",
          "[celestron][aux][tcp][reports]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    bool failed_ack = false;
    CelestronAuxLink link([&](int fd, const void* data, std::size_t size, int flags) -> std::ptrdiff_t {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        if (size >= 5 && bytes[4] == 0x51) {
            failed_ack = true;
            errno = EPIPE;
            return -1;
        }
        return ::send(fd, data, size, flags);
    });
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto generation = link.session_generation();
    server.report_before_reply(AuxFrame{0x11, 0x20, 0x51, {1}});
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    REQUIRE(failed_ack);
    CHECK_FALSE(link.is_connected());
    CHECK_FALSE(link.is_healthy());
    CHECK(link.session_generation() > generation);
    CHECK(link.take_controller_reports().empty());
    CHECK_THROWS_AS(link.send_stop(CgxAxis::RightAscension, 100ms), AlpacaException);
    CHECK_THROWS_AS(link.send_stop(CgxAxis::Declination, 100ms), AlpacaException);
    CelestronAuxLink replacement;
    CHECK_NOTHROW(replacement.connect_tcp("127.0.0.1", server.port(), 500ms));
    CHECK(replacement.get_axis_position(CgxAxis::RightAscension) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - a checksum-invalid final frame cannot satisfy an exchange",
          "[celestron][aux][tcp][reports]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto generation = link.session_generation();
    server.corrupt_next_reply();
    server.close_after_reply();
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK(link.is_desynchronized());
    CHECK_FALSE(link.is_healthy());
    CHECK(link.session_generation() > generation);
    CHECK(link.take_controller_reports().empty());
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::Declination), AlpacaException);
    link.disconnect();
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    CHECK(link.get_axis_position(CgxAxis::Declination) == 0x123456);
}

TEST_CASE("Celestron AUX TCP - a truncated final frame closes the link and releases endpoint ownership",
          "[celestron][aux][tcp][reports]") {
    FakeCelestronAuxServer server;
    REQUIRE(server.ok());
    CelestronAuxLink link;
    link.connect_tcp("127.0.0.1", server.port(), 500ms);
    const auto generation = link.session_generation();
    server.truncate_next_reply();
    CHECK_THROWS_AS(link.get_axis_position(CgxAxis::RightAscension), AlpacaException);
    CHECK_FALSE(link.is_connected());
    CHECK_FALSE(link.is_healthy());
    CHECK(link.session_generation() > generation);
    CHECK(link.take_controller_reports().empty());
    CHECK_THROWS_AS(link.send_stop(CgxAxis::Declination, 100ms), AlpacaException);
    CelestronAuxLink replacement;
    CHECK_NOTHROW(replacement.connect_tcp("127.0.0.1", server.port(), 500ms));
    CHECK(replacement.get_axis_position(CgxAxis::Declination) == 0x123456);
}
