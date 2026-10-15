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

namespace alpacacore::vendor::celestron {

namespace {

std::uint8_t axis_address(CgxAxis axis) {
    switch (axis) {
        case CgxAxis::RightAscension:
        case CgxAxis::Declination:
            return static_cast<std::uint8_t>(axis);
    }
    throw AlpacaException("Celestron AUX axis is invalid", AlpacaError::InvalidValue);
}

void require_empty_ack(const AuxFrame& response, const char* operation) {
    if (!response.payload.empty()) {
        throw AlpacaException(std::string(operation) + " returned an unexpected payload", AlpacaError::DriverException);
    }
}

bool decode_axis_completion(const AuxFrame& response, const char* operation) {
    if (response.payload.size() != 1 || (response.payload[0] != 0x00 && response.payload[0] != 0xff)) {
        throw AlpacaException(std::string(operation) + " returned an invalid completion status",
                              AlpacaError::DriverException);
    }
    return response.payload[0] == 0xff;
}

}  // namespace

std::uint32_t CelestronAuxLink::get_axis_position(CgxAxis axis, std::chrono::milliseconds timeout) {
    const AuxFrame response =
        exchange_typed(AuxFrame{.destination = axis_address(axis), .command = 0x01, .payload = {}}, timeout);
    if (response.payload.size() != 3) {
        throw AlpacaException("MC_GET_POSITION returned a payload that is not 24 bits", AlpacaError::DriverException);
    }
    return (static_cast<std::uint32_t>(response.payload[0]) << 16) |
           (static_cast<std::uint32_t>(response.payload[1]) << 8) | static_cast<std::uint32_t>(response.payload[2]);
}

void CelestronAuxLink::set_axis_position(CgxAxis axis, std::uint32_t position, std::chrono::milliseconds timeout) {
    if (position >= (1U << 24)) {
        throw AlpacaException("Celestron AUX encoder position must fit in 24 bits", AlpacaError::InvalidValue);
    }
    const AuxFrame request{.destination = axis_address(axis),
                           .command = 0x04,
                           .payload = {static_cast<std::uint8_t>(position >> 16),
                                       static_cast<std::uint8_t>(position >> 8), static_cast<std::uint8_t>(position)}};
    require_empty_ack(exchange_typed(request, timeout), "MC_SET_POSITION");
}

void CelestronAuxLink::goto_axis_fast(CgxAxis axis, std::uint32_t position, std::chrono::milliseconds timeout) {
    goto_axis(axis, position, 0x02, "MC_GOTO_FAST", timeout);
}

void CelestronAuxLink::goto_axis_slow(CgxAxis axis, std::uint32_t position, std::chrono::milliseconds timeout) {
    goto_axis(axis, position, 0x17, "MC_GOTO_SLOW", timeout);
}

void CelestronAuxLink::goto_axis(CgxAxis axis, std::uint32_t position, std::uint8_t command, const char* operation,
                                 std::chrono::milliseconds timeout) {
    if (position >= (1U << 24)) {
        throw AlpacaException("Celestron AUX GOTO position must fit in 24 bits", AlpacaError::InvalidValue);
    }
    const AuxFrame request{.destination = axis_address(axis),
                           .command = command,
                           .payload = {static_cast<std::uint8_t>(position >> 16),
                                       static_cast<std::uint8_t>(position >> 8), static_cast<std::uint8_t>(position)}};
    require_empty_ack(exchange_typed(request, timeout), operation);
}

bool CelestronAuxLink::is_axis_slew_done(CgxAxis axis, std::chrono::milliseconds timeout) {
    return decode_axis_completion(
        exchange_typed(AuxFrame{.destination = axis_address(axis), .command = 0x13, .payload = {}}, timeout),
        "MC_SLEW_DONE");
}

void CelestronAuxLink::start_axis_home(CgxAxis axis, std::chrono::milliseconds timeout) {
    require_empty_ack(
        exchange_typed(AuxFrame{.destination = axis_address(axis), .command = 0x0b, .payload = {}}, timeout),
        "MC_LEVEL_START");
}

bool CelestronAuxLink::is_axis_home_done(CgxAxis axis, std::chrono::milliseconds timeout) {
    return decode_axis_completion(
        exchange_typed(AuxFrame{.destination = axis_address(axis), .command = 0x12, .payload = {}}, timeout),
        "MC_LEVEL_DONE");
}

void CelestronAuxLink::pulse_guide_axis(CgxAxis axis, int velocity_percent, int duration_centiseconds,
                                        std::chrono::milliseconds timeout) {
    if (velocity_percent < -100 || velocity_percent > 100 || duration_centiseconds < 1 || duration_centiseconds > 255) {
        throw AlpacaException("Celestron AUX pulse guide rate or duration is out of range", AlpacaError::InvalidValue);
    }
    const auto velocity = static_cast<std::uint8_t>(static_cast<std::int8_t>(velocity_percent));
    const AuxFrame request{.destination = axis_address(axis),
                           .command = 0x26,
                           .payload = {velocity, static_cast<std::uint8_t>(duration_centiseconds)}};
    require_empty_ack(exchange_typed(request, timeout), "MC_AUX_GUIDE");
}

bool CelestronAuxLink::is_aux_guide_active(CgxAxis axis, std::chrono::milliseconds timeout) {
    const AuxFrame response =
        exchange_typed(AuxFrame{.destination = axis_address(axis), .command = 0x27, .payload = {}}, timeout);
    if (response.payload.size() != 1 || response.payload[0] > 1) {
        throw AlpacaException("MC_AUX_GUIDE_ACTIVE returned an invalid status", AlpacaError::DriverException);
    }
    return response.payload[0] == 1;
}

}  // namespace alpacacore::vendor::celestron
