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

#include <alpacacore/vendor/celestron/celestron_cgx_geometry.h>

#include <cmath>

#include "catch2_compat.h"

using namespace alpacacore::vendor::celestron;

TEST_CASE("Celestron AUX geometry - independent branch and hemisphere fixtures", "[celestron][aux][geometry][unit]") {
    // Candidate home frame: HA=6h at the pole; 0x200000 counts is 45 degrees.
    // The two branches differ by 180 degrees in RA and opposite Dec travel.
    // These are analytical fixtures, not southern or SkyPortal motion qualification.
    struct Fixture {
        CgxHemisphere hemisphere;
        CgxPointingSide side;
        std::uint32_t ra;
        std::uint32_t dec;
        double lst;
        double sky_ra;
        double sky_dec;
        std::uint32_t ra_home = 0x400000;
        std::uint32_t dec_home = 0x800000;
    };
    const Fixture fixtures[] = {
        {CgxHemisphere::North, CgxPointingSide::Normal, 0x600000, 0xa00000, 3, 18, 45},
        {CgxHemisphere::North, CgxPointingSide::BeyondPole, 0xe00000, 0x600000, 3, 18, 45},
        {CgxHemisphere::North, CgxPointingSide::Normal, 0x200000, 0xe00000, 1, 22, -45},
        {CgxHemisphere::North, CgxPointingSide::BeyondPole, 0xa00000, 0x200000, 1, 22, -45},
        {CgxHemisphere::South, CgxPointingSide::Normal, 0x200000, 0xa00000, 3, 18, -45},
        {CgxHemisphere::South, CgxPointingSide::BeyondPole, 0xa00000, 0x600000, 3, 18, -45},
        {CgxHemisphere::South, CgxPointingSide::Normal, 0x600000, 0xe00000, 1, 22, 45},
        {CgxHemisphere::South, CgxPointingSide::BeyondPole, 0xe00000, 0x200000, 1, 22, 45},
        {CgxHemisphere::North, CgxPointingSide::Normal, 0x100000, 0x300000, 3, 18, 45, 0xf00000, 0x100000},
        {CgxHemisphere::North, CgxPointingSide::BeyondPole, 0x900000, 0xf00000, 3, 18, 45, 0xf00000, 0x100000},
        // The HC-homed northern rig reported count 1 on both axes; displacements remain analytical.
        {CgxHemisphere::North, CgxPointingSide::Normal, 0x200001, 0x200001, 3, 18, 45, 1, 1},
        {CgxHemisphere::North, CgxPointingSide::BeyondPole, 0xa00001, 0xe00001, 3, 18, 45, 1, 1},
    };
    for (const auto& f : fixtures) {
        CAPTURE(f.ra, f.dec, f.lst, f.sky_ra, f.sky_dec, f.ra_home, f.dec_home);
        const auto sky = cgx_encoder_position_to_sky(f.ra, f.dec, f.lst, f.hemisphere, f.ra_home, f.dec_home);
        CHECK(sky.right_ascension_hours == f.sky_ra);
        CHECK(sky.declination_degrees == f.sky_dec);
        CHECK(sky.pointing_side == f.side);
        // Inverse input is independently specified, never the forward result.
        const auto counts =
            cgx_sky_position_to_encoders(f.sky_ra, f.sky_dec, f.lst, f.hemisphere, f.side, f.ra_home, f.dec_home);
        CHECK(counts.first == f.ra);
        CHECK(counts.second == f.dec);
    }
}

TEST_CASE("Celestron AUX geometry - signed displacement and one-count pole boundaries",
          "[celestron][aux][geometry][unit]") {
    constexpr double count_degrees = 360.0 / 16777216.0;
    constexpr double count_hours = 24.0 / 16777216.0;
    for (const auto hemisphere : {CgxHemisphere::North, CgxHemisphere::South}) {
        const double h = hemisphere == CgxHemisphere::North ? 1.0 : -1.0;
        for (const auto side : {CgxPointingSide::Normal, CgxPointingSide::BeyondPole}) {
            const std::uint32_t dec = side == CgxPointingSide::Normal ? 0xa00000 : 0x600000;
            const auto before = cgx_encoder_position_to_sky(0x400000, dec, 10, hemisphere);
            const auto after = cgx_encoder_position_to_sky(0x400001, dec + 1, 10, hemisphere);
            CHECK(std::abs(std::remainder(after.right_ascension_hours - before.right_ascension_hours, 24.0) +
                           h * count_hours) < 1e-12);
            const double north_sign = side == CgxPointingSide::Normal ? -h : h;
            CHECK(std::abs(after.declination_degrees - before.declination_degrees - north_sign * count_degrees) <
                  1e-12);
            CHECK(after.pointing_side == side);
        }
        for (const int offset : {-1, 0, 1}) {
            const auto sky = cgx_encoder_position_to_sky(0x400000, 0x800000 + offset, 6, hemisphere);
            CHECK(std::abs(sky.declination_degrees - h * (90 - std::abs(offset) * count_degrees)) < 1e-12);
            CHECK(sky.pointing_side == (offset < 0 ? CgxPointingSide::BeyondPole : CgxPointingSide::Normal));
            CHECK(sky.right_ascension_hours == (offset < 0 ? 12.0 : 0.0));
        }
    }
}
