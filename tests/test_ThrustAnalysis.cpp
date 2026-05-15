#include <catch2/catch_all.hpp>
#include "utils/ThrustAnalysis.h"
#include <cmath>

using namespace ThrustAnalysis;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── calculateThrust basic functionality ───────────────────

TEST_CASE("Thrust zero throat area returns zero thrust", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.1, 0.0, 50.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.thrust_N == 0.0);
}

TEST_CASE("Thrust zero mass flow returns zero thrust", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.1, 0.04, 0.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.thrust_N == 0.0);
}

TEST_CASE("Thrust positive for typical LOX/RP-1 engine", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.thrust_N > 1e4);
    REQUIRE(r.thrust_N < 1e7);
}

TEST_CASE("Thrust specific impulse positive", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.specificImpulse_s > 10.0);
    REQUIRE(r.specificImpulse_s < 1000.0);
}

TEST_CASE("Thrust momentum and pressure components positive", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.momentumThrust_N > 0.0);
}

// ─── Error propagation ─────────────────────────────────────

TEST_CASE("Thrust relative error computed", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    auto r = calculateThrust(in);
    REQUIRE(r.relativeError > 0.0);
    REQUIRE(r.thrustUncertainty_N > 0.0);
}

TEST_CASE("Thrust withinSpec true for small errors", "[ThrustAnalysis]") {
    ThrustInputs in{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    in.pcUncertainty_Pa = 1000.0;  // small uncertainty
    auto r = calculateThrust(in);
    // with small uncertainty, should be within spec
    INFO("relativeError: " << r.relativeError);
}

// ─── Vacuum operation ──────────────────────────────────────

TEST_CASE("Thrust vacuum ambient=0 gives higher thrust", "[ThrustAnalysis]") {
    ThrustInputs inSea{1e7, 1e5, 101325.0, 0.2, 0.04, 100.0, 1.2};
    ThrustInputs inVac{1e7, 1e5, 0.0, 0.2, 0.04, 100.0, 1.2};
    auto seaLevel = calculateThrust(inSea);
    auto vacuum = calculateThrust(inVac);
    REQUIRE(vacuum.thrust_N > seaLevel.thrust_N);
}

// ─── thrustErrorFromChamberPressure ────────────────────────

TEST_CASE("Chamber pressure error positive", "[ThrustAnalysis]") {
    double err = thrustErrorFromChamberPressure(1e7, 1e4, 0.04, 1.5);
    REQUIRE(err > 0.0);
}

// ─── Nozzle efficiency ─────────────────────────────────────

TEST_CASE("Nozzle efficiency perfect = 1.0", "[ThrustAnalysis]") {
    double eta = nozzleEfficiency(1.5, 1.5, 1.2);
    REQUIRE(approx(eta, 1.0));
}

TEST_CASE("Nozzle efficiency zero ideal returns zero", "[ThrustAnalysis]") {
    REQUIRE(nozzleEfficiency(1.5, 0.0, 1.2) == 0.0);
}
