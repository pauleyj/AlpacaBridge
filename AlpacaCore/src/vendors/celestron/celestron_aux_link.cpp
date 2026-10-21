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

#include <alpacacore/util/error_handling.h>
#include <alpacacore/util/logging.h>
#include <alpacacore/util/serial_io.h>
#include <alpacacore/util/serial_port_registry.h>
#include <alpacacore/vendor/celestron/celestron_aux_link.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <filesystem>
#include <string>
#include <utility>

namespace alpacacore::vendor::celestron {

namespace {

constexpr std::uint8_t kAuxStart = 0x3b;
constexpr std::size_t kMaximumFrameSize = 32;
constexpr auto kMaximumExchangeTimeout = std::chrono::seconds(5);

void validate_timeout(std::chrono::milliseconds timeout, bool tcp) {
    if (timeout <= std::chrono::milliseconds::zero() || timeout > kMaximumExchangeTimeout) {
        throw AlpacaException(tcp ? "Celestron AUX timeout must be between 1 and 5000 ms"
                                  : "Celestron CGX AUX timeout must be between 1 and 5000 ms",
                              AlpacaError::InvalidValue);
    }
}

[[noreturn]] void throw_link_error(const std::string& message, bool tcp = false) {
    throw AlpacaException((tcp ? "Celestron AUX: " : "Celestron CGX USB: ") + message, AlpacaError::DriverException);
}

std::uint8_t axis_address(CgxAxis axis) {
    switch (axis) {
        case CgxAxis::RightAscension:
        case CgxAxis::Declination:
            return static_cast<std::uint8_t>(axis);
    }
    throw AlpacaException("Celestron CGX AUX axis is invalid", AlpacaError::InvalidValue);
}

int poll_timeout_ms(std::chrono::steady_clock::time_point deadline) {
    const auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining <= std::chrono::steady_clock::duration::zero()) {
        return 0;
    }
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
    return static_cast<int>(std::max<std::int64_t>(1, milliseconds.count()));
}

}  // namespace

CelestronAuxLink::~CelestronAuxLink() { disconnect(); }

void CelestronAuxLink::connect_serial(const std::string& port_path, AuxLinkSpeed speed) {
    if (port_path.empty()) {
        throw_link_error("a serial port path is required");
    }

    std::lock_guard<std::timed_mutex> lock(mutex_);
    std::error_code path_error;
    const std::filesystem::path canonical = std::filesystem::canonical(port_path, path_error);
    const std::string registry_path = path_error ? port_path : canonical.string();
    struct stat held{}, current{};
    if (!tcp_ && serial_fd_ >= 0 && registry_path == registered_port_path_ && ::fstat(serial_fd_, &held) == 0 &&
        ::stat(port_path.c_str(), &current) == 0 && held.st_nlink != 0 && held.st_dev == current.st_dev &&
        held.st_ino == current.st_ino && held.st_rdev == current.st_rdev) {
        if (desynchronized_) {
            // Preserve exclusive USB ownership; the fault already invalidated references.
            if (::tcflush(serial_fd_, TCIFLUSH) != 0) {
                disconnect_locked();
                throw_link_error("cannot clear stale AUX input before recovery");
            }
            receive_buffer_.clear();
            controller_reports_.clear();
            desynchronized_ = false;
            healthy_.store(true);
        }
        return;
    }
    disconnect_locked();
    if (!util::try_mark_serial_port_open(registry_path)) {
        throw AlpacaException("Celestron CGX USB port is already in use: " + port_path, AlpacaError::InvalidOperation);
    }

    const int fd = ::open(port_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        util::mark_serial_port_closed(registry_path);
        throw_link_error("cannot open " + port_path + ": " + util::errno_string(errno));
    }

    bool exclusive = false;
    try {
        if (::ioctl(fd, TIOCEXCL) != 0)
            throw_link_error("cannot claim exclusive serial access: " + util::errno_string(errno));
        exclusive = true;
        struct termios settings{};
        if (::tcgetattr(fd, &settings) != 0)
            throw_link_error("cannot read serial settings: " + util::errno_string(errno));
        ::cfmakeraw(&settings);
        settings.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
        settings.c_cflag |= CS8 | CREAD | CLOCAL;
        settings.c_cc[VMIN] = 0;
        settings.c_cc[VTIME] = 0;
        const speed_t baud = speed == AuxLinkSpeed::Baud9600 ? B9600 : B115200;
        if (::cfsetispeed(&settings, baud) != 0 || ::cfsetospeed(&settings, baud) != 0 ||
            ::tcsetattr(fd, TCSANOW, &settings) != 0)
            throw_link_error(std::string("cannot configure ") + (speed == AuxLinkSpeed::Baud9600 ? "9600" : "115200") +
                             "-baud serial link: " + util::errno_string(errno));
        if (!util::set_nonblocking(fd))
            throw_link_error("cannot enable bounded serial I/O: " + util::errno_string(errno));
        if (::tcflush(fd, TCIFLUSH) != 0)
            throw_link_error("cannot clear stale AUX input before reconnect: " + util::errno_string(errno));
        registered_port_path_ = registry_path;
        receive_buffer_.clear();
        serial_fd_ = fd;
        ++session_generation_;
        healthy_.store(true);
    } catch (...) {
        if (exclusive) (void)::ioctl(fd, TIOCNXCL);
        ::close(fd);
        util::mark_serial_port_closed(registry_path);
        registered_port_path_.clear();
        throw;
    }
}

