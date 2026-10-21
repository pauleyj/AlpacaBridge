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
#include <alpacacore/util/poll_settle.h>
#include <alpacacore/util/state_snapshot.h>
#include <alpacacore/vendor/celestron/celestron_aux_link.h>
#include <alpacacore/vendor/celestron/celestron_cgx_geometry.h>
#include <alpacacore/vendor/celestron/celestron_nexstar_aux_telescope_driver.h>
#include <alpacacore/version.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <numbers>
#include <utility>

namespace alpacacore::vendor::celestron {

namespace {
using namespace std::chrono_literals;
constexpr std::array axes{CgxAxis::RightAscension, CgxAxis::Declination};

class AuxOperationDeadline final : public AlpacaException {
public:
    AuxOperationDeadline()
        : AlpacaException("SkyPortal AUX operation deadline exceeded", AlpacaError::DriverException) {}
};

class AuxSlewFailure final : public AlpacaException {
public:
    explicit AuxSlewFailure(const std::string& message) : AlpacaException(message, AlpacaError::DriverException) {}
};

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
    std::chrono::system_clock::time_point ra_sample_time{};
    bool moving = false;
    std::array<bool, 2> axis_moving{};
    bool guiding = false;
    std::array<bool, 2> axis_guiding{};
    bool aborted = false;
};

enum class AuxMotion : std::uint8_t {
    Idle,
    HomeRequested,
    Homing,
    StopRequested,
    Stopping,
    ControlRequested,
    SlewRequested,
    Slewing,
    GuideRequested,
    Guiding
};

struct SlewRequest {
    double ra;
    double dec;
    CgxPointingSide side;
    bool via_home = false;
};

struct HomeReference {
    std::array<std::uint32_t, 2> position;
    std::uint64_t session;
    std::array<std::uint32_t, 2> idle_position;
    std::array<std::uint32_t, 2> home_labels;
};

struct RetainedReference {
    HomeReference reference;
    std::string firmware;
};

struct ControlRequest {
    struct Failure {
        std::string message;
        int code;
    };
    double ra = 0;
    double dec = 0;
    std::optional<bool> tracking;
    std::promise<std::optional<Failure>> completion;
};

bool same_position(const std::array<std::uint32_t, 2>& a, const std::array<std::uint32_t, 2>& b) {
    return std::abs(signed_cgx_encoder_delta(a[0], b[0])) <= 2 && std::abs(signed_cgx_encoder_delta(a[1], b[1])) <= 2;
}

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
    std::string get_description() const override { return "Celestron SkyPortal AUX mount (home-reference profile)"; }
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
    bool get_can_find_home() const override { return true; }
    bool get_can_park() const override { return false; }
    bool get_can_unpark() const override { return false; }
    bool get_can_set_park() const override { return false; }
    bool get_can_pulse_guide() const override { return true; }
    bool get_can_set_guide_rates() const override { return false; }
    bool get_can_set_declination_rate() const override { return false; }
    bool get_can_set_right_ascension_rate() const override { return false; }
    bool get_can_set_tracking() const override { return true; }
    bool get_can_set_pier_side() const override { return false; }
    bool get_can_slew() const override { return false; }
    bool get_can_slew_async() const override { return true; }
    bool get_can_slew_alt_az() const override { return false; }
    bool get_can_slew_alt_az_async() const override { return false; }
    bool get_can_sync() const override { return true; }
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
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        if (motion_ != AuxMotion::Idle || tracking_) return false;
        const auto status = healthy_status_locked();
        require_motion_success_locked();
        return reference_ && reference_->session == link_.session_generation() && link_.is_healthy() &&
               same_position(status.position, reference_->position);
    }
    bool get_at_park() const override {
        healthy_status();
        return false;
    }
    bool get_tracking() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        healthy_status_locked();
        if (motion_ == AuxMotion::ControlRequested)
            throw AlpacaException("SkyPortal AUX tracking transition is pending", AlpacaError::InvalidOperation);
        return tracking_;
    }
    bool get_slewing() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        if (motion_ != AuxMotion::Idle && motion_ != AuxMotion::ControlRequested &&
            motion_ != AuxMotion::GuideRequested && motion_ != AuxMotion::Guiding)
            return true;
        const auto status = healthy_status_locked();
        require_motion_success_locked();
        return status.moving;
    }
    bool get_is_pulse_guiding() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto status = healthy_status_locked();
        require_motion_success_locked();
        return motion_ == AuxMotion::GuideRequested || motion_ == AuxMotion::Guiding || status.guiding;
    }
    double get_right_ascension() const override { return sky_position().right_ascension_hours; }
    double get_declination() const override { return sky_position().declination_degrees; }
    double get_altitude() const override { return horizontal_position().first; }
    double get_azimuth() const override { return horizontal_position().second; }
    int get_side_of_pier() const override { return sky_position().pointing_side == CgxPointingSide::Normal ? 0 : 1; }
    int get_destination_side_of_pier(double ra, double dec) const override {
        range(ra, 0, 24, "RightAscension");
        range(dec, -90, 90, "Declination");
        std::lock_guard<std::mutex> lock(mutex_);
        healthy_status_locked();
        const auto lst = cgx_local_sidereal_time_hours(std::chrono::system_clock::now(),
                                                       site_coordinate_locked(info_.site_longitude));
        return std::remainder(lst - ra, 24.0) >= 0 ? 0 : 1;
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
        return {15.0411 / 7200.0, 15.0411 / 7200.0};
    }
    void set_guide_rate(const GuideRate&) override {
        require_connected();
        unsupported("GuideRate");
    }
    void set_tracking(bool enabled) override {
        auto request = std::make_shared<ControlRequest>();
        request->tracking = enabled;
        auto completion = request->completion.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            healthy_status_locked(/*require_fresh=*/true);
            require_idle_locked();
            require_motion_success_locked();
            if (enabled && (!reference_ || reference_->session != link_.session_generation() || !link_.is_healthy()))
                uncalibrated();
            if (tracking_ == enabled) return;
            control_ = request;
            motion_ = AuxMotion::ControlRequested;
            snapshot_.write([](AuxStatus&) {});
        }
        if (const auto failure = completion.get()) throw AlpacaException(failure->message, failure->code);
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
        std::lock_guard<std::mutex> lock(mutex_);
        healthy_status_locked(/*require_fresh=*/true);
        require_idle_locked();
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
        return site_coordinate_locked(info_.site_latitude);
    }
    double get_site_longitude() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        return site_coordinate_locked(info_.site_longitude);
    }
    void set_site_latitude(double value) override {
        range(value, -90, 90, "SiteLatitude");
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        require_idle_locked();
        if (tracking_)
            throw AlpacaException("Switch Tracking off before changing SiteLatitude", AlpacaError::InvalidOperation);
        info_.site_latitude = value;
    }
    void set_site_longitude(double value) override {
        range(value, -180, 180, "SiteLongitude");
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        require_idle_locked();
        if (tracking_)
            throw AlpacaException("Switch Tracking off before changing SiteLongitude", AlpacaError::InvalidOperation);
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
        std::lock_guard<std::mutex> lock(mutex_);
        const auto status = healthy_status_locked(/*require_fresh=*/true);
        require_idle_locked();
        if (tracking_) throw AlpacaException("Switch Tracking off before FindHome", AlpacaError::InvalidOperation);
        if (reference_ && reference_->session == link_.session_generation() && link_.is_healthy() &&
            same_position(status.position, reference_->position))
            return;
        reference_.reset();
        motion_error_.clear();
        motion_ = AuxMotion::HomeRequested;
        snapshot_.write([](AuxStatus&) {});  // Discard a sample begun before this request.
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
    void abort_slew() override {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        if (motion_ == AuxMotion::StopRequested || motion_ == AuxMotion::Stopping) return;
        if (motion_ == AuxMotion::Idle) {
            healthy_status_locked(/*require_fresh=*/true);
            if (motion_error_.empty()) return;
        }
        finish_control_locked(std::make_exception_ptr(
            AlpacaException("SkyPortal AUX operation cancelled by AbortSlew", AlpacaError::InvalidOperation)));
        restore_tracking_after_stop_ = (motion_ == AuxMotion::SlewRequested || motion_ == AuxMotion::Slewing) &&
                                       motion_error_.empty() && !health_.faulted() && reference_ &&
                                       link_.is_healthy() && reference_->session == link_.session_generation();
        if (!restore_tracking_after_stop_) reference_.reset();
        motion_error_.clear();
        motion_ = AuxMotion::StopRequested;
        snapshot_.write([](AuxStatus&) {});
    }
    void move_axis(int axis, double rate) override {
        axis_range(axis);
        range(rate, -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), "Rate");
        require_connected();
        unsupported("MoveAxis");
    }
    void pulse_guide(int direction, int duration) override {
        if (direction < 0 || direction > 3 || duration < 0 || duration > 600000)
            throw AlpacaException("PulseGuide direction or duration is invalid", AlpacaError::InvalidValue);
        std::lock_guard<std::mutex> lock(mutex_);
        healthy_status_locked(/*require_fresh=*/true);
        require_idle_locked();
        require_motion_success_locked();
        if (!tracking_) throw AlpacaException("Enable Tracking before PulseGuide", AlpacaError::InvalidOperation);
        const auto sky = sky_position_locked(0);
        if (duration == 0) return;
        guide_axis_ = direction < 2 ? CgxAxis::Declination : CgxAxis::RightAscension;
        guide_velocity_ = direction == 1 || direction == 3 ? 50 : -50;
        if (direction < 2 && sky.pointing_side == CgxPointingSide::BeyondPole) guide_velocity_ = -guide_velocity_;
        guide_centiseconds_ = std::max(1, (duration + 5) / 10);
        motion_ = AuxMotion::GuideRequested;
        snapshot_.write([](AuxStatus&) {});
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
        std::lock_guard<std::mutex> lock(mutex_);
        healthy_status_locked(/*require_fresh=*/true);
        require_idle_locked();
        require_motion_success_locked();
        if (!tracking_)
            throw AlpacaException("Enable Tracking before an equatorial slew", AlpacaError::InvalidOperation);
        const auto lst = cgx_local_sidereal_time_hours(std::chrono::system_clock::now(),
                                                       site_coordinate_locked(info_.site_longitude));
        const auto current = sky_position_locked(lst);
        const auto side = std::remainder(lst - ra, 24.0) >= 0 ? CgxPointingSide::Normal : CgxPointingSide::BeyondPole;
        const SlewRequest request{ra == 24 ? 0 : ra, dec, side, side != current.pointing_side};
        if (!reference_) uncalibrated();
        static_cast<void>(slew_positions(request, *reference_, lst, true));
        slew_ = request;
        targets_ = {ra, dec};
        target_set_ = {true, true};
        motion_ = AuxMotion::SlewRequested;
        snapshot_.write([](AuxStatus&) {});
    }
    void sync_to_coordinates(double ra, double dec) override {
        range(ra, 0, 24, "RightAscension");
        range(dec, -90, 90, "Declination");
        auto request = std::make_shared<ControlRequest>();
        request->ra = ra;
        request->dec = dec;
        auto completion = request->completion.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            healthy_status_locked(/*require_fresh=*/true);
            require_idle_locked();
            require_motion_success_locked();
            if (!reference_ || reference_->session != link_.session_generation() || !link_.is_healthy()) uncalibrated();
            control_ = request;
            motion_ = AuxMotion::ControlRequested;
            snapshot_.write([](AuxStatus&) {});
        }
        // Transfer error data, not an exception object shared with the I/O worker.
        if (const auto failure = completion.get()) throw AlpacaException(failure->message, failure->code);
    }
    void slew_to_target_async() override {
        std::array<double, 2> targets{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            require_connected();
            if (!target_set_[0] || !target_set_[1])
                throw AlpacaException("Both target coordinates must be set", AlpacaError::InvalidOperation);
            targets = targets_;
        }
        slew_to_coordinates_async(targets[0], targets[1]);
    }
    void sync_to_target() override {
        std::array<double, 2> targets{};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            require_connected();
            if (!target_set_[0] || !target_set_[1])
                throw AlpacaException("Both target coordinates must be set", AlpacaError::InvalidOperation);
            targets = targets_;
        }
        sync_to_coordinates(targets[0], targets[1]);
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
    static double site_coordinate_locked(const std::optional<double>& coordinate) {
        if (!coordinate)
            throw AlpacaException("SkyPortal AUX site coordinates are unavailable", AlpacaError::DriverException);
        return *coordinate;
    }
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
        return healthy_status_locked();
    }
    CgxSkyPosition sky_position_locked(double lst) const {
        const auto status = healthy_status_locked();
        require_motion_success_locked();
        if (motion_ == AuxMotion::ControlRequested)
            throw AlpacaException("SkyPortal AUX coordinates are being synchronized", AlpacaError::InvalidOperation);
        if (!reference_ || reference_->session != link_.session_generation() || !link_.is_healthy()) uncalibrated();
        if (tracking_)
            lst = cgx_local_sidereal_time_hours(status.ra_sample_time, site_coordinate_locked(info_.site_longitude));
        // This profile admits only a northern CGX; changing reported site latitude cannot flip its mechanical frame.
        return cgx_encoder_position_to_sky(status.position[0], status.position[1], lst, CgxHemisphere::North,
                                           reference_->position[0], reference_->position[1]);
    }
    CgxSkyPosition sky_position() const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        const auto lst = cgx_local_sidereal_time_hours(std::chrono::system_clock::now(),
                                                       site_coordinate_locked(info_.site_longitude));
        return sky_position_locked(lst);
    }
    std::pair<double, double> horizontal_position() const {
        std::lock_guard<std::mutex> lock(mutex_);
        require_connected();
        const auto lst = cgx_local_sidereal_time_hours(std::chrono::system_clock::now(),
                                                       site_coordinate_locked(info_.site_longitude));
        const auto sky = sky_position_locked(lst);
        const double latitude = site_coordinate_locked(info_.site_latitude) * std::numbers::pi / 180.0;
        const double dec = sky.declination_degrees * std::numbers::pi / 180.0;
        const double ha = (lst - sky.right_ascension_hours) * std::numbers::pi / 12.0;
        const double up = std::sin(dec) * std::sin(latitude) + std::cos(dec) * std::cos(latitude) * std::cos(ha);
        const double east = -std::cos(dec) * std::sin(ha);
        const double north = std::sin(dec) * std::cos(latitude) - std::cos(dec) * std::sin(latitude) * std::cos(ha);
        const double azimuth = std::fmod(std::atan2(east, north) * 180.0 / std::numbers::pi + 360.0, 360.0);
        return {std::asin(std::clamp(up, -1.0, 1.0)) * 180.0 / std::numbers::pi, azimuth};
    }
    void require_idle_locked() const {
        if (motion_ != AuxMotion::Idle)
            throw AlpacaException("Wait for SkyPortal AUX motion to finish; overlapping guide pulses are not supported",
                                  AlpacaError::InvalidOperation);
    }
    void require_motion_success_locked() const {
        if (!motion_error_.empty()) throw AlpacaException(motion_error_, AlpacaError::DriverException);
    }
    AuxStatus healthy_status_locked(bool require_fresh = false) const {
        require_connected();
        if (health_.faulted())
            throw AlpacaException("SkyPortal AUX communications compromised: " + health_.fault(),
                                  AlpacaError::DriverException);
        const auto frame = snapshot_.read();
        if (!frame)
            throw AlpacaException("SkyPortal AUX status telemetry is unavailable", AlpacaError::DriverException);
        if (require_fresh && frame->stale)
            throw AlpacaException("SkyPortal AUX status telemetry is stale", AlpacaError::DriverException);
        return frame->value;
    }
    AuxFrame query(CgxAxis axis, std::uint8_t command, std::vector<std::uint8_t> payload = {}) {
        return link_.exchange(AuxFrame{0x20, static_cast<std::uint8_t>(axis), command, std::move(payload)},
                              response_budget());
    }
    std::chrono::milliseconds response_budget() const {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(io_deadline_ - clock_.now());
        if (remaining <= 0ms) throw AuxOperationDeadline();
        return std::min(remaining, info_.response_timeout);
    }
    AuxStatus sample();
    void sample_axis_motion(AuxStatus& status, std::size_t axis);
    void sample_guiding(AuxStatus& status);
    void finish_sample(AuxStatus& status);
    void check_reports();
    void stop_axes();
    AuxStatus takeover();
    std::optional<AuxStatus> continuity_checkpoint(const std::array<std::uint32_t, 2>& expected);
    std::string identify();
    void poll_status(util::OperationContext& context);
    void run_control(util::OperationContext& context, const std::shared_ptr<ControlRequest>& request);
    void run_slew(util::OperationContext& context);
    void run_guide(util::OperationContext& context);
    std::array<std::uint32_t, 2> slew_positions(const SlewRequest& request, const HomeReference& reference, double lst,
                                                bool approach) const;
    void write_tracking(bool enabled, bool declination_stopped = false);
    void finish_control_locked(const std::exception_ptr& failure = nullptr) {
        if (!control_) return;
        auto request = std::move(control_);
        std::optional<ControlRequest::Failure> result;
        if (failure) {
            try {
                std::rethrow_exception(failure);
            } catch (const AlpacaException& error) {
                result = ControlRequest::Failure{error.what(), error.error_code()};
            } catch (const std::exception& error) {
                result = ControlRequest::Failure{error.what(), AlpacaError::DriverException};
            } catch (...) {
                result =
                    ControlRequest::Failure{"Unknown SkyPortal AUX operation failure", AlpacaError::DriverException};
            }
        }
        request->completion.set_value(std::move(result));
    }
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
    AuxMotion motion_ = AuxMotion::Idle;
    CgxAxis guide_axis_ = CgxAxis::RightAscension;
    int guide_velocity_ = 50;
    int guide_centiseconds_ = 0;
    std::optional<HomeReference> reference_;
    std::optional<RetainedReference> retained_reference_;
    bool disconnect_io_fault_ = false;
    std::string motion_error_;
    std::shared_ptr<ControlRequest> control_;
    bool tracking_ = false;
    bool restore_tracking_after_stop_ = false;
    SlewRequest slew_{};
    std::array<std::uint8_t, 2> approach_{};
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
        if (versions[i].payload.size() != 4)
            throw AlpacaException(
                "SkyPortal AUX requires four-byte motor firmware replies on both axes; legacy or mixed profiles are "
                "not qualified",
                AlpacaError::DriverException);
        // TODO: qualify other firmware profiles before relying on guide-active queries there.
        const auto& version = versions[i].payload;
        if (version[0] * 256 + version[1] < 6 * 256 + 50)
            throw AlpacaException("SkyPortal AUX takeover requires motor firmware 6.50 or newer",
                                  AlpacaError::DriverException);
    }
    if (versions[0].payload != versions[1].payload)
        throw AlpacaException(
            "SkyPortal AUX requires matching motor firmware on both axes; mismatched profiles are not qualified",
            AlpacaError::DriverException);
    // USB identity evidence: docs/celestron-cgx-usb-identity.md. SkyPortal remains unqualified.
    for (const auto axis : axes) {
        const auto model = query(axis, 0x05);
        if (model.payload != std::vector<std::uint8_t>{0x17, 0x88})
            throw AlpacaException(
                "SkyPortal AUX requires CGX motor model 0x1788 on both axes; CGX-L and other identities are not "
                "qualified",
                AlpacaError::DriverException);
    }
    for (std::size_t i = 0; i < axes.size(); ++i) {
        const auto approach = query(axes[i], 0xfc);
        if (approach.payload.size() != 1 || approach.payload[0] > 1)
            throw AlpacaException("SkyPortal AUX controller approach direction is invalid",
                                  AlpacaError::DriverException);
        approach_[i] = approach.payload[0];
    }
    const auto format_version = [](const auto& bytes) {
        return std::to_string(bytes[0]) + "." + std::to_string(bytes[1]) + "." +
               std::to_string((static_cast<std::uint16_t>(bytes[2]) << 8) | bytes[3]);
    };
    return "RA " + format_version(versions[0].payload) + ", Dec " + format_version(versions[1].payload);
}

