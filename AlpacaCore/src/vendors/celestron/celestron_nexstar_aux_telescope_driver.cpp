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

#include <alpacacore/async_connectable.h>
#include <alpacacore/util/async_operation.h>
#include <alpacacore/util/link_health.h>
#include <alpacacore/util/state_snapshot.h>
#include <alpacacore/vendor/celestron/celestron_aux_link.h>
#include <alpacacore/vendor/celestron/celestron_cgx_geometry.h>
#include <alpacacore/vendor/celestron/celestron_nexstar_aux_telescope_driver.h>
#include <alpacacore/version.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <numbers>

namespace alpacacore::vendor::celestron {

namespace {
using namespace std::chrono_literals;
constexpr std::array axes{CgxAxis::RightAscension, CgxAxis::Declination};

void range(double value, double minimum, double maximum, const char* label) {
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        throw AlpacaException(std::string(label) + " is outside its valid range", AlpacaError::InvalidValue);
    }
}
void axis_range(int axis) {
    if (axis < 0 || axis > 2) throw AlpacaException("Axis must be 0, 1 or 2", AlpacaError::InvalidValue);
}
[[noreturn]] void unsupported(const char* member) {
    throw AlpacaException(std::string(member) + " is not supported by the initial SkyPortal AUX profile",
                          AlpacaError::NotImplemented);
}
[[noreturn]] void uncalibrated() {
    throw AlpacaException("SkyPortal AUX pointing reference is not established; coordinate operations are unavailable",
                          AlpacaError::InvalidOperation);
}

struct AuxStatus {
    std::array<std::uint32_t, 2> position{};
    bool moving = false;
    bool guiding = false;
};

class CelestronNexStarAuxTelescopeDriver final : public TelescopeDriver, protected AsyncConnectable {
public:
    CelestronNexStarAuxTelescopeDriver(int number, NexStarAuxConnectionInfo info, util::TaskClock& clock)
        : AsyncConnectable("Celestron AUX"),
          number_(number),
          info_(std::move(info)),
          clock_(clock),
          snapshot_(clock, 3s),
          poll_("Celestron AUX status", generation_, clock) {}
    ~CelestronNexStarAuxTelescopeDriver() override {
        shutdown_connection();
        try {
            set_connected(false);
        } catch (const std::exception& error) {
            ALPACA_LOG_ERROR("Celestron AUX", std::string("Shutdown could not verify stopping: ") + error.what());
        }
    }
    ALPACA_EXPOSE_CONNECT_ERROR()
    int get_device_number() const override { return number_; }
    DeviceType get_device_type() const override { return DeviceType::Telescope; }
    std::string get_name() const override { return "Celestron SkyPortal AUX Telescope"; }
    std::string get_unique_id() const override { return "Celestron_NexStar_AUX_" + std::to_string(number_); }
    std::string get_description() const override {
        return "Celestron CGX SkyPortal AUX mount (lifecycle-only profile)";
    }
    std::string get_driver_info() const override { return "AlpacaCore Celestron SkyPortal AUX Driver"; }
    std::string get_driver_version() const override { return kVersion; }
    int get_interface_version() const override { return 4; }
    bool get_connected() const override { return connected_.load(); }
    bool get_connecting() const override { return connection_task_active(); }
    void connect() override { start_connection_task(true); }
    void disconnect() override { start_connection_task(false); }
    void set_connected(bool value) override;
    std::optional<std::string> get_device_firmware() const override {
        std::lock_guard<std::mutex> lock(firmware_mutex_);
        return firmware_;
    }
    std::string get_link_fault() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return health_.fault();
    }
    std::vector<std::string> get_supported_actions() const override { return {}; }
    bool can_action(std::string_view) const override { return false; }
    std::string action(std::string_view, std::string_view) override {
        throw AlpacaException("No SkyPortal AUX custom actions are supported", AlpacaError::ActionNotImplemented);
    }
    std::string command_blind(std::string_view, bool) override { unsupported("CommandBlind"); }
    bool command_bool(std::string_view, bool) override { unsupported("CommandBool"); }
    std::string command_string(std::string_view, bool) override { unsupported("CommandString"); }

