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
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace alpacacore::test {

// Transport fake only: no motor dynamics, calibration, or firmware qualification.
// CGX identity replies mirror docs/celestron-cgx-usb-identity.md, not SkyPortal rig evidence.
// Unknown commands are silent. Every socket wait/send and teardown is bounded.
class FakeCelestronAuxServer final {
public:
    using Frame = vendor::celestron::AuxFrame;
    FakeCelestronAuxServer() {
        listener_ = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (listener_ < 0) return;
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
            ::listen(listener_, 4) != 0)
            return;
        socklen_t size = sizeof(address);
        if (::getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &size) != 0) return;
        port_ = ntohs(address.sin_port);
        worker_ = std::thread([this] { run(); });
    }
    ~FakeCelestronAuxServer() {
        stop_ = true;
        cv_.notify_all();
        if (listener_ >= 0) ::shutdown(listener_, SHUT_RDWR);
        if (worker_.joinable()) worker_.join();
        if (listener_ >= 0) ::close(listener_);
    }
    FakeCelestronAuxServer(const FakeCelestronAuxServer&) = delete;
    FakeCelestronAuxServer& operator=(const FakeCelestronAuxServer&) = delete;
    int port() const { return port_; }
    bool ok() const { return port_ > 0; }
    void set_muted(bool value) { muted_ = value; }
    void corrupt_next_reply() { corrupt_ = true; }
    void report_before_reply() { report_ = true; }
    void report_before_reply(const Frame& report) {
        std::lock_guard<std::mutex> lock(mutex_);
        before_reply_ = vendor::celestron::encode_aux_frame(report);
    }
    void report_after_reply(const Frame& report) {
        std::lock_guard<std::mutex> lock(mutex_);
        after_reply_ = vendor::celestron::encode_aux_frame(report);
    }
    void truncate_next_reply() { truncate_ = true; }
    void close_after_reply() { close_after_ = true; }
    void fragment_next_reply() { fragment_ = true; }
    void unrelated_until_deadline() { noise_ = true; }
    void close_next_connection() { close_next_ = true; }
    void enable_cgx_profile() { cgx_ = true; }
    void set_model(std::uint16_t value) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& model : models_) model = {static_cast<std::uint8_t>(value >> 8), static_cast<std::uint8_t>(value)};
    }
    void set_model(vendor::celestron::CgxAxis axis, std::vector<std::uint8_t> reply) {
        std::lock_guard<std::mutex> lock(mutex_);
        models_[axis == vendor::celestron::CgxAxis::Declination ? 1 : 0] = std::move(reply);
    }
    void set_firmware(vendor::celestron::CgxAxis axis, std::vector<std::uint8_t> reply) {
        std::lock_guard<std::mutex> lock(mutex_);
        firmware_[axis == vendor::celestron::CgxAxis::Declination ? 1 : 0] = std::move(reply);
    }
    void set_moving(bool value) {
        for (auto& moving : moving_) moving = value;
    }
    void ignore_stops(bool value) { ignore_stops_ = value; }
    void lose_stop_reply(std::uint8_t axis) { lose_stop_axis_ = axis; }
    bool wait_for_fragment() {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::seconds(2), [this] { return held_; });
    }
    void release_fragment() {
        std::lock_guard<std::mutex> lock(mutex_);
        released_ = true;
        cv_.notify_all();
    }
    std::vector<Frame> requests() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return requests_;
    }
    bool wait_for_requests(std::size_t count) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::seconds(2), [&] { return requests_.size() >= count; });
    }