void CelestronAuxLink::resync() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (serial_fd_ < 0) {
        throw AlpacaException("Celestron AUX link is not connected", AlpacaError::NotConnected);
    }
    if (tcp_) {
        throw AlpacaException("Celestron AUX TCP requires an explicit reconnect", AlpacaError::InvalidOperation);
    }
    if (!desynchronized_) return;
    if (::tcflush(serial_fd_, TCIFLUSH) != 0) {
        disconnect_locked();
        throw_link_error("cannot clear stale AUX input before recovery");
    }
    receive_buffer_.clear();
    controller_reports_.clear();
    desynchronized_ = false;
    healthy_.store(true);
}

void CelestronAuxLink::connect_tcp(const std::string& address, int port, std::chrono::milliseconds timeout) {
    validate_timeout(timeout, true);
    if (port < 1 || port > 65535 || address.empty() || address.size() > INET6_ADDRSTRLEN ||
        address.find('\0') != std::string::npos) {
        throw AlpacaException("Celestron AUX requires a numeric IP address and a port between 1 and 65535",
                              AlpacaError::InvalidValue);
    }
    sockaddr_storage storage{};
    auto* ipv4 = reinterpret_cast<sockaddr_in*>(&storage);
    auto* ipv6 = reinterpret_cast<sockaddr_in6*>(&storage);
    socklen_t size;
    if (::inet_pton(AF_INET, address.c_str(), &ipv4->sin_addr) == 1) {
        ipv4->sin_family = AF_INET;
        ipv4->sin_port = htons(static_cast<std::uint16_t>(port));
        size = sizeof(sockaddr_in);
    } else if (::inet_pton(AF_INET6, address.c_str(), &ipv6->sin6_addr) == 1) {
        ipv6->sin6_family = AF_INET6;
        ipv6->sin6_port = htons(static_cast<std::uint16_t>(port));
        size = sizeof(sockaddr_in6);
    } else {
        throw AlpacaException("Celestron AUX host must be a numeric IPv4 or IPv6 address", AlpacaError::InvalidValue);
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    // Relative waits avoid GCC 14's TSan/steady-clock try_lock_until compile defect.
    if (!lock.try_lock_for(deadline - std::chrono::steady_clock::now()) || std::chrono::steady_clock::now() >= deadline)
        throw_link_error("TCP connect timed out waiting for AUX I/O", true);
    disconnect_locked();
    endpoint_claim_.claim(reinterpret_cast<const sockaddr*>(&storage), port);
    try {
        serial_fd_ = ::socket(storage.ss_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (serial_fd_ < 0) throw_link_error("cannot open TCP socket: " + util::errno_string(errno), true);
        tcp_ = true;
        source_address_ = 0x20;
        const int result = ::connect(serial_fd_, reinterpret_cast<const sockaddr*>(&storage), size);
        if (result < 0) {
            if (errno != EINPROGRESS) throw_link_error("TCP connect failed: " + util::errno_string(errno), true);
            wait_for_locked(POLLOUT, deadline);
        }
        int error = 0;
        socklen_t error_size = sizeof(error);
        if (::getsockopt(serial_fd_, SOL_SOCKET, SO_ERROR, &error, &error_size) != 0 || error != 0) {
            throw_link_error("TCP connect failed: " + util::errno_string(error != 0 ? error : errno), true);
        }
        ++session_generation_;
        healthy_.store(true);
    } catch (...) {
        disconnect_locked();
        throw;
    }
}

bool CelestronAuxLink::is_desynchronized() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return desynchronized_;
}

void CelestronAuxLink::send_stop(CgxAxis axis, std::chrono::milliseconds timeout) {
    validate_timeout(timeout, tcp_.load());
    const auto destination = axis_address(axis);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    if (tcp_.load()) {
        if (!lock.try_lock_for(deadline - std::chrono::steady_clock::now()) ||
            std::chrono::steady_clock::now() >= deadline)
            throw_link_error("TCP stop timed out waiting for AUX I/O", true);
    } else {
        lock.lock();
        deadline = std::chrono::steady_clock::now() + timeout;
    }
    if (serial_fd_ < 0) throw AlpacaException("Celestron AUX is not connected", AlpacaError::NotConnected);
    // Replies to a send-only stop may arrive later. No subsequent normal
    // exchange may treat one as acknowledgement of a different stop.
    desynchronized_ = true;
    ++session_generation_;
    healthy_.store(false);
    write_frame_locked(encode_aux_frame(AuxFrame{source_address_, destination, 0x24, {0}}), deadline);
    // SkyPortal motion stops can leave sidereal tracking running; share the same send deadline.
    if (tcp_) write_frame_locked(encode_aux_frame(AuxFrame{source_address_, destination, 0x06, {0, 0, 0}}), deadline);
}

std::vector<AuxFrame> CelestronAuxLink::take_controller_reports() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    std::vector<AuxFrame> result;
    result.swap(controller_reports_);
    return result;
}

