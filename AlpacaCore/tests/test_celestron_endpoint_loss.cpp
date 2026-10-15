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

#include <alpacacore/util/serial_io.h>
#include <alpacacore/vendor/celestron/celestron_endpoint_claim.h>
#include <alpacacore/vendor/celestron/celestron_protocol_wrapper.h>
#include <alpacacore/vendor/celestron/celestron_telescope_driver.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>

#include "catch2_compat.h"
#include "fake_mount_server.h"

namespace {

using namespace alpacacore;
using namespace alpacacore::vendor::celestron;
using namespace std::chrono_literals;

// No worker or device protocol: each test controls a bounded loopback TCP peer.
class EndpointLossFixture {
public:
    EndpointLossFixture() {
        hc.disconnect();
        listener_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (listener_ < 0) return;
        address_.sin_family = AF_INET;
        address_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        const int receive_buffer = 4096;
        if (::setsockopt(listener_, SOL_SOCKET, SO_RCVBUF, &receive_buffer, sizeof(receive_buffer)) != 0) return;
        if (::bind(listener_, reinterpret_cast<const sockaddr*>(&address_), sizeof(address_)) != 0 ||
            ::listen(listener_, 1) != 0)
            return;
        socklen_t size = sizeof(address_);
        if (::getsockname(listener_, reinterpret_cast<sockaddr*>(&address_), &size) == 0)
            port_ = ntohs(address_.sin_port);
    }
    ~EndpointLossFixture() {
        hc.disconnect();
        if (peer_ >= 0) ::close(peer_);
        if (listener_ >= 0) ::close(listener_);
    }
    EndpointLossFixture(const EndpointLossFixture&) = delete;
    EndpointLossFixture& operator=(const EndpointLossFixture&) = delete;
    bool ok() const { return port_ > 0; }
    void connect() {
        ConnectionInfo info;
        info.type = ConnectionType::Network;
        info.host = "127.0.0.1";
        info.tcp_port = port_;
        info.response_timeout_ms = 50;
        REQUIRE(hc.connect(info));
        pollfd descriptor{listener_, POLLIN, 0};
        REQUIRE(::poll(&descriptor, 1, 1000) > 0);
        peer_ = ::accept4(listener_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        REQUIRE(peer_ >= 0);
    }
    void reply(std::string_view bytes) {
        REQUIRE(util::send_all(peer_, bytes.data(), bytes.size(), MSG_NOSIGNAL | MSG_DONTWAIT));
    }
    void finish_reply() {
        // Half-close keeps the read side alive so an ensuing command cannot provoke RST.
        REQUIRE(::shutdown(peer_, SHUT_WR) == 0);
    }
    void reset_peer() {
        const linger reset{1, 0};
        REQUIRE(::setsockopt(peer_, SOL_SOCKET, SO_LINGER, &reset, sizeof(reset)) == 0);
        REQUIRE(::close(peer_) == 0);
        peer_ = -1;
    }
    void claim(CelestronEndpointClaim& claim) const {
        claim.claim(reinterpret_cast<const sockaddr*>(&address_), port_);
    }

    CelestronProtocolWrapper& hc = CelestronProtocolWrapper::instance();

private:
    int listener_ = -1;
    int peer_ = -1;
    int port_ = 0;
    sockaddr_in address_{};
};

}  // namespace

TEST_CASE("Celestron HC TCP - FIN releases ownership during normal and binary replies", "[celestron][hc][tcp]") {
    for (const int binary_bytes : {0, 1, 2}) {
        CAPTURE(binary_bytes);
        EndpointLossFixture fixture;
        REQUIRE(fixture.ok());
        fixture.connect();
        if (binary_bytes > 0) fixture.reply(std::string("\x05\x23", binary_bytes));
        fixture.finish_reply();
        // One byte fails in the binary payload; two fail in the '#' drain.
        CHECK_THROWS_AS(binary_bytes == 0 ? fixture.hc.send_raw_command("") : fixture.hc.get_handset_firmware_version(),
                        AlpacaException);
        CHECK_FALSE(fixture.hc.is_connected());
        CelestronEndpointClaim replacement;
        CHECK_NOTHROW(fixture.claim(replacement));
    }
}

TEST_CASE("Celestron HC TCP - RST send failure releases ownership for normal and blind commands",
          "[celestron][hc][tcp]") {
    for (const bool blind : {false, true}) {
        CAPTURE(blind);
        EndpointLossFixture fixture;
        REQUIRE(fixture.ok());
        fixture.connect();
        fixture.reset_peer();
        // A large send must encounter the reset even if its first bytes were accepted.
        const std::string command(8 * 1024 * 1024, 'x');
        try {
            if (blind)
                fixture.hc.send_command_blind(command);
            else
                static_cast<void>(fixture.hc.send_command(command));
            FAIL("Expected a reset peer to refuse the send");
        } catch (const AlpacaException& error) {
            CHECK(std::string(error.what()) == "Failed to send command to Celestron mount");
            CHECK(error.error_code() == AlpacaError::DriverException);
        }
        CHECK_FALSE(fixture.hc.is_connected());
        CelestronEndpointClaim replacement;
        CHECK_NOTHROW(fixture.claim(replacement));
    }
}

TEST_CASE("Celestron HC TCP - RST response loss releases endpoint ownership", "[celestron][hc][tcp]") {
    EndpointLossFixture fixture;
    REQUIRE(fixture.ok());
    fixture.connect();
    fixture.reset_peer();
    // Empty sends perform no syscall, leaving ECONNRESET for the response reader.
    CHECK_THROWS_AS(fixture.hc.send_raw_command(""), AlpacaException);
    CHECK_FALSE(fixture.hc.is_connected());
    CelestronEndpointClaim replacement;
    CHECK_NOTHROW(fixture.claim(replacement));
}

TEST_CASE("Celestron HC TCP - blind response loss is not swallowed", "[celestron][hc][tcp]") {
    EndpointLossFixture fixture;
    REQUIRE(fixture.ok());
    fixture.connect();
    fixture.finish_reply();
    // An empty send isolates EOF in the optional response drain, not send failure.
    CHECK_THROWS_AS(fixture.hc.send_command_blind(""), AlpacaException);
    CHECK_FALSE(fixture.hc.is_connected());
    CelestronEndpointClaim replacement;
    CHECK_NOTHROW(fixture.claim(replacement));
}

TEST_CASE("Celestron HC TCP - live send backpressure retains endpoint ownership", "[celestron][hc][tcp]") {
    EndpointLossFixture fixture;
    REQUIRE(fixture.ok());
    fixture.connect();
    // The peer deliberately does not drain its small receive window.
    const auto started = std::chrono::steady_clock::now();
    CHECK_THROWS_AS(fixture.hc.send_command_blind(std::string(8 * 1024 * 1024, 'x')), AlpacaException);
    CHECK(std::chrono::steady_clock::now() - started < 5s);
    CHECK(fixture.hc.is_connected());
    CelestronEndpointClaim competing;
    CHECK_THROWS_AS(fixture.claim(competing), AlpacaException);
    fixture.hc.disconnect();
    CHECK_NOTHROW(fixture.claim(competing));
}

TEST_CASE("Celestron HC TCP - live response timeouts retain endpoint ownership", "[celestron][hc][tcp]") {
    for (const int response : {0, 1, 2}) {
        CAPTURE(response);
        EndpointLossFixture fixture;
        REQUIRE(fixture.ok());
        fixture.connect();
        if (response == 0) {
            CHECK_THROWS_AS(fixture.hc.send_command("Kx"), AlpacaException);
        } else if (response == 1) {
            CHECK_NOTHROW(fixture.hc.send_command_blind("Kx"));
        } else {
            fixture.reply("\x05\x23");
            CHECK(fixture.hc.get_handset_firmware_version() == "5.35");
        }
        CHECK(fixture.hc.is_connected());
        CelestronEndpointClaim competing;
        CHECK_THROWS_AS(fixture.claim(competing), AlpacaException);
        fixture.hc.disconnect();
        CHECK_NOTHROW(fixture.claim(competing));
    }
}

TEST_CASE("Celestron HC TCP - a lost socket retains its driver's singleton reservation until teardown",
          "[celestron][hc][tcp]") {
    const auto reply = [](const std::string& command) {
        if (command == "V") return std::string("\x05\x23#", 3);
        if (command.size() == 8 && command[0] == 'P')
            return std::string(static_cast<unsigned char>(command[7]), '\0') + "#";
        return test::FakeMountServer::default_responder()(command);
    };
    for (const bool destroy_owner : {false, true}) {
        CAPTURE(destroy_owner);
        CelestronProtocolWrapper::instance().disconnect();
        test::FakeMountServer first(reply);
        std::atomic<int> second_requests{0};
        test::FakeMountServer second([&](const std::string& command) {
            ++second_requests;
            return reply(command);
        });
        REQUIRE(first.ok());
        REQUIRE(second.ok());
        ConnectionInfo info;
        info.type = ConnectionType::Network;
        info.host = "127.0.0.1";
        info.tcp_port = first.port();
        info.response_timeout_ms = 50;
        auto owner = create_celestron_telescope_with_site(0, info, std::nullopt, std::nullopt, std::nullopt, false);
        info.tcp_port = second.port();
        auto contender = create_celestron_telescope_with_site(1, info, std::nullopt, std::nullopt, std::nullopt, false);
        REQUIRE_NOTHROW(owner->set_connected(true));
        REQUIRE(first.drop_connections());
        CHECK_THROWS_AS(owner->command_string("Kx", true), AlpacaException);
        CHECK_FALSE(CelestronProtocolWrapper::instance().is_connected());
        CHECK_THROWS_AS(contender->set_connected(true), AlpacaException);
        CHECK_FALSE(contender->get_connected());
        CHECK_THROWS_AS(owner->command_string("Ky", true), AlpacaException);
        CHECK(second_requests.load() == 0);
        // Releasing the lost endpoint must not release the logical HC owner.
        sockaddr_in address{};
        REQUIRE(::inet_pton(AF_INET, info.host.c_str(), &address.sin_addr) == 1);
        address.sin_family = AF_INET;
        CelestronEndpointClaim aux_claim;
        CHECK_NOTHROW(aux_claim.claim(reinterpret_cast<const sockaddr*>(&address), first.port()));
        if (destroy_owner)
            owner.reset();
        else
            owner->set_connected(false);
        CelestronEndpointClaim competing;
        CHECK_THROWS_AS(competing.claim(reinterpret_cast<const sockaddr*>(&address), first.port()), AlpacaException);
        REQUIRE_NOTHROW(contender->set_connected(true));
        CHECK(contender->command_string("Kq", true) == "q");
        owner.reset();
        CHECK(contender->command_string("Kr", true) == "r");
    }
}