    AlignmentMode get_alignment_mode() const override { return AlignmentMode::GermanPolar; }
    EquatorialSystem get_equatorial_system() const override { return EquatorialSystem::Topocentric; }
    bool get_can_find_home() const override { return false; }
    bool get_can_park() const override { return false; }
    bool get_can_unpark() const override { return false; }
    bool get_can_set_park() const override { return false; }
    bool get_can_pulse_guide() const override { return false; }
    bool get_can_set_guide_rates() const override { return false; }
    bool get_can_set_declination_rate() const override { return false; }
    bool get_can_set_right_ascension_rate() const override { return false; }
    bool get_can_set_tracking() const override { return false; }
    bool get_can_set_pier_side() const override { return false; }
    bool get_can_slew() const override { return false; }
    bool get_can_slew_async() const override { return false; }
    bool get_can_slew_alt_az() const override { return false; }
    bool get_can_slew_alt_az_async() const override { return false; }
    bool get_can_sync() const override { return false; }
    bool get_can_sync_alt_az() const override { return false; }
    bool get_can_move_axis(int axis) const override {
        axis_range(axis);
        return false;
    }
    std::pair<double, double> get_axis_rate_range(int axis) const override {
        axis_range(axis);
        require_connected();
        return {0.0, 0.0};
    }
    std::vector<std::pair<double, double>> get_axis_rate_ranges(int axis) const override {
        axis_range(axis);
        require_connected();
        return {};
    }
    bool get_at_home() const override {
        healthy_status();
        return false;
    }
    bool get_at_park() const override {
        healthy_status();
        return false;
    }
    bool get_tracking() const override {
        healthy_status();
        return false;
    }
    bool get_slewing() const override { return healthy_status().moving; }
    bool get_is_pulse_guiding() const override { return healthy_status().guiding; }
    double get_right_ascension() const override {
        healthy_status();
        uncalibrated();
    }
    double get_declination() const override {
        healthy_status();
        uncalibrated();
    }
    double get_altitude() const override {
        healthy_status();
        uncalibrated();
    }
    double get_azimuth() const override {
        healthy_status();
        uncalibrated();
    }
    int get_side_of_pier() const override {
        healthy_status();
        uncalibrated();
    }
    int get_destination_side_of_pier(double ra, double dec) const override {
        range(ra, 0, 24, "RightAscension");
        range(dec, -90, 90, "Declination");
        healthy_status();
        uncalibrated();
    }
    double get_right_ascension_rate() const override {
        healthy_status();
        return 0;
    }
    double get_declination_rate() const override {
        healthy_status();
        return 0;
    }
    void set_right_ascension_rate(double) override {
        require_connected();
        unsupported("RightAscensionRate");
    }
    void set_declination_rate(double) override {
        require_connected();
        unsupported("DeclinationRate");
    }
    GuideRate get_guide_rate() const override {
        require_connected();
        unsupported("GuideRate");
    }
    void set_guide_rate(const GuideRate&) override {
        require_connected();
        unsupported("GuideRate");
    }
    void set_tracking(bool) override {
        require_connected();
        unsupported("Tracking writes");
    }
    int get_tracking_rate() const override {
        healthy_status();
        return 0;
    }
    std::vector<int> get_tracking_rates() const override {
        require_connected();
        return {0};
    }
    void set_tracking_rate(int rate) override {
        if (rate != 0) throw AlpacaException("Only sidereal TrackingRate is supported", AlpacaError::InvalidValue);
        require_connected();
        unsupported("TrackingRate writes");
    }
    bool get_does_refraction() const override {
        require_connected();
        return false;
    }
    void set_does_refraction(bool) override {
        require_connected();
        unsupported("DoesRefraction writes");
    }
    void set_side_of_pier(int side) override {
        if (side != 0 && side != 1) throw AlpacaException("SideOfPier must be 0 or 1", AlpacaError::InvalidValue);
        require_connected();
        unsupported("SideOfPier writes");
    }