AuxStatus CelestronNexStarAuxTelescopeDriver::sample() {
    AuxStatus status;
    for (std::size_t i = 0; i < axes.size(); ++i) sample_axis_motion(status, i);
    sample_guiding(status);
    finish_sample(status);
    return status;
}

void CelestronNexStarAuxTelescopeDriver::sample_axis_motion(AuxStatus& status, std::size_t axis) {
    const auto done = query(axes[axis], 0x13);
    if (done.payload.size() != 1 || (done.payload[0] != 0 && done.payload[0] != 0xff && done.payload[0] != 0xfe))
        throw AlpacaException("SkyPortal AUX slew status is invalid", AlpacaError::DriverException);
    status.axis_moving[axis] = done.payload[0] == 0;
    status.moving = status.moving || status.axis_moving[axis];
    status.aborted = status.aborted || done.payload[0] == 0xfe;
}

void CelestronNexStarAuxTelescopeDriver::sample_guiding(AuxStatus& status) {
    for (std::size_t i = 0; i < axes.size(); ++i) {
        status.axis_guiding[i] = link_.is_aux_guide_active(axes[i], response_budget());
        status.guiding = status.axis_guiding[i] || status.guiding;
    }
}

void CelestronNexStarAuxTelescopeDriver::finish_sample(AuxStatus& status) {
    // Read positions after completion flags, with the time-sensitive RA sample last.
    for (int i = 1; i >= 0; --i) status.position[i] = link_.get_axis_position(axes[i], response_budget());
    status.ra_sample_time = std::chrono::system_clock::now();
    check_reports();
}