private:
    bool send_bytes(int fd, const std::vector<std::uint8_t>& bytes) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        std::size_t sent = 0;
        while (!stop_ && sent < bytes.size() && std::chrono::steady_clock::now() < deadline) {
            pollfd descriptor{fd, POLLOUT, 0};
            if (::poll(&descriptor, 1, 10) <= 0) continue;
            const auto n = ::send(fd, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (n > 0)
                sent += static_cast<std::size_t>(n);
            else if (n == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
                return false;
        }
        return sent == bytes.size();
    }
    void serve(int fd) {
        std::vector<std::uint8_t> pending;
        while (!stop_) {
            pollfd descriptor{fd, POLLIN, 0};
            if (::poll(&descriptor, 1, 10) <= 0) continue;
            std::uint8_t bytes[128]{};
            const auto n = ::recv(fd, bytes, sizeof(bytes), MSG_DONTWAIT);
            if (n <= 0) return;
            pending.insert(pending.end(), bytes, bytes + n);
            while (pending.size() >= 2 && pending.size() >= static_cast<std::size_t>(pending[1]) + 3) {
                const auto size = static_cast<std::size_t>(pending[1]) + 3;
                Frame request;
                try {
                    request = vendor::celestron::decode_aux_frame(std::span(pending.data(), size));
                } catch (...) {
                    return;
                }
                pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(size));
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    requests_.push_back(request);
                    cv_.notify_all();
                }
                if (noise_) {
                    const auto unrelated = vendor::celestron::encode_aux_frame(Frame{0x11, 0x20, 0x01, {0, 0, 0}});
                    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
                    while (!stop_ && std::chrono::steady_clock::now() < deadline) {
                        if (!send_bytes(fd, unrelated)) return;
                    }
                    continue;
                }
                if (muted_ || request.command == 0x50 || request.command == 0x51) continue;
                const std::size_t axis = request.destination == 0x11 ? 1 : 0;
                std::vector<std::uint8_t> payload;
                if (request.command == 0xfe || (cgx_ && request.command == 0x05)) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    payload = request.command == 0x05 ? models_[axis]
                                                      : (cgx_ ? firmware_[axis] : std::vector<std::uint8_t>{7, 18});
                } else if (cgx_ && request.command == 0xfc)
                    payload = {0};
                else if (cgx_ && request.command == 0x13)
                    payload = {static_cast<std::uint8_t>(moving_[axis] ? 0 : 0xff)};
                else if (cgx_ && request.command == 0x27)
                    payload = {0};
                else if (request.command == 0x01) {
                    if (cgx_ && moving_[axis]) position_[axis] += 100;
                    const auto position = position_[axis].load();
                    payload = {static_cast<std::uint8_t>(position >> 16), static_cast<std::uint8_t>(position >> 8),
                               static_cast<std::uint8_t>(position)};
                } else if (request.command != 0x24)
                    continue;
                if (cgx_ && request.command == 0x24) {
                    if (!ignore_stops_) moving_[axis] = false;
                    if (lose_stop_axis_ == request.destination) continue;
                }
                auto reply = vendor::celestron::encode_aux_frame(
                    Frame{request.destination, request.source, request.command, payload});
                if (corrupt_.exchange(false)) reply.back() ^= 1;
                if (report_.exchange(false)) {
                    auto warning = vendor::celestron::encode_aux_frame(Frame{0x10, request.source, 0x50, {1}});
                    warning.insert(warning.end(), reply.begin(), reply.end());
                    reply = std::move(warning);
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    reply.insert(reply.begin(), before_reply_.begin(), before_reply_.end());
                    reply.insert(reply.end(), after_reply_.begin(), after_reply_.end());
                    before_reply_.clear();
                    after_reply_.clear();
                }
                const bool truncated = truncate_.exchange(false);
                if (truncated) reply.resize(2);
                if (fragment_.exchange(false)) {
                    if (!send_bytes(fd, {reply[0], reply[1]})) return;
                    std::unique_lock<std::mutex> lock(mutex_);
                    held_ = true;
                    cv_.notify_all();
                    cv_.wait(lock, [this] { return released_ || stop_; });
                    if (stop_) return;
                    reply.erase(reply.begin(), reply.begin() + 2);
                }
                if (!send_bytes(fd, reply)) return;
                const bool close_after = close_after_.exchange(false);
                if (truncated || close_after) return;
            }
        }
    }
    void run() {
        while (!stop_) {
            pollfd descriptor{listener_, POLLIN, 0};
            if (::poll(&descriptor, 1, 10) <= 0) continue;
            const int fd = ::accept4(listener_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (fd < 0) continue;
            if (!close_next_.exchange(false)) serve(fd);
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
    }
    int listener_ = -1;
    int port_ = 0;
    std::thread worker_;
    std::atomic<bool> stop_{false}, muted_{false}, corrupt_{false}, report_{false}, fragment_{false}, noise_{false},
        close_next_{false};
    std::atomic<bool> truncate_{false}, close_after_{false};
    std::atomic<bool> cgx_{false}, ignore_stops_{false};
    std::array<std::atomic<bool>, 2> moving_{false, false};
    std::atomic<std::uint8_t> lose_stop_axis_{0};
    std::array<std::vector<std::uint8_t>, 2> models_{{{0x17, 0x88}, {0x17, 0x88}}};
    std::array<std::vector<std::uint8_t>, 2> firmware_{{{7, 17, 0, 25}, {7, 17, 0, 25}}};
    std::array<std::atomic<std::uint32_t>, 2> position_{0x123456, 0x123456};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool held_ = false, released_ = false;
    std::vector<std::uint8_t> before_reply_, after_reply_;
    std::vector<Frame> requests_;
};

}  // namespace alpacacore::test