    double get_aperture_diameter() const override { return read_value(info_.aperture_diameter); }
    double get_aperture_area() const override {
        const double diameter = get_aperture_diameter();
        const double area = std::numbers::pi * diameter * diameter / 4;
        if (!std::isfinite(area))
            throw AlpacaException("ApertureArea cannot be represented", AlpacaError::DriverException);
        return area;
    }
    double get_focal_length() const override { return read_value(info_.focal_length); }
    void set_aperture_diameter(double value) override {
        range(value, 0, std::numeric_limits<double>::max(), "ApertureDiameter");
        write_value(info_.aperture_diameter, value);
    }
    void set_focal_length(double value) override {
        range(value, 0, std::numeric_limits<double>::max(), "FocalLength");
        write_value(info_.focal_length, value);
    }
    double get_site_elevation() const override { return read_value(info_.site_elevation); }
    void set_site_elevation(double value) override {
        range(value, -300, 10000, "SiteElevation");
        write_value(info_.site_elevation, value);
    }
    double get_site_latitude() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        return *info_.site_latitude;
    }
    double get_site_longitude() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        return *info_.site_longitude;
    }
    void set_site_latitude(double value) override {
        range(value, -90, 90, "SiteLatitude");
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        info_.site_latitude = value;
    }
    void set_site_longitude(double value) override {
        range(value, -180, 180, "SiteLongitude");
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        info_.site_longitude = value;
    }
    double get_sidereal_time() const override {
        return cgx_local_sidereal_time_hours(get_utc_date(), get_site_longitude());
    }
    std::chrono::system_clock::time_point get_utc_date() const override {
        require_connected();
        return std::chrono::system_clock::now();
    }
    void set_utc_date(std::chrono::system_clock::time_point) override {
        require_connected();
        unsupported("UTCDate writes");
    }
    int get_slew_settle_time() const override { return read_value(settle_seconds_); }
    void set_slew_settle_time(int value) override {
        if (value < 0) throw AlpacaException("SlewSettleTime must be non-negative", AlpacaError::InvalidValue);
        write_value(settle_seconds_, value);
    }
    double get_target_right_ascension() const override { return target(true); }
    double get_target_declination() const override { return target(false); }
    void set_target_right_ascension(double value) override {
        range(value, 0, 24, "TargetRightAscension");
        set_target(true, value);
    }
    void set_target_declination(double value) override {
        range(value, -90, 90, "TargetDeclination");
        set_target(false, value);
    }

    void find_home() override {
        require_connected();
        unsupported("FindHome");
    }
    void park() override {
        require_connected();
        unsupported("Park");
    }
    void unpark() override {
        require_connected();
        unsupported("Unpark");
    }
    void set_park() override {
        require_connected();
        unsupported("SetPark");
    }
    void abort_slew() override { healthy_status(); /* No owned motion is enabled in this profile. */ }
    void move_axis(int axis, double rate) override {
        axis_range(axis);
        range(rate, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), "Rate");
        require_connected();
        unsupported("MoveAxis");
    }
    void pulse_guide(int direction, int duration) override {
        if (direction < 0 || direction > 3 || duration <= 0 || duration > 600000)
            throw AlpacaException("PulseGuide direction or duration is invalid", AlpacaError::InvalidValue);
        require_connected();
        unsupported("PulseGuide");
    }
    void slew_to_coordinates(double, double) override {
        require_connected();
        unsupported("SlewToCoordinates");
    }
    void slew_to_target() override {
        require_connected();
        unsupported("SlewToTarget");
    }
    void slew_to_coordinates_async(double ra, double dec) override {
        range(ra, 0, 24, "RightAscension");
        range(dec, -90, 90, "Declination");
        require_connected();
        unsupported("SlewToCoordinatesAsync");
    }
    void sync_to_coordinates(double ra, double dec) override {
        range(ra, 0, 24, "RightAscension");
        range(dec, -90, 90, "Declination");
        require_connected();
        unsupported("SyncToCoordinates");
    }
    void slew_to_target_async() override {
        require_targets();
        unsupported("SlewToTargetAsync");
    }
    void sync_to_target() override {
        require_targets();
        unsupported("SyncToTarget");
    }
    void slew_to_alt_az(double, double) override {
        require_connected();
        unsupported("SlewToAltAz");
    }
    void slew_to_alt_az_async(double, double) override {
        require_connected();
        unsupported("SlewToAltAzAsync");
    }
    void sync_to_alt_az(double, double) override {
        require_connected();
        unsupported("SyncToAltAz");
    }