void CelestronNexStarAuxTelescopeDriver::check_reports() {
    for (const auto& report : link_.take_controller_reports()) {
        if (report.command == 0x50 && report.payload[0] == 0) {
            ALPACA_LOG_WARN("Celestron AUX", "Motor controller reports a low battery");
        } else {
            throw AlpacaException(
                "SkyPortal AUX controller reports a motion limit or error; disable competing bus masters and reconnect",
                AlpacaError::DriverException);
        }
    }
}

void CelestronNexStarAuxTelescopeDriver::stop_axes() {
    std::exception_ptr failure;
    for (const auto axis : axes) {
        // MC_MOVE_POS(0) can leave the sidereal rate running; clear that register explicitly too.
        for (const std::uint8_t command : {0x24, 0x06}) {
            try {
                if (!query(axis, command, std::vector<std::uint8_t>(command == 0x06 ? 3 : 1, 0)).payload.empty())
                    throw AlpacaException("SkyPortal AUX stop acknowledgement is invalid",
                                          AlpacaError::DriverException);
            } catch (...) {
                if (!failure) failure = std::current_exception();
            }
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
}

AuxStatus CelestronNexStarAuxTelescopeDriver::takeover() {
    stop_axes();
    // A native packet can drain for 2.55 s, then needs two stationary samples 500 ms apart.
    const auto deadline = std::min(clock_.now() + 4s, io_deadline_);
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

std::optional<AuxStatus> CelestronNexStarAuxTelescopeDriver::continuity_checkpoint(
    const std::array<std::uint32_t, 2>& expected) {
    io_deadline_ = std::min(io_deadline_, clock_.now() + 1s);
    util::ConsecutiveSettle settling(3, 4);
    std::optional<std::array<std::uint32_t, 2>> previous;
    while (true) {
        const auto current = sample();
        if (current.moving || current.guiding || current.aborted || !same_position(current.position, expected))
            return std::nullopt;
        const auto decision = settling.feed(previous && same_position(current.position, *previous));
        if (decision == util::ConsecutiveSettle::State::Settled) return current;
        if (decision == util::ConsecutiveSettle::State::TimedOut) return std::nullopt;
        previous = current.position;
    }
}

void CelestronNexStarAuxTelescopeDriver::set_connected(bool value) {
    std::lock_guard<std::mutex> transition(transition_mutex_);
    std::optional<RetainedReference> retained;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!value && record_disconnect_if_connect_in_flight(connected_.load())) return;
        if (value && consume_pending_disconnect(connected_.load())) return;
        if (value && connected_ && !health_.faulted()) return;
        if (!value && !connected_ && retained_reference_ && link_.is_healthy() &&
            retained_reference_->reference.session == link_.session_generation())
            return;
        if (value) {
            retained = std::exchange(retained_reference_, std::nullopt);
        } else {
            if (connected_ && motion_ == AuxMotion::Idle && motion_error_.empty() && !health_.faulted() && reference_ &&
                link_.is_healthy() && reference_->session == link_.session_generation()) {
                std::lock_guard<std::mutex> firmware_lock(firmware_mutex_);
                if (firmware_) retained = RetainedReference{*reference_, *firmware_};
            }
            retained_reference_.reset();
        }
        clear_firmware();
        disconnect_io_fault_ = false;
        connected_ = false;
        finish_control_locked(std::make_exception_ptr(
            AlpacaException("SkyPortal AUX Sync cancelled by connection change", AlpacaError::NotConnected)));
        snapshot_.reset();
        target_set_ = {};
        reference_.reset();
        motion_ = AuxMotion::Idle;
        motion_error_.clear();
        tracking_ = false;
        restore_tracking_after_stop_ = false;
        if (value) health_.reset();
    }
    poll_.cancel_all_and_join();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (disconnect_io_fault_) retained.reset();
    }
    clear_firmware();
    io_deadline_ = clock_.now() + 4s;
    if (!value) {
        std::exception_ptr failure;
        std::string stop_error;
        try {
            if (link_.is_connected()) {
                const auto stopped = takeover();
                if (retained) {
                    const auto checked = continuity_checkpoint(stopped.position);
                    if (checked && link_.is_healthy() && retained->reference.session == link_.session_generation())
                        retained->reference.idle_position = checked->position;
                    else
                        retained.reset();
                }
            } else {
                retained.reset();
            }
        } catch (const std::exception& error) {
            failure = std::current_exception();
            stop_error = error.what();
        } catch (...) {
            failure = std::current_exception();
            stop_error = "unknown stop verification failure";
        }
        if (failure) retained.reset();
        if (!retained) link_.disconnect();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            retained_reference_ = std::move(retained);
            health_.reset();
            // AsyncConnectable logs teardown exceptions rather than keeping a
            // Connect error. Keep unverified stopping visible in management too.
            if (failure) health_.note_failure("Stop verification failed: " + stop_error, 1, clock_.now());
        }
        if (failure) std::rethrow_exception(failure);
        return;
    }
    if (!retained || !link_.is_healthy() || retained->reference.session != link_.session_generation()) {
        retained.reset();
        link_.disconnect();
    }
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
        if (!retained) link_.connect_tcp(config.host, config.tcp_port, 500ms);
        const auto firmware = identify();
        const auto connect_deadline = io_deadline_;
        std::optional<AuxStatus> stopped;
        if (retained && retained->firmware == firmware)
            stopped = continuity_checkpoint(retained->reference.idle_position);
        if (!stopped) {
            retained.reset();
            io_deadline_ = connect_deadline;
            stopped = takeover();
        }
        static_cast<void>(
            lost_at_.consume());  // Approved policy always stops on relink; no surviving motion is resumed.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (consume_pending_disconnect(false)) {
                link_.disconnect();
                return;
            }
            health_.reset();
            if (retained) {
                if (!link_.is_healthy() || retained->reference.session != link_.session_generation())
                    throw AlpacaException("SkyPortal AUX logical reconnect lost its reference session",
                                          AlpacaError::DriverException);
                reference_ = retained->reference;
            }
            snapshot_.publish(snapshot_.begin_poll(), *stopped);
            connected_ = true;
            std::lock_guard<std::mutex> firmware_lock(firmware_mutex_);
            firmware_ = firmware;
        }
        poll_.start([this](util::OperationContext& context) { poll_status(context); });
    } catch (...) {
        clear_firmware();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connected_ = false;
            finish_control_locked(std::current_exception());
            reference_.reset();
            retained_reference_.reset();
            motion_ = AuxMotion::Idle;
            snapshot_.reset();
        }
        link_.disconnect();
        throw;
    }
}

