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

#include <alpacacore/vendor/celestron/celestron_cgx_protocol.h>
#include <alpacacore/vendor/celestron/celestron_endpoint_claim.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace alpacacore::vendor::celestron {

enum class CgxAxis : std::uint8_t { RightAscension = 0x10, Declination = 0x11 };

/// Direct AUX serial rate: 115200 for CGX/CGX-L, 9600 for the focus motor USB port.
enum class AuxLinkSpeed : std::uint8_t { Baud115200, Baud9600 };

/// Per-driver, serialized AUX link. Serial uses source 0x00; SkyPortal uses 0x20.
class CelestronAuxLink final {
public:
    // Inject TCP sends to exercise partial OS writes without touching a real mount.
    using TcpSender = std::function<std::ptrdiff_t(int, const void*, std::size_t, int)>;
    CelestronAuxLink() = default;
    explicit CelestronAuxLink(TcpSender tcp_sender) : tcp_sender_(std::move(tcp_sender)) {}
    ~CelestronAuxLink();

    CelestronAuxLink(const CelestronAuxLink&) = delete;
    CelestronAuxLink& operator=(const CelestronAuxLink&) = delete;

    void connect_serial(const std::string& port_path, AuxLinkSpeed speed = AuxLinkSpeed::Baud115200);
    // Flush a held serial port without reopening it; TCP requires explicit reconnect.
    void resync();
    // Numeric IPv4/IPv6 endpoints only: DNS must not introduce an unbounded wait.
    void connect_tcp(const std::string& address, int port, std::chrono::milliseconds timeout);
    void disconnect();
    bool is_connected() const;
    bool is_desynchronized() const;
    bool is_healthy() const { return healthy_.load(); }
    std::uint64_t session_generation() const { return session_generation_.load(); }

    // Sends only MC_MOVE_POS(0), even on a poisoned stream. Not proof of stopping.
    void send_stop(CgxAxis axis, std::chrono::milliseconds timeout);
    // Bounded pending reports, consumed by the driver outside the link lock.
    std::vector<AuxFrame> take_controller_reports();

    AuxFrame exchange(const AuxFrame& request, std::chrono::milliseconds timeout);
    std::uint32_t get_axis_position(CgxAxis axis, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    void set_axis_position(CgxAxis axis, std::uint32_t position,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    void goto_axis_fast(CgxAxis axis, std::uint32_t position,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    void goto_axis_slow(CgxAxis axis, std::uint32_t position,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    bool is_axis_slew_done(CgxAxis axis, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    void start_axis_home(CgxAxis axis, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    bool is_axis_home_done(CgxAxis axis, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    void pulse_guide_axis(CgxAxis axis, int velocity_percent, int duration_centiseconds,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
    bool is_aux_guide_active(CgxAxis axis, std::chrono::milliseconds timeout = std::chrono::milliseconds(500));

private:
    AuxFrame exchange_typed(AuxFrame request, std::chrono::milliseconds timeout);
    AuxFrame exchange_impl(AuxFrame request, std::chrono::milliseconds timeout, bool select_source);
    void goto_axis(CgxAxis axis, std::uint32_t position, std::uint8_t command, const char* operation,
                   std::chrono::milliseconds timeout);
    void disconnect_locked();
    void write_frame_locked(const std::vector<std::uint8_t>& bytes, std::chrono::steady_clock::time_point deadline);
    void wait_for_locked(short events, std::chrono::steady_clock::time_point deadline);
    std::vector<std::uint8_t> read_frame_locked(std::chrono::steady_clock::time_point deadline);
    void acknowledge_report_locked(const AuxFrame& report, std::chrono::steady_clock::time_point deadline);

    mutable std::timed_mutex mutex_;
    int serial_fd_ = -1;
    std::atomic<std::uint64_t> session_generation_{0};
    std::atomic<bool> healthy_{false};
    std::atomic<bool> tcp_{false};
    std::uint8_t source_address_ = 0x00;
    bool desynchronized_ = false;
    std::string registered_port_path_;
    CelestronEndpointClaim endpoint_claim_;
    std::vector<std::uint8_t> receive_buffer_;
    std::vector<AuxFrame> controller_reports_;
    TcpSender tcp_sender_;
};

}  // namespace alpacacore::vendor::celestron