void CelestronAuxLink::acknowledge_report_locked(const AuxFrame& report,
                                                 std::chrono::steady_clock::time_point deadline) {
    if (report.payload.size() != 1) throw_link_error("invalid controller report payload", true);
    write_frame_locked(encode_aux_frame(AuxFrame{report.destination, report.source, report.command, {}}), deadline);
    // Losing a motion report is unsafe: fail rather than silently dropping it.
    if (controller_reports_.size() >= 16) throw_link_error("controller report queue overflow", true);
    controller_reports_.push_back(report);
}

void CelestronAuxLink::disconnect() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    disconnect_locked();
}

bool CelestronAuxLink::is_connected() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return serial_fd_ >= 0;
}

AuxFrame CelestronAuxLink::exchange(const AuxFrame& request, std::chrono::milliseconds timeout) {
    return exchange_impl(request, timeout, false);
}

AuxFrame CelestronAuxLink::exchange_typed(AuxFrame request, std::chrono::milliseconds timeout) {
    return exchange_impl(std::move(request), timeout, true);
}

AuxFrame CelestronAuxLink::exchange_impl(AuxFrame request, std::chrono::milliseconds timeout, bool select_source) {
    validate_timeout(timeout, tcp_.load());

    auto deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    if (tcp_.load()) {
        if (!lock.try_lock_for(deadline - std::chrono::steady_clock::now()) ||
            std::chrono::steady_clock::now() >= deadline)
            throw_link_error("TCP exchange timed out waiting for AUX I/O", true);
    } else {
        lock.lock();
        deadline = std::chrono::steady_clock::now() + timeout;
    }
    if (serial_fd_ < 0) {
        throw AlpacaException("Celestron AUX is not connected", AlpacaError::NotConnected);
    }
    if (desynchronized_) {
        throw_link_error("AUX response timed out previously; reconnect before issuing another command", tcp_);
    }

    if (select_source) request.source = source_address_;

    const auto started = std::chrono::steady_clock::now();
    const auto log_failure = [&](std::string_view reason) noexcept {
        try {
            const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started);
            ALPACA_LOG_ERROR("Celestron CGX", "AUX transaction failed: source=" + std::to_string(request.source) +
                                                  " destination=" + std::to_string(request.destination) +
                                                  " command=" + std::to_string(request.command) +
                                                  " elapsed_ms=" + std::to_string(elapsed.count()) + " timeout_ms=" +
                                                  std::to_string(timeout.count()) + " error=" + std::string(reason));
        } catch (...) {
            // Diagnostics must never replace the original transaction failure.
            return;
        }
    };
    std::vector<std::uint8_t> request_bytes;
    try {
        request_bytes = encode_aux_frame(request);
    } catch (const std::invalid_argument& error) {
        throw_link_error(std::string("invalid AUX request: ") + error.what(), tcp_);
    }
    bool request_started = false;
    try {
        request_started = true;
        write_frame_locked(request_bytes, deadline);

        while (true) {
            AuxFrame response;
            response = decode_aux_frame(read_frame_locked(deadline));
            if (tcp_ && (response.command == 0x50 || response.command == 0x51) &&
                (response.source == 0x10 || response.source == 0x11) &&
                (response.destination == source_address_ || response.destination == 0x00)) {
                acknowledge_report_locked(response, deadline);
                continue;
            }
            if (response.source == request.destination && response.destination == request.source &&
                response.command == request.command) {
                return response;
            }
            // AUX carries no transaction id. Discard traffic for another address or
            // command while preserving the one deadline for this exchange.
        }
    } catch (const std::invalid_argument& error) {
        if (request_started) {
            log_failure(error.what());
            desynchronized_ = true;
            ++session_generation_;
            healthy_.store(false);
        }
        throw_link_error(std::string("invalid AUX response: ") + error.what(), tcp_);
    } catch (...) {
        if (request_started) {
            try {
                throw;
            } catch (const std::exception& error) {
                log_failure(error.what());
            } catch (...) {
                log_failure("non-standard exception");
            }
            desynchronized_ = true;
            ++session_generation_;
            healthy_.store(false);
        }
        throw;
    }
}