void CelestronNexStarAuxTelescopeDriver::lose_link_locked() {
    retained_reference_.reset();
    restore_tracking_after_stop_ = false;
    lost_at_.note(clock_.now());
    clear_firmware();
    connected_ = false;
    finish_control_locked(std::make_exception_ptr(
        AlpacaException("SkyPortal AUX connection lost during Sync", AlpacaError::NotConnected)));
    snapshot_.reset();
    target_set_ = {};
    reference_.reset();
    motion_ = AuxMotion::Idle;
}

void CelestronNexStarAuxTelescopeDriver::write_tracking(bool enabled, bool declination_stopped) {
    const auto write = [&](CgxAxis axis, std::vector<std::uint8_t> payload) {
        if (!query(axis, 0x06, std::move(payload)).payload.empty())
            throw AlpacaException("SkyPortal AUX tracking acknowledgement is invalid", AlpacaError::DriverException);
    };
    // The admitted mechanical frame is northern, independently of later site edits.
    if (enabled) {
        if (!declination_stopped) write(CgxAxis::Declination, {0, 0, 0});
        write(CgxAxis::RightAscension, {0xff, 0xff});
    } else {
        write(CgxAxis::RightAscension, {0, 0, 0});
        write(CgxAxis::Declination, {0, 0, 0});
    }
}