protected:
    std::optional<std::chrono::nanoseconds> device_state_age() const override {
        const auto frame = snapshot_.read();
        return frame ? std::optional<std::chrono::nanoseconds>(clock_.now() - frame->measured_at) : std::nullopt;
    }

private:
    void require_connected() const {
        if (!connected_) throw AlpacaException("SkyPortal AUX is not connected", AlpacaError::NotConnected);
    }
    template <typename T>
    T read_value(const T& value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        return value;
    }
    template <typename T>
    void write_value(T& destination, T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        destination = value;
    }
    void set_target(bool ra, double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        const int index = ra ? 0 : 1;
        targets_[index] = value;
        target_set_[index] = true;
    }
    double target(bool ra) const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        const int index = ra ? 0 : 1;
        if (!target_set_[index])
            throw AlpacaException("Target coordinate has not been set", AlpacaError::InvalidOperation);
        return targets_[index];
    }
    void require_targets() const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        if (!target_set_[0] || !target_set_[1])
            throw AlpacaException("Both target coordinates must be set", AlpacaError::InvalidOperation);
    }
    AuxStatus healthy_status() const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        if (health_.faulted())
            throw AlpacaException("SkyPortal AUX communications compromised: " + health_.fault(),
                                  AlpacaError::DriverException);
        const auto frame = snapshot_.read();
        if (!frame || frame->stale)
            throw AlpacaException("SkyPortal AUX status telemetry is stale", AlpacaError::DriverException);
        return frame->value;
    }
    AuxFrame query(CgxAxis axis, std::uint8_t command, std::vector<std::uint8_t> payload = {}) {
        return link_.exchange(AuxFrame{0x20, static_cast<std::uint8_t>(axis), command, std::move(payload)},
                              response_budget());
    }
    std::chrono::milliseconds response_budget() const {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(io_deadline_ - clock_.now());
        if (remaining <= 0ms)
            throw AlpacaException("SkyPortal AUX operation deadline exceeded", AlpacaError::DriverException);
        return std::min(remaining, info_.response_timeout);
    }
    AuxStatus sample();
    AuxStatus takeover();
    std::string identify();
    void poll_status(util::OperationContext& context);
    void lose_link_locked();
    void clear_firmware() {
        std::lock_guard<std::mutex> lock(firmware_mutex_);
        firmware_.reset();
    }

    const int number_;
    NexStarAuxConnectionInfo info_;
    util::TaskClock& clock_;
    mutable std::mutex mutex_;
    std::mutex transition_mutex_;
    mutable std::mutex firmware_mutex_;
    std::optional<std::string> firmware_;
    std::atomic<bool> connected_{false};
    CelestronAuxLink link_;
    util::StateSnapshot<AuxStatus> snapshot_;
    util::PolledLinkHealth health_;
    util::LinkLostStamp lost_at_;
    util::OperationGeneration generation_;
    util::AsyncOperation poll_;
    // Only the serialized connection path or its joined-before-teardown poll owns I/O.
    util::TaskClock::clock::time_point io_deadline_{};
    std::array<double, 2> targets_{};
    std::array<bool, 2> target_set_{};
    int settle_seconds_ = 0;
};

std::string CelestronNexStarAuxTelescopeDriver::identify() {
    std::array<AuxFrame, 2> versions;
    for (int i = 0; i < 2; ++i) {
        versions[i] = query(axes[i], 0xfe);
        if (versions[i].payload.size() != 2)
            throw AlpacaException("SkyPortal AUX motor firmware reply must contain two bytes",
                                  AlpacaError::DriverException);
        // TODO: qualify other firmware profiles before relying on guide-active queries there.
        const auto& version = versions[i].payload;
        if (version[0] * 256 + version[1] < 6 * 256 + 50)
            throw AlpacaException("SkyPortal AUX takeover requires motor firmware 6.50 or newer",
                                  AlpacaError::DriverException);
    }
    const auto model = query(CgxAxis::RightAscension, 0x05);
    if (model.payload != std::vector<std::uint8_t>{23})
        throw AlpacaException("This SkyPortal AUX profile supports CGX (model 23) only; other models are not qualified",
                              AlpacaError::DriverException);
    for (const auto axis : axes) {
        const auto approach = query(axis, 0xfc);
        if (approach.payload.size() != 1 || approach.payload[0] > 1)
            throw AlpacaException("SkyPortal AUX controller approach direction is invalid",
                                  AlpacaError::DriverException);
    }
    return "RA " + std::to_string(versions[0].payload[0]) + "." + std::to_string(versions[0].payload[1]) + ", Dec " +
           std::to_string(versions[1].payload[0]) + "." + std::to_string(versions[1].payload[1]);
}

