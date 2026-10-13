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
