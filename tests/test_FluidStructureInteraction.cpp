#include <catch2/catch_all.hpp>
#include "utils/FluidStructureInteraction.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace FluidStructureInteraction;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Hoop Stress ───────────────────────────────────────────

TEST_CASE("Hoop stress Barlow formula", "[FluidStructureInteraction]") {
    double sigma = hoopStress(1e6, 0.1, 0.005);
    REQUIRE(approx(sigma, 1e7));
}

TEST_CASE("Hoop stress zero wall thickness returns large", "[FluidStructureInteraction]") {
    REQUIRE(hoopStress(1e6, 0.1, 0.0) > 1e20);
}

// ─── Longitudinal Stress ───────────────────────────────────

TEST_CASE("Longitudinal stress half of hoop", "[FluidStructureInteraction]") {
    double sigmaH = hoopStress(1e6, 0.1, 0.005);
    double sigmaL = longitudinalStress(1e6, 0.1, 0.005);
    REQUIRE(approxRel(sigmaH, 2.0 * sigmaL, 1e-6));
}

// ─── Von Mises ─────────────────────────────────────────────

TEST_CASE("Von Mises equal-biaxial equals sigma", "[FluidStructureInteraction]") {
    double vm = vonMisesPlaneStress(1e8, 1e8);
    REQUIRE(approx(vm, 1e8));
}

TEST_CASE("Von Mises positive for positive stresses", "[FluidStructureInteraction]") {
    double vm = vonMisesPlaneStress(2e8, 1e8);
    REQUIRE(vm > 0.0);
}

// ─── Safety Factor ─────────────────────────────────────────

TEST_CASE("Safety factor yield/VM ratio", "[FluidStructureInteraction]") {
    double sf = safetyFactorAgainstYield(3e8, 1e8);
    REQUIRE(approx(sf, 3.0));
}

TEST_CASE("Safety factor zero VM returns large", "[FluidStructureInteraction]") {
    REQUIRE(safetyFactorAgainstYield(3e8, 0.0) > 1e9);
}

// ─── Korteweg Wave Speed ───────────────────────────────────

TEST_CASE("Korteweg wave speed less than uncoupled", "[FluidStructureInteraction]") {
    double c0 = std::sqrt(2.2e9 / 1000.0); // uncoupled: ~1483 m/s
    double c = kortevegWaveSpeed(2.2e9, 1000.0, 2e11, 0.1, 0.005);
    REQUIRE(c > 0.0);
    REQUIRE(c < c0);
}

TEST_CASE("Korteweg zero fluid density returns zero", "[FluidStructureInteraction]") {
    REQUIRE(kortevegWaveSpeed(2.2e9, 0.0, 2e11, 0.1, 0.005) == 0.0);
}

// ─── Radial Deflection ─────────────────────────────────────

TEST_CASE("Radial deflection positive for internal pressure", "[FluidStructureInteraction]") {
    double d = radialDeflection(1e6, 0.1, 0.005, 2e11, 0.3);
    REQUIRE(d > 0.0);
}

TEST_CASE("Radial deflection zero modulus returns zero", "[FluidStructureInteraction]") {
    REQUIRE(radialDeflection(1e6, 0.1, 0.005, 0.0, 0.3) == 0.0);
}

// ─── Compute Stresses Integration ──────────────────────────

TEST_CASE("computeStresses for LOX pipe gives plausible results", "[FluidStructureInteraction]") {
    PipeMechanics pipe{0.1, 0.005, 2e11, 0.3, 3e8, 8000.0};
    FluidCoupling fluid{1e7, 9e8, 1140.0}; // LOX at ~90K
    auto r = computeStresses(pipe, fluid);

    REQUIRE(r.hoopStress_Pa > 0.0);
    REQUIRE(r.longitudinalStress_Pa > 0.0);
    REQUIRE(r.vonMisesStress_Pa > 0.0);
    REQUIRE(r.safetyFactor > 0.0);
    REQUIRE(r.kortevegWaveSpeed_mps > 0.0);
    REQUIRE(r.radialDeflection_m > 0.0);
}

TEST_CASE("computeStresses yield exceeded for high pressure", "[FluidStructureInteraction]") {
    PipeMechanics pipe{0.1, 0.001, 2e11, 0.3, 3e8, 8000.0};
    FluidCoupling fluid{5e7, 9e8, 1140.0};
    auto r = computeStresses(pipe, fluid);
    REQUIRE(r.yieldExceeded == (r.vonMisesStress_Pa > pipe.yieldStrength_Pa));
}

// ─── Material Database ─────────────────────────────────────

TEST_CASE("Material 316L has known properties", "[FluidStructureInteraction]") {
    auto m = material316L();
    REQUIRE(m.youngsModulus_Pa > 1e11);
    REQUIRE(m.yieldStrength_Pa > 1e8);
}

TEST_CASE("materialByName 316L returns 316L properties", "[FluidStructureInteraction]") {
    auto m = materialByName("316L");
    REQUIRE(approx(m.youngsModulus_Pa, 2e11));
}

TEST_CASE("materialByName Inconel returns Inconel properties", "[FluidStructureInteraction]") {
    auto m = materialByName("Inconel 718");
    REQUIRE(approx(m.yieldStrength_Pa, 1.1e9));
}

TEST_CASE("materialByName unknown returns 316L default", "[FluidStructureInteraction]") {
    auto m = materialByName("UnknownMaterial123");
    REQUIRE(approx(m.youngsModulus_Pa, material316L().youngsModulus_Pa));
}