AuxStatus CelestronNexStarAuxTelescopeDriver::sample() {
    AuxStatus status;
    for (int i = 0; i < 2; ++i) {
        status.position[i] = link_.get_axis_position(axes[i], response_budget());
        const auto done = query(axes[i], 0x13);
        if (done.payload.size() != 1 || (done.payload[0] != 0 && done.payload[0] != 0xff && done.payload[0] != 0xfe))
            throw AlpacaException("SkyPortal AUX slew status is invalid", AlpacaError::DriverException);
        status.moving = status.moving || done.payload[0] == 0;
        status.guiding = link_.is_aux_guide_active(axes[i], response_budget()) || status.guiding;
    }
    for (const auto& report : link_.take_controller_reports()) {
        if (report.command == 0x50 && report.payload[0] == 0) {
            ALPACA_LOG_WARN("Celestron AUX", "Motor controller reports a low battery");
        } else {
            throw AlpacaException(
                "SkyPortal AUX controller reports a motion limit or error; disable competing bus masters and reconnect",
                AlpacaError::DriverException);
        }
    }
    return status;
}

AuxStatus CelestronNexStarAuxTelescopeDriver::takeover() {
    std::exception_ptr failure;
    for (const auto axis : axes) {
        try {
            if (!query(axis, 0x24, {0}).payload.empty())
                throw AlpacaException("SkyPortal AUX stop acknowledgement is invalid", AlpacaError::DriverException);
        } catch (...) {
            if (!failure) failure = std::current_exception();
        }
    }
    if (failure) {
        for (const auto axis : axes) {
            try {
                link_.send_stop(axis, 250ms);
            } catch (const std::exception& error) {
                ALPACA_LOG_ERROR("Celestron AUX", error.what());
            }
        }
        std::rethrow_exception(failure);
    }
    const auto deadline = std::min(clock_.now() + 3500ms, io_deadline_);
    auto previous = sample();
    do {
        clock_.sleep_for(500ms);
        if (clock_.now() >= deadline) break;
        const auto current = sample();
        // TODO: qualify stationary-count tolerance and native pulse cessation on the northern CGX rig.
        if (!current.moving && !current.guiding && current.position == previous.position) return current;
        previous = current;
    } while (clock_.now() < deadline);
    throw AlpacaException(
        "SkyPortal AUX could not verify both axes stopped; disable HC tracking and competing apps before Connect",
        AlpacaError::DriverException);
}

