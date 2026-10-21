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

#include <alpacacore/vendor/celestron/celestron_nexstar_aux_telescope_driver.h>

#include <algorithm>
#include <cmath>

#include "catch2_compat.h"
#include "concurrency_stress.h"
#include "fake_celestron_aux_server.h"

namespace {
auto make_driver(const std::shared_ptr<alpacacore::test::FakeCelestronAuxServer>& fake) {
    alpacacore::vendor::celestron::NexStarAuxConnectionInfo info;
    info.host = "127.0.0.1";
    info.tcp_port = fake->port();
    info.site_latitude = 45;
    info.site_longitude = -75;
    return alpacacore::vendor::celestron::create_celestron_nexstar_aux_telescope(0, info);
}
}  // namespace

TEST_CASE("Celestron SkyPortal AUX - connected lifecycle and snapshot stress", "[celestron][telescope][stress]") {
    auto fake = std::make_shared<alpacacore::test::FakeCelestronAuxServer>();
    REQUIRE(fake->ok());
    fake->enable_cgx_profile();
    auto driver = make_driver(fake);
    driver->set_connected(true);
    REQUIRE(driver->get_connected());
    alpacacore::test::StressCallGuard guard{alpacacore::AlpacaError::NotConnected,
                                            alpacacore::AlpacaError::InvalidOperation};
    alpacacore::test::run_lifecycle_stress(*driver, [&](alpacacore::AlpacaDriver& base) {
        auto& telescope = static_cast<alpacacore::TelescopeDriver&>(base);
        guard([&] { telescope.set_target_right_ascension(6); });
        guard([&] { telescope.set_target_declination(20); });
        guard([&] { telescope.find_home(); });
        guard([&] { telescope.set_tracking(true); });
        guard([&] { telescope.pulse_guide(0, 100); });
        guard([&] { static_cast<void>(telescope.get_is_pulse_guiding()); });
        guard([&] { telescope.set_tracking_rate(0); });
        guard([&] { telescope.set_tracking(false); });
        guard([&] { telescope.abort_slew(); });
        guard([&] { telescope.sync_to_coordinates(6, 20); });
        guard([&] { telescope.sync_to_target(); });
        guard([&] { static_cast<void>(telescope.get_at_home()); });
        guard([&] { static_cast<void>(telescope.get_right_ascension()); });
        guard([&] { static_cast<void>(telescope.get_declination()); });
        guard([&] { static_cast<void>(telescope.get_altitude()); });
        guard([&] { static_cast<void>(telescope.get_azimuth()); });
        guard([&] { static_cast<void>(telescope.get_side_of_pier()); });
        guard([&] { static_cast<void>(telescope.get_target_right_ascension()); });
        guard([&] { static_cast<void>(telescope.get_slewing()); });
        guard([&] { static_cast<void>(telescope.get_tracking()); });
        guard([&] { static_cast<void>(telescope.get_device_state()); });
        guard([&] { static_cast<void>(telescope.get_device_firmware()); });
    });
    CHECK(alpacacore::test::settle_connected(*driver, false));
    INFO(guard.report());
    CHECK(guard.unexpected_count() == 0);
    CHECK(guard.total_calls() > 0);
}

TEST_CASE("Celestron SkyPortal AUX - destruction joins an in-flight takeover", "[celestron][telescope][stress]") {
    auto fake = std::make_shared<alpacacore::test::FakeCelestronAuxServer>();
    REQUIRE(fake->ok());
    fake->enable_cgx_profile();
    alpacacore::test::run_destruction_during_connect_stress([fake] { return make_driver(fake); }, 10);
}

