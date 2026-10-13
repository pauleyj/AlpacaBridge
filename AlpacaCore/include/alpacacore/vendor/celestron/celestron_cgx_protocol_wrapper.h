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

#include <alpacacore/vendor/celestron/celestron_aux_link.h>

namespace alpacacore::vendor::celestron {

// Keep the direct-USB backend's source interface while sharing AUX framing/I/O.
using CelestronCgxProtocol = CelestronAuxLink;

}  // namespace alpacacore::vendor::celestron