std::array<std::uint32_t, 2> CelestronNexStarAuxTelescopeDriver::slew_positions(const SlewRequest& request,
                                                                                const HomeReference& reference,
                                                                                double lst, bool approach) const {
    const auto side =
        std::remainder(lst - request.ra, 24.0) >= 0 ? CgxPointingSide::Normal : CgxPointingSide::BeyondPole;
    if (side != request.side)
        throw AlpacaException("SkyPortal AUX target crossed the unqualified meridian route",
                              AlpacaError::InvalidOperation);
    const auto target = cgx_sky_position_to_encoders(request.ra, request.dec, lst, CgxHemisphere::North, request.side,
                                                     reference.position[0], reference.position[1]);
    std::array positions{target.first, target.second};
    if (approach) {
        const auto offset = encode_cgx_encoder_degrees(1.0);
        for (std::size_t i = 0; i < axes.size(); ++i)
            positions[i] = (positions[i] + (approach_[i] == 0 ? (1U << 24) - offset : offset)) & 0xffffff;
    }
    const auto sky = cgx_encoder_position_to_sky(positions[0], positions[1], lst, CgxHemisphere::North,
                                                 reference.position[0], reference.position[1]);
    const auto approach_side = std::remainder(lst - sky.right_ascension_hours, 24.0) >= 0 ? CgxPointingSide::Normal
                                                                                          : CgxPointingSide::BeyondPole;
    if (sky.pointing_side != request.side || approach_side != request.side)
        throw AlpacaException("SkyPortal AUX target or one-degree approach crosses an unqualified pointing branch",
                              AlpacaError::InvalidOperation);
    return positions;
}

void CelestronNexStarAuxTelescopeDriver::run_slew(util::OperationContext& context) {
    SlewRequest request{};
    HomeReference reference{};
    double longitude;
    int settle;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (motion_ != AuxMotion::SlewRequested || !connected_ || context.stop_reason() != util::StopReason::None)
            return;
        request = slew_;
        if (!reference_) uncalibrated();
        reference = *reference_;
        longitude = site_coordinate_locked(info_.site_longitude);
        settle = settle_seconds_;
        motion_ = AuxMotion::Slewing;
    }
    const auto deadline = clock_.now() + 180s;
    const auto owned = [&] {
        std::lock_guard<std::mutex> lock(mutex_);
        return connected_ && motion_ == AuxMotion::Slewing && context.stop_reason() == util::StopReason::None;
    };
    const auto budget = [&] {
        io_deadline_ = std::min(deadline, clock_.now() + 3s);
        if (clock_.now() >= deadline) throw AuxSlewFailure("SkyPortal AUX slew exceeded its overall deadline");
    };
    const auto publish = [&](const AuxStatus& current) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (motion_ == AuxMotion::Slewing && connected_) snapshot_.publish(snapshot_.begin_poll(), current);
    };
    const auto wait_axes = [&](std::chrono::milliseconds cadence, const std::array<bool, 2>& selected,
                               bool restore_tracking) -> std::optional<AuxStatus> {
        std::optional<AuxStatus> previous;
        std::array last_progress{clock_.now(), clock_.now()};
        auto next_telemetry = clock_.now() + 250ms;
        while (owned()) {
            budget();
            AuxStatus current;
            for (std::size_t i = 0; i < axes.size(); ++i)
                if (selected[i]) sample_axis_motion(current, i);
            check_reports();
            if (current.aborted)
                throw AlpacaException("SkyPortal AUX slew was aborted by the controller or interrupted by guiding",
                                      AlpacaError::DriverException);
            // Poll only commanded-axis completion on the short path; keep full telemetry for stall/guide checks.
            if (!current.moving || clock_.now() >= next_telemetry) {
                sample_guiding(current);
                check_reports();
                if (current.guiding)
                    throw AlpacaException("SkyPortal AUX slew was interrupted by guiding",
                                          AlpacaError::DriverException);
                if (!current.moving && restore_tracking) {
                    if (!owned()) return std::nullopt;
                    // Resume before encoder reads so their round trips do not add stationary RA drift.
                    write_tracking(true, true);
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (motion_ != AuxMotion::Slewing || !connected_) return std::nullopt;
                    tracking_ = true;
                }
                finish_sample(current);
                for (std::size_t i = 0; i < axes.size(); ++i) {
                    if (!previous || current.position[i] != previous->position[i]) last_progress[i] = clock_.now();
                    if (current.axis_moving[i] && clock_.now() - last_progress[i] >= 10s)
                        throw AuxSlewFailure("SkyPortal AUX slew stalled on " + std::string(i == 0 ? "RA" : "Dec"));
                }
                publish(current);
                if (!current.moving) return current;
                previous = current;
                next_telemetry = clock_.now() + 250ms;
            }
            if (!context.wait_for(cadence)) return std::nullopt;
        }
        return std::nullopt;
    };
    const auto send_positions = [&](const std::array<std::uint32_t, 2>& target, bool fast,
                                    const std::array<bool, 2>& selected) {
        for (std::size_t i = 0; i < axes.size(); ++i) {
            if (!owned()) return false;
            if (!selected[i]) continue;
            budget();
            if (fast)
                link_.goto_axis_fast(axes[i], target[i], response_budget());
            else
                link_.goto_axis_slow(axes[i], target[i], response_budget());
            check_reports();
        }
        return owned();
    };
    const auto positions_now = [&](bool approach) {
        return slew_positions(request, reference,
                              cgx_local_sidereal_time_hours(std::chrono::system_clock::now(), longitude), approach);
    };
    const auto arrival_errors = [&](const AuxStatus& current) {
        const auto sky = cgx_encoder_position_to_sky(
            current.position[0], current.position[1], cgx_local_sidereal_time_hours(current.ra_sample_time, longitude),
            CgxHemisphere::North, reference.position[0], reference.position[1]);
        return std::array{std::abs(std::remainder(sky.right_ascension_hours - request.ra, 24.0)) * 15.0,
                          std::abs(sky.declination_degrees - request.dec)};
    };
    if (!owned()) return;
    budget();
    write_tracking(false);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (motion_ != AuxMotion::Slewing || !connected_) return;
        tracking_ = false;
    }
    if (request.via_home) {
        // Home can relabel the encoders; no reference is trustworthy until both labels are restored.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (motion_ != AuxMotion::Slewing || !connected_) return;
            reference_.reset();
        }
        budget();
        auto previous = sample();
        std::array<std::uint64_t, 2> travel{};
        std::array last_progress{clock_.now(), clock_.now()};
        for (const auto axis : axes) {
            if (!owned()) return;
            budget();
            link_.start_axis_home(axis, response_budget());
            check_reports();
        }
        util::ConsecutiveSettle home_settle(3, 720);
        while (owned()) {
            budget();
            bool done = true;
            for (const auto axis : axes) {
                const bool axis_done = link_.is_axis_home_done(axis, response_budget());
                done = done && axis_done;
            }
            check_reports();
            const auto current = sample();
            if (current.aborted || current.guiding)
                throw AlpacaException("SkyPortal AUX meridian home route was interrupted",
                                      AlpacaError::DriverException);
            for (std::size_t i = 0; i < axes.size(); ++i) {
                const auto delta = std::abs(signed_cgx_encoder_delta(current.position[i], previous.position[i]));
                travel[i] += delta;
                if (delta != 0) last_progress[i] = clock_.now();
                if (travel[i] >= (1ULL << 23))
                    throw AuxSlewFailure("SkyPortal AUX meridian home route exceeded 180 degrees of travel");
                if (current.axis_moving[i] && clock_.now() - last_progress[i] >= 10s)
                    throw AuxSlewFailure("SkyPortal AUX meridian home route stalled");
            }
            const auto decision =
                home_settle.feed(done && !current.moving && same_position(current.position, previous.position));
            previous = current;
            publish(current);
            if (decision == util::ConsecutiveSettle::State::Settled) break;
            if (decision == util::ConsecutiveSettle::State::TimedOut)
                throw AuxSlewFailure("SkyPortal AUX meridian home route did not settle");
            if (!context.wait_for(250ms)) return;
        }
        for (std::size_t i = 0; i < axes.size(); ++i) {
            if (!owned()) return;
            budget();
            link_.set_axis_position(axes[i], reference.home_labels[i], response_budget());
            check_reports();
        }
        util::ConsecutiveSettle label_settle(3, 30);
        while (owned()) {
            budget();
            const auto current = sample();
            if (current.moving || current.guiding || current.aborted)
                throw AlpacaException("SkyPortal AUX meridian home labels changed during motion",
                                      AlpacaError::DriverException);
            const auto decision = label_settle.feed(same_position(current.position, reference.home_labels));
            if (decision == util::ConsecutiveSettle::State::Settled) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (motion_ != AuxMotion::Slewing || !connected_) return;
                if (!link_.is_healthy() || link_.session_generation() != reference.session)
                    throw AlpacaException("SkyPortal AUX meridian home lost its reference session",
                                          AlpacaError::DriverException);
                reference.idle_position = current.position;
                reference_ = reference;
                snapshot_.publish(snapshot_.begin_poll(), current);
                break;
            }
            if (decision == util::ConsecutiveSettle::State::TimedOut)
                throw AuxSlewFailure("SkyPortal AUX meridian home labels did not settle");
            if (!context.wait_for(50ms)) return;
        }
    }
    if (!send_positions(positions_now(true), true, {true, true}) || !wait_axes(250ms, {true, true}, false)) return;
    std::array<bool, 2> correction{true, true};
    AuxStatus arrived;
    bool verified = false;
    for (int round = 0; round < 3; ++round) {
        if (round > 0) {
            if (!owned()) return;
            budget();
            write_tracking(false);
            std::lock_guard<std::mutex> lock(mutex_);
            if (motion_ != AuxMotion::Slewing || !connected_) return;
            tracking_ = false;
        }
        if (!send_positions(positions_now(false), false, correction)) return;
        const auto current = wait_axes(20ms, correction, true);
        if (!current) return;
        arrived = *current;
        const auto errors = arrival_errors(arrived);
        ALPACA_LOG_DEBUG("Celestron AUX", "SLOW round=" + std::to_string(round + 1) +
                                              " RA_error_arcsec=" + std::to_string(errors[0] * 3600.0) +
                                              " Dec_error_arcsec=" + std::to_string(errors[1] * 3600.0) +
                                              " RA_count=" + std::to_string(arrived.position[0]) +
                                              " Dec_count=" + std::to_string(arrived.position[1]));
        correction = {errors[0] > 3.0 / 3600.0, errors[1] > 3.0 / 3600.0};
        if (!correction[0] && !correction[1]) {
            verified = true;
            break;
        }
        if (errors[0] > 1.0 || errors[1] > 1.0)
            throw AuxSlewFailure("SkyPortal AUX slew is outside the one-degree correction bound");
    }
    if (!verified) throw AuxSlewFailure("SkyPortal AUX slew exhausted three slow-approach/correction rounds");
    if (!owned()) return;
    const auto settle_end = clock_.now() + std::chrono::seconds(settle);
    do {
        if (!owned()) return;
        budget();
        arrived = sample();
        if (arrived.moving || arrived.guiding || arrived.aborted)
            throw AlpacaException("SkyPortal AUX unexpected motion during slew settling", AlpacaError::DriverException);
        publish(arrived);
        if (clock_.now() >= settle_end) break;
        if (!context.wait_for(250ms)) return;
    } while (true);
    const auto final_errors = arrival_errors(arrived);
    if (final_errors[0] > 3.0 / 3600.0 || final_errors[1] > 3.0 / 3600.0)
        throw AuxSlewFailure(
            "SkyPortal AUX target drifted outside arrival tolerance during tracking restoration or settling");
    std::lock_guard<std::mutex> lock(mutex_);
    if (motion_ != AuxMotion::Slewing || !connected_ || context.stop_reason() != util::StopReason::None) return;
    if (!link_.is_healthy() || link_.session_generation() != reference.session)
        throw AlpacaException("SkyPortal AUX slew lost its reference session", AlpacaError::DriverException);
    reference.idle_position = arrived.position;
    reference_ = reference;
    motion_ = AuxMotion::Idle;
}