TEST_CASE("Celestron SkyPortal AUX - active GOTO lifecycle ownership stress", "[celestron][telescope][stress]") {
    using namespace std::chrono_literals;
    bool via_home = false;
    SECTION("Same-branch GOTO") {}
    SECTION("Meridian home route") { via_home = true; }
    auto fake = std::make_shared<alpacacore::test::FakeCelestronAuxServer>();
    REQUIRE(fake->ok());
    fake->enable_cgx_profile();
    fake->set_goto_stuck(true);
    auto driver = make_driver(fake);
    driver->set_connected(true);
    driver->find_home();
    const auto home_deadline = std::chrono::steady_clock::now() + 5s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < home_deadline) std::this_thread::sleep_for(10ms);
    REQUIRE(driver->get_at_home());
    driver->set_tracking(true);
    if (via_home) {
        fake->set_home_polls(0, -1);
        fake->set_home_polls(1, -1);
    }
    driver->slew_to_coordinates_async(std::fmod(driver->get_right_ascension() + (via_home ? 12.0 : 0.0), 24.0), 88.5);
    const auto dispatch_deadline = std::chrono::steady_clock::now() + 2s;
    bool dispatched = false;
    while (std::chrono::steady_clock::now() < dispatch_deadline) {
        const auto requests = fake->requests();
        dispatched = std::count_if(requests.begin(), requests.end(), [via_home](const auto& frame) {
                         return frame.command == (via_home ? 0x0b : 0x02);
                     }) == (via_home ? 4 : 2);
        if (dispatched) break;
        std::this_thread::sleep_for(10ms);
    }
    REQUIRE(dispatched);
    alpacacore::test::StressCallGuard guard{alpacacore::AlpacaError::NotConnected,
                                            alpacacore::AlpacaError::InvalidOperation};
    alpacacore::test::run_lifecycle_stress(*driver, [&](alpacacore::AlpacaDriver& base) {
        auto& telescope = static_cast<alpacacore::TelescopeDriver&>(base);
        guard([&] { telescope.abort_slew(); });
        guard([&] { telescope.slew_to_coordinates_async(6, 20); });
        guard([&] { telescope.set_tracking(false); });
        guard([&] { telescope.sync_to_coordinates(6, 20); });
        guard([&] { static_cast<void>(telescope.get_slewing()); });
        guard([&] { static_cast<void>(telescope.get_tracking()); });
    });
    CHECK(alpacacore::test::settle_connected(*driver, false));
    INFO(guard.report());
    CHECK(guard.unexpected_count() == 0);
    CHECK(guard.total_calls() > 0);
}

TEST_CASE("Celestron SkyPortal AUX - active guide lifecycle ownership stress", "[celestron][telescope][stress]") {
    using namespace std::chrono_literals;
    auto fake = std::make_shared<alpacacore::test::FakeCelestronAuxServer>();
    fake->enable_cgx_profile();
    auto driver = make_driver(fake);
    driver->set_connected(true);
    driver->find_home();
    const auto home_deadline = std::chrono::steady_clock::now() + 5s;
    while (driver->get_slewing() && std::chrono::steady_clock::now() < home_deadline) std::this_thread::sleep_for(10ms);
    REQUIRE(driver->get_at_home());
    driver->set_tracking(true);
    driver->pulse_guide(1, 10000);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    bool sent = false;
    while (!sent && std::chrono::steady_clock::now() < deadline) {
        const auto frames = fake->requests();
        sent = std::any_of(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; });
        std::this_thread::sleep_for(10ms);
    }
    REQUIRE(sent);
    alpacacore::test::StressCallGuard guard{alpacacore::AlpacaError::NotConnected,
                                            alpacacore::AlpacaError::InvalidOperation};
    alpacacore::test::run_lifecycle_stress(*driver, [&](alpacacore::AlpacaDriver& base) {
        auto& telescope = static_cast<alpacacore::TelescopeDriver&>(base);
        guard([&] { telescope.pulse_guide(0, 100); });
        guard([&] { telescope.abort_slew(); });
        guard([&] { static_cast<void>(telescope.get_is_pulse_guiding()); });
        guard([&] { static_cast<void>(telescope.get_slewing()); });
    });
    CHECK(alpacacore::test::settle_connected(*driver, false));
    INFO(guard.report());
    CHECK(guard.unexpected_count() == 0);
    CHECK(guard.total_calls() > 0);
    const auto frames = fake->requests();
    CHECK(std::count_if(frames.begin(), frames.end(), [](const auto& f) { return f.command == 0x26; }) == 1);
}
