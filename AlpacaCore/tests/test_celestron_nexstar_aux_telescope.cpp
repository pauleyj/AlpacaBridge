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
}  // namespace

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

TEST_CASE("Celestron SkyPortal AUX Driver - Device metadata", "[celestron][nexstar-aux][telescope][unit]") {
    auto driver = create_celestron_nexstar_aux_telescope(3, info());
    CHECK(driver->get_device_number() == 3);
    CHECK(driver->get_unique_id() == "Celestron_NexStar_AUX_3");
    CHECK(driver->get_description() == "Celestron CGX SkyPortal AUX mount (lifecycle-only profile)");
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
    CHECK(requests[7].command == 0x24);
    CHECK(requests[7].destination == 0x11);
    CHECK_FALSE(driver->get_tracking());
    CHECK_FALSE(driver->get_slewing());
    CHECK_FALSE(driver->get_is_pulse_guiding());
    CHECK_FALSE(driver->get_at_park());
    error_code([&] { driver->get_right_ascension(); }, AlpacaError::InvalidOperation);
    error_code([&] { driver->get_side_of_pier(); }, AlpacaError::InvalidOperation);
    CHECK_FALSE(driver->get_device_state().empty());
    CHECK(fake.requests().size() == requests.size());
    transition(*driver, clock, false);
    CHECK_FALSE(driver->get_connected());
    CHECK_FALSE(driver->get_device_firmware());
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
    CHECK_FALSE(driver->get_can_find_home());
    CHECK_FALSE(driver->get_can_slew_async());
    CHECK_FALSE(driver->get_can_sync());
    CHECK_FALSE(driver->get_can_pulse_guide());
    CHECK_FALSE(driver->get_can_set_tracking());
    CHECK(driver->get_axis_rate_ranges(0).empty());
    error_code([&] { driver->park(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->unpark(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->set_park(); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->pulse_guide(0, 10); }, AlpacaError::MethodNotImplemented);
    error_code([&] { driver->set_tracking(true); }, AlpacaError::PropertyNotImplemented);
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
    CHECK_FALSE(driver->get_can_slew_async());
    CHECK_FALSE(driver->get_can_set_tracking());
    CHECK_FALSE(driver->get_can_pulse_guide());
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
    auto driver = create_celestron_nexstar_aux_telescope(0, info(fake.port()), clock);
    VirtualDisconnect cleanup{*driver, clock};
    transition(*driver, clock, true);
    REQUIRE(clock.wait_for_wait_count(2, 2s));
    fake.set_muted(true);
    clock.advance(1s);
    REQUIRE(clock.wait_for_wait_count(3, 5s));
    CHECK(driver->get_connected());
    CHECK_FALSE(driver->get_link_fault().empty());
    error_code([&] { driver->get_slewing(); }, AlpacaError::DriverException);
    error_code([&] { driver->get_tracking(); }, AlpacaError::DriverException);
    const auto requests = fake.requests().size();
    clock.advance(30s);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (driver->get_connected() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    CHECK_FALSE(driver->get_connected());
    CHECK(driver->get_device_state().empty());
    CHECK_FALSE(driver->get_device_firmware());
    CHECK_FALSE(driver->get_link_fault().empty());
    CHECK(fake.requests().size() == requests);
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