void CelestronAuxLink::disconnect_locked() {
    healthy_.store(false);
    if (serial_fd_ >= 0) {
        ++session_generation_;
        if (!tcp_) (void)::ioctl(serial_fd_, TIOCNXCL);
        ::close(serial_fd_);
        serial_fd_ = -1;
    }
    util::mark_serial_port_closed(registered_port_path_);
    registered_port_path_.clear();
    endpoint_claim_.release();
    tcp_ = false;
    source_address_ = 0x00;
    receive_buffer_.clear();
    controller_reports_.clear();
    desynchronized_ = false;
}

void CelestronAuxLink::wait_for_locked(short events, std::chrono::steady_clock::time_point deadline) {
    const bool tcp = tcp_;
    while (true) {
        if (std::chrono::steady_clock::now() >= deadline) {
            throw_link_error("AUX transaction timed out", tcp);
        }
        struct pollfd descriptor{};
        descriptor.fd = serial_fd_;
        descriptor.events = events;
        const int result = ::poll(&descriptor, 1, poll_timeout_ms(deadline));
        if (result > 0) {
            if (std::chrono::steady_clock::now() >= deadline) {
                throw_link_error("AUX transaction timed out", tcp);
            }
            // TCP may have a final readable reply together with FIN; serial loss is terminal.
            if (tcp_ && (descriptor.revents & events) != 0) return;
            if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                disconnect_locked();
                throw_link_error(
                    tcp ? "AUX transport became unavailable" : "serial device was removed or became unavailable", tcp);
            }
            if ((descriptor.revents & events) != 0) return;
            continue;
        }
        if (result == 0) {
            if (std::chrono::steady_clock::now() >= deadline) {
                throw_link_error("AUX transaction timed out", tcp);
            }
            continue;
        }
        if (errno != EINTR) {
            throw_link_error("serial poll failed: " + util::errno_string(errno), tcp);
        }
    }
}