void CelestronNexStarAuxTelescopeDriver::run_guide(util::OperationContext& context) {
    CgxAxis axis;
    int velocity, remaining;
    HomeReference reference{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_ || motion_ != AuxMotion::GuideRequested || context.stop_reason() != util::StopReason::None)
            return;
        axis = guide_axis_;
        velocity = guide_velocity_;
        remaining = guide_centiseconds_;
        if (!reference_) uncalibrated();
        reference = *reference_;
        motion_ = AuxMotion::Guiding;
    }
    const auto owned = [&] {
        std::lock_guard<std::mutex> lock(mutex_);
        return connected_ && motion_ == AuxMotion::Guiding && context.stop_reason() == util::StopReason::None;
    };
    AuxStatus current;
    while (remaining > 0 && owned()) {
        const int chunk = std::min(remaining, 255);
        io_deadline_ = clock_.now() + 3s;
        link_.pulse_guide_axis(axis, velocity, chunk, response_budget());
        check_reports();
        remaining -= chunk;
        const auto earliest_done = clock_.now() + std::chrono::milliseconds(chunk * 10);
        const auto deadline = earliest_done + 1s;
        do {
            if (!owned()) return;  // Abort owns stopping; disconnect joins this worker before takeover.
            io_deadline_ = std::min(deadline, clock_.now() + 3s);
            current = sample();
            if (current.moving || current.axis_guiding[axis == CgxAxis::RightAscension ? 1 : 0])
                throw AlpacaException("SkyPortal AUX guide interrupted by uncommanded motion",
                                      AlpacaError::DriverException);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!connected_ || motion_ != AuxMotion::Guiding) return;
                snapshot_.publish(snapshot_.begin_poll(), current);
            }
            if (clock_.now() >= earliest_done && !current.guiding && !current.moving) break;
            if (clock_.now() >= deadline)
                throw AuxSlewFailure("SkyPortal AUX guide pulse did not finish its current segment");
            if (!context.wait_for(50ms)) return;
        } while (true);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || motion_ != AuxMotion::Guiding || context.stop_reason() != util::StopReason::None) return;
    if (!link_.is_healthy() || link_.session_generation() != reference.session)
        throw AlpacaException("SkyPortal AUX guide lost its reference session", AlpacaError::DriverException);
    reference.idle_position = current.position;
    reference_ = reference;
    motion_ = AuxMotion::Idle;
}

