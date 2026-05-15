#include <catch2/catch_all.hpp>
#include "utils/SSTTurbulence.h"
#include <cmath>

using namespace SSTTurbulence;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── estimateKandOmega ─────────────────────────────────────

TEST_CASE("k and omega positive for nonzero velocity", "[SSTTurbulence]") {
    PipeFlowConditions fc{1000.0, 5.0, 0.1, 1e-3, 1e-5};
    double k, omega;
    estimateKandOmega(fc, k, omega);
    REQUIRE(k > 0.0);
    REQUIRE(omega > 0.0);
}

TEST_CASE("k zero for zero velocity", "[SSTTurbulence]") {
    PipeFlowConditions fc{1000.0, 0.0, 0.1, 1e-3, 1e-5};
    double k, omega;
    estimateKandOmega(fc, k, omega);
    REQUIRE(k == 0.0);
    REQUIRE(omega > 0.0);
}

// ─── Blending Functions ────────────────────────────────────

TEST_CASE("F1 blending function in [0,1]", "[SSTTurbulence]") {
    double F1 = blendingFunctionF1(1.0, 100.0, 0.05, 1e-6, 1000.0);
    REQUIRE(F1 >= 0.0);
    REQUIRE(F1 <= 1.0);
}

TEST_CASE("F2 blending function in [0,1]", "[SSTTurbulence]") {
    double F2 = blendingFunctionF2(1.0, 100.0, 0.05, 1e-6);
    REQUIRE(F2 >= 0.0);
    REQUIRE(F2 <= 1.0);
}

// ─── Eddy Viscosity SST ────────────────────────────────────

TEST_CASE("Eddy viscosity positive for turbulent flow", "[SSTTurbulence]") {
    double mu_t = eddyViscositySST(1.0, 100.0, 50.0, 1000.0);
    REQUIRE(mu_t > 0.0);
}

TEST_CASE("Eddy viscosity zero for zero k", "[SSTTurbulence]") {
    double mu_t = eddyViscositySST(0.0, 100.0, 50.0, 1000.0);
    REQUIRE(mu_t == 0.0);
}

// ─── computePipeTurbulence ─────────────────────────────────

TEST_CASE("Pipe turbulence laminar branch returns 64/Re", "[SSTTurbulence]") {
    // Re = U*D*rho/mu = 0.001*0.1*1000/0.001 = 100 < 2300 (laminar)
    PipeFlowConditions fc{1000.0, 0.001, 0.1, 1e-3, 0.001};
    auto r = computePipeTurbulence(fc);
    REQUIRE(r.eddyViscosity == 0.0);
    REQUIRE(approx(r.frictionFactor, 64.0 / 100.0));
}

TEST_CASE("Pipe turbulence turbulent branch returns positive eddy viscosity", "[SSTTurbulence]") {
    PipeFlowConditions fc{1000.0, 10.0, 0.1, 1e-5, 1e-5};
    auto r = computePipeTurbulence(fc);
    REQUIRE(r.eddyViscosity > 0.0);
    REQUIRE(r.frictionFactor > 0.0);
}

TEST_CASE("Pipe turbulence zero velocity returns zero result", "[SSTTurbulence]") {
    PipeFlowConditions fc{1000.0, 0.0, 0.1, 1e-5, 1e-5};
    auto r = computePipeTurbulence(fc);
    REQUIRE(r.effectiveViscosity == 0.0);
    REQUIRE(r.frictionFactor == 0.0);
}

// ─── effectiveFrictionFactorSST ────────────────────────────

TEST_CASE("SST friction factor convenience wrapper", "[SSTTurbulence]") {
    double f = effectiveFrictionFactorSST(100000.0, 1e-5, 0.1, 1000.0, 1e-3, 10.0);
    REQUIRE(f > 0.0);
    REQUIRE(f < 0.1);
}

// ─── Wall Function Helpers ─────────────────────────────────

TEST_CASE("Wall k estimate positive", "[SSTTurbulence]") {
    double k = wallKEstimate(0.5);
    REQUIRE(k > 0.0);
}

TEST_CASE("Wall omega estimate positive", "[SSTTurbulence]") {
    double omega = wallOmegaEstimate(0.5, 1e-6, 1.0);
    REQUIRE(omega > 0.0);
}

TEST_CASE("Wall omega zero viscosity returns large", "[SSTTurbulence]") {
    double omega = wallOmegaEstimate(0.5, 0.0, 1.0);
    REQUIRE(omega > 1e9);
}