void CelestronNexStarAuxTelescopeDriver::set_connected(bool value) {
    std::lock_guard<std::mutex> transition(transition_mutex_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!value && record_disconnect_if_connect_in_flight(connected_.load())) return;
        if (value && consume_pending_disconnect(connected_.load())) return;
        if (value && connected_ && !health_.faulted()) return;
        clear_firmware();
        connected_ = false;
        snapshot_.reset();
        target_set_ = {};
        if (value) health_.reset();
    }
    poll_.cancel_all_and_join();
    clear_firmware();
    io_deadline_ = clock_.now() + 4s;
    if (!value) {
        std::exception_ptr failure;
        std::string stop_error;
        try {
            if (link_.is_connected()) static_cast<void>(takeover());
        } catch (const std::exception& error) {
            failure = std::current_exception();
            stop_error = error.what();
        } catch (...) {
            failure = std::current_exception();
            stop_error = "unknown stop verification failure";
        }
        link_.disconnect();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            health_.reset();
            // AsyncConnectable logs teardown exceptions rather than keeping a
            // Connect error. Keep unverified stopping visible in management too.
            if (failure) health_.note_failure("Stop verification failed: " + stop_error, 1, clock_.now());
        }
        if (failure) std::rethrow_exception(failure);
        return;
    }
    link_.disconnect();
    try {
        NexStarAuxConnectionInfo config;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            config = info_;
        }
        if (!config.site_latitude || !config.site_longitude)
            throw AlpacaException("Set SiteLatitude and SiteLongitude before SkyPortal AUX Connect",
                                  AlpacaError::DriverException);
        if (*config.site_latitude < 0)
            throw AlpacaException(
                "Southern SkyPortal AUX operation is not qualified; this profile requires a northern CGX",
                AlpacaError::DriverException);
        if (config.response_timeout < 1ms || config.response_timeout > 5000ms)
            throw AlpacaException("SkyPortal AUX responseTimeoutMs must be between 1 and 5000",
                                  AlpacaError::DriverException);
        link_.connect_tcp(config.host, config.tcp_port, 500ms);
        const auto firmware = identify();
        const auto stopped = takeover();
        static_cast<void>(
            lost_at_.consume());  // Approved policy always stops on relink; no surviving motion is resumed.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (consume_pending_disconnect(false)) {
                link_.disconnect();
                return;
            }
            health_.reset();
            snapshot_.publish(snapshot_.begin_poll(), stopped);
            connected_ = true;
            std::lock_guard<std::mutex> firmware_lock(firmware_mutex_);
            firmware_ = firmware;
        }
        poll_.start([this](util::OperationContext& context) { poll_status(context); });
    } catch (...) {
        clear_firmware();
        connected_ = false;
        snapshot_.reset();
        link_.disconnect();
        throw;
    }
}

void CelestronNexStarAuxTelescopeDriver::lose_link_locked() {
    lost_at_.note(clock_.now());
    clear_firmware();
    connected_ = false;
    snapshot_.reset();
    target_set_ = {};
}

void CelestronNexStarAuxTelescopeDriver::poll_status(util::OperationContext& context) {
    while (context.wait_for(1s)) {
        if (!connected_) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (health_.faulted()) {
                if (health_.fault_stale(clock_.now())) {
                    lose_link_locked();
                    link_.disconnect();
                    return;
                }
                continue;  // A latched ownership/report fault also needs explicit client recovery.
            }
        }
        try {
            io_deadline_ = clock_.now() + 3s;
            const auto token = snapshot_.begin_poll();
            const auto current = sample();
            std::lock_guard<std::mutex> lock(mutex_);
            if (!connected_ || context.stop_reason() != util::StopReason::None) return;
            const auto previous = snapshot_.read();
            if (current.moving || current.guiding || (previous && current.position != previous->value.position))
                throw AlpacaException(
                    "Uncommanded axis motion detected; disable HC tracking and competing apps before reconnecting",
                    AlpacaError::DriverException);
            health_.on_reply();
            snapshot_.publish(token, current);
        } catch (const std::exception& error) {
            // A faulted/poisoned stream cannot recover without the client's reconnect.
            std::lock_guard<std::mutex> lock(mutex_);
            if (!connected_) return;
            if (health_.note_failure(error.what(), 1, clock_.now())) ALPACA_LOG_ERROR("Celestron AUX", error.what());
            if (!link_.is_connected() || health_.fault_stale(clock_.now())) {
                lose_link_locked();
                link_.disconnect();
                return;
            }
        }
    }
}

}  // namespace

std::unique_ptr<TelescopeDriver> create_celestron_nexstar_aux_telescope(int device_number,
                                                                        const NexStarAuxConnectionInfo& info,
                                                                        util::TaskClock& clock) {
    if (info.site_latitude) range(*info.site_latitude, -90, 90, "SiteLatitude");
    if (info.site_longitude) range(*info.site_longitude, -180, 180, "SiteLongitude");
    range(info.site_elevation, -300, 10000, "SiteElevation");
    range(info.aperture_diameter, 0, std::numeric_limits<double>::max(), "ApertureDiameter");
    range(info.focal_length, 0, std::numeric_limits<double>::max(), "FocalLength");
    return std::make_unique<CelestronNexStarAuxTelescopeDriver>(device_number, info, clock);
}

}  // namespace alpacacore::vendor::celestron
