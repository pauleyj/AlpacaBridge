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

#pragma once

#include <alpacacore/telescope_driver.h>
#include <alpacacore/util/task_clock.h>

#include <memory>
#include <optional>
#include <string>

namespace alpacacore::vendor::celestron {

struct NexStarAuxConnectionInfo {
    std::string host;
    int tcp_port = 2000;
    std::chrono::milliseconds response_timeout{5000};
    std::optional<double> site_latitude;
    std::optional<double> site_longitude;
    double site_elevation = 0.0;
    double aperture_diameter = 0.0;
    double focal_length = 0.0;
};

// Construction is hardware-free. This initial profile requires matching four-byte
// motor firmware and model 0x1788 on both axes of a northern CGX (USB identity evidence),
// takes over into a verified stopped state, and serves status telemetry. Sky
// coordinates/motion remain unavailable until the reference/geometry slice;
// SkyPortal identity and takeover still require independent hardware qualification.
std::unique_ptr<TelescopeDriver> create_celestron_nexstar_aux_telescope(
    int device_number, const NexStarAuxConnectionInfo& info, util::TaskClock& clock = util::default_task_clock());

}  // namespace alpacacore::vendor::celestron
