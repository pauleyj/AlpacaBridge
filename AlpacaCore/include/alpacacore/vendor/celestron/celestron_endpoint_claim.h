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

#include <alpacacore/util/error_handling.h>
#include <arpa/inet.h>

#include <mutex>
#include <set>
#include <string>
#include <utility>

namespace alpacacore::vendor::celestron {

// Shared by HC and raw AUX, with one atomic claim before either opens TCP.
class CelestronEndpointClaim final {
public:
    ~CelestronEndpointClaim() { release(); }
    CelestronEndpointClaim() = default;
    CelestronEndpointClaim(const CelestronEndpointClaim&) = delete;
    CelestronEndpointClaim& operator=(const CelestronEndpointClaim&) = delete;

    void claim(const sockaddr* address, int port) {
        char text[INET6_ADDRSTRLEN]{};
        if (address->sa_family == AF_INET) {
            ::inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in*>(address)->sin_addr, text, sizeof(text));
        } else {
            const auto& ipv6 = reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr;
            if (IN6_IS_ADDR_V4MAPPED(&ipv6)) {
                ::inet_ntop(AF_INET, &ipv6.s6_addr[12], text, sizeof(text));
            } else {
                ::inet_ntop(AF_INET6, &ipv6, text, sizeof(text));
            }
        }
        std::string key = std::string(text) + ":" + std::to_string(port);
        std::lock_guard<std::mutex> lock(mutex());
        if (!key_.empty() || !endpoints().insert(key).second) {
            throw AlpacaException("Celestron network endpoint is already in use: " + key,
                                  AlpacaError::InvalidOperation);
        }
        key_ = std::move(key);
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex());
        if (!key_.empty()) endpoints().erase(key_);
        key_.clear();
    }

private:
    static std::mutex& mutex() {
        static std::mutex value;
        return value;
    }
    static std::set<std::string>& endpoints() {
        static std::set<std::string> value;
        return value;
    }
    std::string key_;
};

}  // namespace alpacacore::vendor::celestron