void CelestronNexStarAuxTelescopeDriver::run_control(util::OperationContext& context,
                                                     const std::shared_ptr<ControlRequest>& request) {
    bool write_started = false;
    bool pre_settle_refused = false;
    const auto require_owner_locked = [&] {
        if (!connected_ || control_ != request || motion_ != AuxMotion::ControlRequested ||
            context.stop_reason() != util::StopReason::None)
            throw AlpacaException("SkyPortal AUX Sync was cancelled", AlpacaError::InvalidOperation);
    };
    const auto require_owner = [&] {
        std::lock_guard<std::mutex> lock(mutex_);
        require_owner_locked();
    };
    try {
        HomeReference reference{};
        double longitude;
        bool restore_tracking;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            require_owner_locked();
            reference = *reference_;
            longitude = site_coordinate_locked(info_.site_longitude);
            restore_tracking = tracking_;
        }
        // TODO: qualify this complete WiFi transaction budget on hardware, not the USB per-read timing.
        io_deadline_ = clock_.now() + 5s;
        if (restore_tracking) {
            require_owner();
            write_started = true;
            write_tracking(false);
        }
        std::optional<std::array<std::uint32_t, 2>> previous;
        AuxStatus current;
        util::ConsecutiveSettle pre_settle(3, 30);
        while (true) {
            require_owner();
            current = sample();
            const bool stable =
                previous && same_position(current.position, *previous) && !current.moving && !current.guiding;
            previous = current.position;
            const auto decision = pre_settle.feed(stable);
            if (decision == util::ConsecutiveSettle::State::Settled) break;
            if (decision == util::ConsecutiveSettle::State::TimedOut || clock_.now() + 50ms >= io_deadline_) {
                pre_settle_refused = true;
                throw AlpacaException("SkyPortal AUX encoders did not settle before Sync",
                                      AlpacaError::DriverException);
            }
            if (!context.wait_for(50ms)) require_owner();
        }
        if (request->tracking.has_value()) {
            require_owner();
            if (*request->tracking) {
                write_started = true;
                write_tracking(true);
            }
            std::lock_guard<std::mutex> lock(mutex_);
            require_owner_locked();
            if (!link_.is_healthy() || link_.session_generation() != reference.session)
                throw AlpacaException("SkyPortal AUX tracking lost its transport session",
                                      AlpacaError::DriverException);
            reference.idle_position = current.position;
            reference_ = reference;
            tracking_ = *request->tracking;
            snapshot_.publish(snapshot_.begin_poll(), current);
            motion_ = AuxMotion::Idle;
            finish_control_locked();
            return;
        }
        const double lst = cgx_local_sidereal_time_hours(std::chrono::system_clock::now(), longitude);
        const auto sky =
            cgx_encoder_position_to_sky(current.position[0], current.position[1], lst, CgxHemisphere::North,
                                        reference.position[0], reference.position[1]);
        const auto labels = cgx_sky_position_to_encoders(request->ra == 24.0 ? 0.0 : request->ra, request->dec, lst,
                                                         CgxHemisphere::North, sky.pointing_side, reference.position[0],
                                                         reference.position[1]);
        const std::array expected{labels.first, labels.second};
        for (std::size_t i = 0; i < axes.size(); ++i)
            reference.home_labels[i] = (reference.home_labels[i] + expected[i] - current.position[i]) & 0xffffff;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            require_owner_locked();
            if (!link_.is_healthy() || link_.session_generation() != reference.session)
                throw AlpacaException("SkyPortal AUX Sync lost its transport session", AlpacaError::DriverException);
            write_started = true;  // A missing write ACK may still have changed the labels.
            reference_.reset();
        }
        for (std::size_t i = 0; i < axes.size(); ++i) {
            require_owner();
            link_.set_axis_position(axes[i], expected[i], response_budget());
            check_reports();
        }
        util::ConsecutiveSettle post_settle(3, 30);
        while (true) {
            require_owner();
            current = sample();
            const auto decision =
                post_settle.feed(same_position(current.position, expected) && !current.moving && !current.guiding);
            if (decision == util::ConsecutiveSettle::State::Settled) break;
            if (decision == util::ConsecutiveSettle::State::TimedOut || clock_.now() + 50ms >= io_deadline_)
                throw AlpacaException("SkyPortal AUX did not retain synchronized encoder labels",
                                      AlpacaError::DriverException);
            if (!context.wait_for(50ms)) require_owner();
        }
        require_owner();
        if (restore_tracking) write_tracking(true);
        std::lock_guard<std::mutex> lock(mutex_);
        require_owner_locked();
        if (!link_.is_healthy() || link_.session_generation() != reference.session)
            throw AlpacaException("SkyPortal AUX Sync lost its transport session", AlpacaError::DriverException);
        reference.idle_position = current.position;
        reference_ = reference;
        targets_ = {request->ra, request->dec};
        target_set_ = {true, true};
        snapshot_.publish(snapshot_.begin_poll(), current);
        motion_ = AuxMotion::Idle;
        finish_control_locked();
    } catch (const std::exception& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (control_ != request) return;  // Abort/disconnect already resolved the waiting client.
        const bool pre_write_deadline = !write_started && dynamic_cast<const AuxOperationDeadline*>(&error) != nullptr;
        if (write_started || (!pre_settle_refused && !pre_write_deadline) || !link_.is_healthy()) {
            reference_.reset();
            snapshot_.reset();
            health_.note_failure(error.what(), 1, clock_.now());
            for (const auto axis : axes) {
                try {
                    link_.send_stop(axis, 250ms);
                } catch (const std::exception& stop_error) {
                    ALPACA_LOG_ERROR("Celestron AUX", stop_error.what());
                }
            }
        }
        motion_ = AuxMotion::Idle;
        finish_control_locked(std::current_exception());
    }
}

