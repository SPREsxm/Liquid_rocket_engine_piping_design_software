#include <catch2/catch_all.hpp>
#include "utils/FluidDynamics.h"
#include <cmath>

using namespace FluidDynamics::GasDynamics;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Speed Coefficient ─────────────────────────────────────

TEST_CASE("Speed coefficient λ(M=0) = 0", "[GasDynamics]") {
    REQUIRE(approx(speedCoefficient(0.0), 0.0));
}

TEST_CASE("Speed coefficient λ(M=1) = 1", "[GasDynamics]") {
    REQUIRE(approx(speedCoefficient(1.0), 1.0));
}

TEST_CASE("Speed coefficient λ increases with M", "[GasDynamics]") {
    REQUIRE(speedCoefficient(2.0) > speedCoefficient(1.0));
}

// ─── Mach from Speed Coefficient ───────────────────────────

TEST_CASE("Mach from λ roundtrip M=0.5", "[GasDynamics]") {
    double lambda = speedCoefficient(0.5);
    double M = machFromSpeedCoefficient(lambda);
    REQUIRE(approxRel(M, 0.5, 1e-6));
}

TEST_CASE("Mach from λ roundtrip M=2.0", "[GasDynamics]") {
    double lambda = speedCoefficient(2.0);
    double M = machFromSpeedCoefficient(lambda);
    REQUIRE(approxRel(M, 2.0, 1e-6));
}

// ─── Pressure Ratio ────────────────────────────────────────

TEST_CASE("Pressure ratio π(M=0) = 1.0", "[GasDynamics]") {
    REQUIRE(approx(pressureRatio(0.0), 1.0));
}

TEST_CASE("Pressure ratio π(M=1) close to critical", "[GasDynamics]") {
    double pi = pressureRatio(1.0, 1.4);
    REQUIRE(approxRel(pi, 0.52828, 0.01));
}

TEST_CASE("Pressure ratio decreases with M", "[GasDynamics]") {
    REQUIRE(pressureRatio(2.0) < pressureRatio(1.0));
}

// ─── Temperature Ratio ─────────────────────────────────────

TEST_CASE("Temperature ratio τ(M=0) = 1.0", "[GasDynamics]") {
    REQUIRE(approx(temperatureRatio(0.0), 1.0));
}

TEST_CASE("Temperature ratio τ(M=1) with γ=1.4", "[GasDynamics]") {
    REQUIRE(approxRel(temperatureRatio(1.0, 1.4), 0.83333, 0.01));
}

// ─── Density Ratio ─────────────────────────────────────────

TEST_CASE("Density ratio ε(M=0) = 1.0", "[GasDynamics]") {
    REQUIRE(approx(densityRatio(0.0), 1.0));
}

TEST_CASE("Density ratio decreases with M", "[GasDynamics]") {
    REQUIRE(densityRatio(2.0) < densityRatio(1.0));
}

// ─── Flow Function q(λ) ────────────────────────────────────

TEST_CASE("Flow function q(0) = 0", "[GasDynamics]") {
    REQUIRE(approx(flowFunction(0.0), 0.0));
}

TEST_CASE("Flow function q(1) = 1", "[GasDynamics]") {
    double q = flowFunction(1.0, 1.4);
    REQUIRE(approxRel(q, 1.0, 1e-6));
}

// ─── Area Ratio ────────────────────────────────────────────

TEST_CASE("Area ratio at M=1 approximately 1.0", "[GasDynamics]") {
    double a = areaRatio(1.0, 1.4);
    REQUIRE(approxRel(a, 1.0, 1e-6));
}

TEST_CASE("Area ratio high at very low M", "[GasDynamics]") {
    double a = areaRatio(0.1, 1.4);
    REQUIRE(a > 1.0);
}

// ─── Mach from Area Ratio ──────────────────────────────────

TEST_CASE("Mach from area ratio subsonic roundtrip", "[GasDynamics]") {
    double ar = areaRatio(0.5, 1.4);
    double M = machFromAreaRatio(ar, false, 1.4);
    REQUIRE(approxRel(M, 0.5, 1e-3));
}

TEST_CASE("Mach from area ratio supersonic roundtrip", "[GasDynamics]") {
    double ar = areaRatio(2.5, 1.4);
    double M = machFromAreaRatio(ar, true, 1.4);
    REQUIRE(approxRel(M, 2.5, 1e-3));
}

// ─── Critical Pressure Ratio ──────────────────────────────

TEST_CASE("Critical pressure ratio with γ=1.4", "[GasDynamics]") {
    REQUIRE(approxRel(criticalPressureRatio(1.4), 0.52828, 0.01));
}

// ─── Thrust Coefficient ────────────────────────────────────

TEST_CASE("Thrust coefficient positive for typical engine", "[GasDynamics]") {
    double Cf = thrustCoefficient(1e7, 1e5, 101325.0, 0.1, 0.04, 1.2);
    REQUIRE(Cf > 1.0);
    REQUIRE(Cf < 3.0);
}

TEST_CASE("Thrust coefficient zero chamber pressure returns zero", "[GasDynamics]") {
    REQUIRE(thrustCoefficient(0.0, 1e5, 101325.0, 0.1, 0.04) == 0.0);
}

// ─── Choked Mass Flow ──────────────────────────────────────

TEST_CASE("Choked mass flow positive for LOX engine", "[GasDynamics]") {
    double mdot = chokedMassFlow(1e7, 0.001, 3500.0, 1.2, 355.0);
    REQUIRE(mdot > 1.0);
    REQUIRE(mdot < 100.0);
}

TEST_CASE("Choked mass flow zero pressure returns zero", "[GasDynamics]") {
    REQUIRE(chokedMassFlow(0.0, 0.001, 3500.0) == 0.0);
}

TEST_CASE("Choked mass flow zero area returns zero", "[GasDynamics]") {
    REQUIRE(chokedMassFlow(1e7, 0.0, 3500.0) == 0.0);
}
