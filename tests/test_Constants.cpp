#include <catch2/catch_all.hpp>
#include "core/Constants.h"
#include "core/Types.h"
#include <QSet>

// ─── SemVer tests ────────────────────────────────────────

TEST_CASE("SemVer fromString parses simple version", "[Constants]") {
    auto v = AppConstants::SemVer::fromString("1.2.3");
    REQUIRE(v.major == 1);
    REQUIRE(v.minor == 2);
    REQUIRE(v.patch == 3);
    REQUIRE(v.preRelease.isEmpty());
}

TEST_CASE("SemVer fromString parses pre-release version", "[Constants]") {
    auto v = AppConstants::SemVer::fromString("2.0.1-beta");
    REQUIRE(v.major == 2);
    REQUIRE(v.minor == 0);
    REQUIRE(v.patch == 1);
    REQUIRE(v.preRelease == "beta");
}

TEST_CASE("SemVer fromString parses zero version", "[Constants]") {
    auto v = AppConstants::SemVer::fromString("0.0.0");
    REQUIRE(v.major == 0);
    REQUIRE(v.minor == 0);
    REQUIRE(v.patch == 0);
}

TEST_CASE("SemVer toString roundtrips", "[Constants]") {
    auto v = AppConstants::SemVer::fromString("3.14.2-rc1");
    REQUIRE(v.toString() == "3.14.2-rc1");
}

TEST_CASE("SemVer equal versions compare equal", "[Constants]") {
    auto a = AppConstants::SemVer::fromString("1.0.0");
    auto b = AppConstants::SemVer::fromString("1.0.0");
    REQUIRE(a == b);
    REQUIRE_FALSE(a < b);
    REQUIRE_FALSE(b < a);
}

TEST_CASE("SemVer major version difference", "[Constants]") {
    auto a = AppConstants::SemVer::fromString("1.0.0");
    auto b = AppConstants::SemVer::fromString("2.0.0");
    REQUIRE(a < b);
    REQUIRE_FALSE(b < a);
}

TEST_CASE("SemVer minor version difference", "[Constants]") {
    auto a = AppConstants::SemVer::fromString("1.0.0");
    auto b = AppConstants::SemVer::fromString("1.5.0");
    REQUIRE(a < b);
}

TEST_CASE("SemVer patch version difference", "[Constants]") {
    auto a = AppConstants::SemVer::fromString("1.0.0");
    auto b = AppConstants::SemVer::fromString("1.0.1");
    REQUIRE(a < b);
}

TEST_CASE("SemVer pre-release is less than release", "[Constants]") {
    auto a = AppConstants::SemVer::fromString("1.0.0-alpha");
    auto b = AppConstants::SemVer::fromString("1.0.0");
    REQUIRE(a < b);
    REQUIRE_FALSE(b < a);
}

TEST_CASE("isCompatibleVersion requires same major", "[Constants]") {
    auto pluginMin = AppConstants::SemVer::fromString("0.1.0");  // same major as app
    REQUIRE(AppConstants::isCompatibleVersion(pluginMin));
}

TEST_CASE("isCompatibleVersion rejects different major", "[Constants]") {
    auto pluginMin = AppConstants::SemVer::fromString("2.0.0");  // different major
    REQUIRE_FALSE(AppConstants::isCompatibleVersion(pluginMin));
}

TEST_CASE("APP_SEMVER matches APP_VERSION", "[Constants]") {
    REQUIRE(AppConstants::APP_SEMVER.toString() == AppConstants::APP_VERSION);
}

// ─── Constants tests ─────────────────────────────────────

TEST_CASE("AppConstants version is non-empty", "[Constants]") {
    REQUIRE_FALSE(AppConstants::APP_VERSION.isEmpty());
    REQUIRE_FALSE(AppConstants::APP_NAME.isEmpty());
    REQUIRE_FALSE(AppConstants::ORG_NAME.isEmpty());
}

TEST_CASE("Zoom constants are valid", "[Constants]") {
    REQUIRE(AppConstants::MIN_ZOOM > 0.0);
    REQUIRE(AppConstants::MAX_ZOOM > AppConstants::MIN_ZOOM);
    REQUIRE(AppConstants::ZOOM_STEP > 1.0);
}

TEST_CASE("MIME type constants are unique", "[Constants]") {
    REQUIRE(QString(AppConstants::MIME_COMPONENT_TYPE)
            != QString(AppConstants::MIME_BLOCK_CLIPBOARD));
}

// ─── Types tests ──────────────────────────────────────────

TEST_CASE("PropertyDescriptor default type is Double", "[Types]") {
    PropertyDescriptor pd;
    REQUIRE(pd.type == PropertyType::Double);
}

TEST_CASE("SolverSettings defaults are reasonable", "[Types]") {
    SolverSettings s;
    REQUIRE(s.tolerance > 0.0);
    REQUIRE(s.maxIterations > 0);
    REQUIRE(s.relaxationFactor > 0.0);
    REQUIRE(s.hardyCrossMaxIter > 0);
    REQUIRE(s.matrixSolverMaxIter > 0);
    REQUIRE(s.targetCourant > 0.0);
    REQUIRE(s.targetCourant <= 1.0);
    REQUIRE(s.gridBaseNodes > 0);
}

// ─── FluidType tests ───────────────────────────────────────────

TEST_CASE("fluidDefaults LOX returns cryogenic density", "[Constants]") {
    auto p = fluidDefaults(FluidType::LOX);
    REQUIRE(p.density > 1000.0);
    REQUIRE(p.density < 1200.0);
    REQUIRE(p.viscosity > 1e-5);
}

TEST_CASE("fluidDefaults RP1 returns kerosene-like density", "[Constants]") {
    auto p = fluidDefaults(FluidType::RP1);
    REQUIRE(p.density > 700.0);
    REQUIRE(p.density < 900.0);
}

TEST_CASE("fluidDefaults LH2 returns very low density", "[Constants]") {
    auto p = fluidDefaults(FluidType::LH2);
    REQUIRE(p.density < 100.0);
}

TEST_CASE("fluidDefaults Water is approximately 998 kg/m3", "[Constants]") {
    auto p = fluidDefaults(FluidType::Water);
    REQUIRE(p.density > 990.0);
    REQUIRE(p.density < 1010.0);
}

TEST_CASE("FluidType enum has 5 values", "[Constants]") {
    // Verify all enum values return distinct densities
    QSet<double> densities;
    densities.insert(fluidDefaults(FluidType::LOX).density);
    densities.insert(fluidDefaults(FluidType::RP1).density);
    densities.insert(fluidDefaults(FluidType::CH4).density);
    densities.insert(fluidDefaults(FluidType::LH2).density);
    densities.insert(fluidDefaults(FluidType::Water).density);
    REQUIRE(densities.size() == 5);
}

TEST_CASE("SolverSettings fromQSettings loads defaults", "[Constants]") {
    auto s = SolverSettings::fromQSettings();
    REQUIRE(s.tolerance > 0.0);
    REQUIRE(s.maxIterations > 0);
    REQUIRE(s.fluidDensity > 0.0);
    REQUIRE(s.fluidViscosity > 0.0);
}