void CelestronNexStarAuxTelescopeDriver::poll_status(util::OperationContext& context) {
    auto next_idle_poll = clock_.now() + 1s;
    auto deadline = clock_.now();
    std::uint64_t home_session = 0;
    std::string last_home_sample = "no completed home sample";
    std::optional<std::array<std::uint32_t, 2>> previous_position;
    std::array<std::uint32_t, 2> home_previous{};
    std::array<std::uint64_t, 2> home_travel{};
    util::ConsecutiveSettle settling(3, 240);
    // One slot owns all connected I/O: queued initiators never join a slow poll.
    while (context.wait_for(250ms)) {
        if (!connected_) return;
        AuxMotion phase;
        bool tracking;
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
            phase = motion_;
            tracking = tracking_;
        }
        try {
            if (phase == AuxMotion::GuideRequested) {
                run_guide(context);
                continue;
            }
            if (phase == AuxMotion::SlewRequested) {
                run_slew(context);
                continue;
            }
            if (phase == AuxMotion::ControlRequested) {
                std::shared_ptr<ControlRequest> request;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    request = control_;
                }
                if (request) run_control(context, request);
                continue;
            }
            if (phase == AuxMotion::HomeRequested || phase == AuxMotion::StopRequested) {
                const bool home = phase == AuxMotion::HomeRequested;
                deadline = clock_.now() + (home ? 60s : 4s);
                io_deadline_ = std::min(deadline, clock_.now() + 3s);
                previous_position.reset();
                settling = util::ConsecutiveSettle(3, home ? 240 : 16);
                if (home) {
                    last_home_sample = "no completed home sample";
                    home_session = link_.session_generation();
                    home_previous = sample().position;
                    home_travel = {};
                    for (const auto axis : axes) {
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            if (!connected_ || context.stop_reason() != util::StopReason::None || motion_ != phase)
                                break;
                        }
                        link_.start_axis_home(axis, response_budget());
                        check_reports();
                    }
                } else {
                    stop_axes();
                }
                std::lock_guard<std::mutex> lock(mutex_);
                if (!connected_ || context.stop_reason() != util::StopReason::None) return;
                if (motion_ == phase) motion_ = home ? AuxMotion::Homing : AuxMotion::Stopping;
                continue;
            }
            if (phase == AuxMotion::Idle && clock_.now() < next_idle_poll) continue;
            if (phase != AuxMotion::Idle && clock_.now() >= deadline) {
                if (phase == AuxMotion::Stopping)
                    throw AlpacaException("SkyPortal AUX could not verify both axes stopped",
                                          AlpacaError::DriverException);
                std::lock_guard<std::mutex> lock(mutex_);
                if (motion_ == phase) {
                    motion_error_ = "SkyPortal AUX FindHome timed out; both axes are being stopped; last sample: " +
                                    last_home_sample;
                    ALPACA_LOG_ERROR("Celestron AUX", motion_error_);
                    motion_ = AuxMotion::StopRequested;
                    restore_tracking_after_stop_ = false;
                    reference_.reset();
                }
                continue;
            }
            io_deadline_ = clock_.now() + 3s;
            if (phase != AuxMotion::Idle) io_deadline_ = std::min(io_deadline_, deadline);
            const auto token = snapshot_.begin_poll();
            auto current = sample();
            bool home_done = true;
            std::array<bool, 2> axis_home_done{};
            if (phase == AuxMotion::Homing) {
                for (std::size_t i = 0; i < axes.size(); ++i) {
                    axis_home_done[i] = link_.is_axis_home_done(axes[i], response_budget());
                    home_done = home_done && axis_home_done[i];
                }
                check_reports();
                if (current.aborted)
                    throw AlpacaException("SkyPortal AUX controller aborted FindHome", AlpacaError::DriverException);
            }
            std::unique_lock<std::mutex> lock(mutex_);
            if (!connected_ || context.stop_reason() != util::StopReason::None) return;
            if (motion_ != phase) continue;
            if (phase == AuxMotion::Homing) {
                for (std::size_t i = 0; i < axes.size(); ++i) {
                    home_travel[i] += std::abs(signed_cgx_encoder_delta(current.position[i], home_previous[i]));
                    home_previous[i] = current.position[i];
                    // This initial WiFi profile permits only a near-home seek, not a full-axis search.
                    if (home_travel[i] >= 16777216ULL / 24) {
                        motion_error_ = "SkyPortal AUX FindHome exceeded 15 degrees of " +
                                        std::string(i == 0 ? "RA" : "Dec") +
                                        " travel; both axes are being stopped. Start near the home position.";
                        ALPACA_LOG_ERROR("Celestron AUX", motion_error_);
                        reference_.reset();
                        motion_ = AuxMotion::StopRequested;
                        restore_tracking_after_stop_ = false;
                        break;
                    }
                }
                if (motion_ != phase) continue;
            }
            const auto previous = snapshot_.read();
            if (phase == AuxMotion::Idle) {
                const bool displaced =
                    tracking_ && reference_
                        ? std::abs(signed_cgx_encoder_delta(current.position[1], reference_->idle_position[1])) > 2
                    : reference_ ? !same_position(current.position, reference_->idle_position)
                                 : previous && current.position != previous->value.position;
                if (current.moving || current.guiding || displaced)
                    throw AlpacaException(
                        "Uncommanded axis motion detected; disable HC tracking and competing apps before reconnecting",
                        AlpacaError::DriverException);
            } else {
                const bool stable = previous_position && same_position(current.position, *previous_position);
                if (phase == AuxMotion::Homing) {
                    last_home_sample = "RA position=" + std::to_string(current.position[0]) +
                                       " home_done=" + std::to_string(axis_home_done[0]) +
                                       "; Dec position=" + std::to_string(current.position[1]) +
                                       " home_done=" + std::to_string(axis_home_done[1]) +
                                       "; moving=" + std::to_string(current.moving) +
                                       " guiding=" + std::to_string(current.guiding) +
                                       " stable=" + std::to_string(stable);
                    ALPACA_LOG_DEBUG("Celestron AUX", "FindHome sample: " + last_home_sample);
                }
                previous_position = current.position;
                const auto decision = settling.feed(home_done && !current.moving && !current.guiding && stable);
                if (decision == util::ConsecutiveSettle::State::Settled) {
                    if (phase == AuxMotion::Homing) {
                        if (!link_.is_healthy() || link_.session_generation() != home_session)
                            throw AlpacaException("SkyPortal AUX home reference lost its transport session",
                                                  AlpacaError::DriverException);
                        reference_ = HomeReference{current.position, home_session, current.position, current.position};
                    }
                    tracking_ = false;
                    if (phase == AuxMotion::Stopping && restore_tracking_after_stop_) {
                        if (!reference_ || !link_.is_healthy() || reference_->session != link_.session_generation())
                            throw AlpacaException("SkyPortal AUX abort lost its tracking reference",
                                                  AlpacaError::DriverException);
                        lock.unlock();
                        write_tracking(true, true);
                        current = sample();
                        lock.lock();
                        if (!connected_ || context.stop_reason() != util::StopReason::None) return;
                        if (motion_ != phase) continue;
                        if (current.moving || current.guiding || current.aborted)
                            throw AlpacaException("SkyPortal AUX abort tracking restoration did not settle",
                                                  AlpacaError::DriverException);
                        reference_->idle_position = current.position;
                        tracking_ = true;
                    }
                    restore_tracking_after_stop_ = false;
                    motion_ = AuxMotion::Idle;
                } else if (decision == util::ConsecutiveSettle::State::TimedOut) {
                    // The next tick applies the same timeout/stop policy as the wall-clock bound.
                    deadline = clock_.now();
                }
            }
            health_.on_reply();
            snapshot_.publish(token, current);
            next_idle_poll = clock_.now() + 1s;
        } catch (const std::exception& error) {
            if ((phase == AuxMotion::SlewRequested || phase == AuxMotion::GuideRequested) &&
                dynamic_cast<const AuxSlewFailure*>(&error) && link_.is_healthy()) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!connected_ || context.stop_reason() != util::StopReason::None) return;
                if (motion_ == AuxMotion::Slewing || motion_ == AuxMotion::Guiding) {
                    // Preserve the operation failure while the normal stop owner verifies physical stopping.
                    motion_error_ = error.what();
                    ALPACA_LOG_ERROR("Celestron AUX", motion_error_);
                    reference_.reset();
                    motion_ = AuxMotion::StopRequested;
                    restore_tracking_after_stop_ = false;
                    snapshot_.write([](AuxStatus&) {});
                }
                continue;
            }
            if (phase != AuxMotion::Idle || tracking) {
                for (const auto axis : axes) {
                    try {
                        link_.send_stop(axis, 250ms);
                    } catch (const std::exception& stop_error) {
                        ALPACA_LOG_ERROR("Celestron AUX", stop_error.what());
                    }
                }
            }
            // A faulted/poisoned stream cannot recover without the client's reconnect.
            std::lock_guard<std::mutex> lock(mutex_);
            if (!connected_) {
                disconnect_io_fault_ = true;
                ALPACA_LOG_ERROR("Celestron AUX", error.what());
                return;
            }
            reference_.reset();
            restore_tracking_after_stop_ = false;
            motion_ = AuxMotion::Idle;
            if (health_.note_failure(error.what(), 1, clock_.now())) ALPACA_LOG_ERROR("Celestron AUX", error.what());
            // Record the fault before waking the waiting Sync client.
            finish_control_locked(std::current_exception());
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