void CelestronAuxLink::write_frame_locked(const std::vector<std::uint8_t>& bytes,
                                          std::chrono::steady_clock::time_point deadline) {
    const bool tcp = tcp_;
    std::size_t written = 0;
    try {
        while (written < bytes.size()) {
            wait_for_locked(POLLOUT, deadline);
            const auto count =
                tcp_ ? (tcp_sender_
                            ? tcp_sender_(serial_fd_, bytes.data() + written, bytes.size() - written, MSG_NOSIGNAL)
                            : ::send(serial_fd_, bytes.data() + written, bytes.size() - written, MSG_NOSIGNAL))
                     : ::write(serial_fd_, bytes.data() + written, bytes.size() - written);
            if (count > 0) {
                written += static_cast<std::size_t>(count);
            } else if (count == 0 || (count < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)) {
                const int error = errno;
                disconnect_locked();
                throw_link_error(
                    std::string(tcp ? "AUX write failed: " : "serial write failed: ") + util::errno_string(error), tcp);
            }
        }
    } catch (...) {
        // A later stop could complete the unfinished TCP frame as a motion command.
        if (tcp_ && written > 0 && written < bytes.size()) {
            disconnect_locked();
        }
        throw;
    }
}

std::vector<std::uint8_t> CelestronAuxLink::read_frame_locked(std::chrono::steady_clock::time_point deadline) {
    const bool tcp = tcp_;
    while (true) {
        if (std::chrono::steady_clock::now() >= deadline) {
            throw_link_error("AUX transaction timed out", tcp);
        }
        const auto start = std::find(receive_buffer_.begin(), receive_buffer_.end(), kAuxStart);
        receive_buffer_.erase(receive_buffer_.begin(), start);
        if (receive_buffer_.size() >= 2) {
            const std::size_t length = receive_buffer_[1];
            if (length < 3 || length + 3 > kMaximumFrameSize) {
                receive_buffer_.erase(receive_buffer_.begin());
                continue;
            }
            const std::size_t frame_size = length + 3;
            if (receive_buffer_.size() >= frame_size) {
                std::vector<std::uint8_t> frame(receive_buffer_.begin(),
                                                receive_buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size));
                receive_buffer_.erase(receive_buffer_.begin(),
                                      receive_buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size));
                return frame;
            }
        }

        wait_for_locked(POLLIN, deadline);
        std::array<std::uint8_t, 128> incoming{};
        const ssize_t count = ::read(serial_fd_, incoming.data(), incoming.size());
        if (count > 0) {
            receive_buffer_.insert(receive_buffer_.end(), incoming.begin(),
                                   incoming.begin() + static_cast<std::ptrdiff_t>(count));
        } else if (count == 0) {
            disconnect_locked();
            throw_link_error(tcp ? "AUX transport returned end-of-file" : "serial device returned end-of-file", tcp);
        } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            const int error = errno;
            disconnect_locked();
            throw_link_error(
                std::string(tcp ? "AUX read failed: " : "serial read failed: ") + util::errno_string(error), tcp);
        }
    }
}

}  // namespace alpacacore::vendor::celestron
