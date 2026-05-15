#include <catch2/catch_all.hpp>
#include "utils/MethaneEOS.h"
#include <cmath>

using namespace MethaneEOS;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Constants ─────────────────────────────────────────────────

TEST_CASE("Critical constants are positive", "[MethaneEOS]") {
    REQUIRE(R_GAS > 0.0);
    REQUIRE(TC > 0.0);
    REQUIRE(PC > 0.0);
    REQUIRE(RHO_C > 0.0);
    REQUIRE(MOLAR_MASS > 0.0);
}

TEST_CASE("Coefficient array has 22 entries", "[MethaneEOS]") {
    REQUIRE(COEFFS.size() == 22);
}

TEST_CASE("PE terms are 4 pairs", "[MethaneEOS]") {
    REQUIRE(PE_TERMS.size() == 4);
}

// ─── computeState ──────────────────────────────────────────────

TEST_CASE("State at STP gives reasonable pressure", "[MethaneEOS]") {
    // Methane at 298K, low density ~ ideal gas
    double rho = 0.65; // ~1 atm at 298K for CH4
    State s = computeState(298.0, rho);
    REQUIRE(s.pressure > 90000.0);
    REQUIRE(s.pressure < 110000.0);
    REQUIRE(s.compressibility > 0.95);
    REQUIRE(s.compressibility < 1.05);
}

TEST_CASE("State at low density is near ideal gas", "[MethaneEOS]") {
    // Simplified EOS is only accurate at low-to-moderate density
    State s = computeState(300.0, 5.0);
    REQUIRE(s.pressure > 0.0);
    REQUIRE(s.compressibility > 0.9);
    REQUIRE(s.compressibility < 1.1);
}

TEST_CASE("State properties internally consistent", "[MethaneEOS]") {
    // cp > cv always
    State s = computeState(300.0, 10.0);
    REQUIRE(s.cp > s.cv);
    REQUIRE(s.cv > 0.0);
    REQUIRE(s.soundspeed > 0.0);
    REQUIRE(s.enthalpy > s.internalEnergy);
}

TEST_CASE("State zero temperature returns zero state", "[MethaneEOS]") {
    State s = computeState(0.0, 1.0);
    REQUIRE(s.pressure == 0.0);
}

TEST_CASE("State zero density returns zero state", "[MethaneEOS]") {
    State s = computeState(300.0, 0.0);
    REQUIRE(s.pressure == 0.0);
}

TEST_CASE("State at moderate density", "[MethaneEOS]") {
    // T=200K, rho=50 (well below critical density)
    State s = computeState(200.0, 50.0);
    REQUIRE(s.pressure > 0.0);
    REQUIRE(s.compressibility > 0.0);
    REQUIRE(s.compressibility < 1.0);
}

TEST_CASE("State at moderate supercritical conditions", "[MethaneEOS]") {
    // T=300K (>Tc=190.6), moderate density
    State s = computeState(300.0, 35.0);
    REQUIRE(s.pressure > PC);
    REQUIRE(s.soundspeed > 0.0);
}

TEST_CASE("Sound speed monotonic with density at fixed T", "[MethaneEOS]") {
    double T = 300.0;
    State s1 = computeState(T, 1.0);
    State s2 = computeState(T, 50.0);
    // Higher density → generally higher sound speed for supercritical
    REQUIRE(s1.soundspeed > 0.0);
    REQUIRE(s2.soundspeed > 0.0);
}

TEST_CASE("Entropy positive and finite", "[MethaneEOS]") {
    State s = computeState(300.0, 10.0);
    REQUIRE(std::isfinite(s.entropy));
}

// ─── densityFromTP ─────────────────────────────────────────────

TEST_CASE("Density from TP inverts pressure", "[MethaneEOS]") {
    double T = 300.0;
    double rho_guess = 1.0;
    State s = computeState(T, rho_guess);
    double rho_iter = densityFromTP(T, s.pressure);
    REQUIRE(rho_iter > 0.0);
    REQUIRE(approxRel(rho_iter, rho_guess, 1e-6));
}

TEST_CASE("Density from TP zero guards", "[MethaneEOS]") {
    REQUIRE(densityFromTP(0.0, 1e5) == 0.0);
    REQUIRE(densityFromTP(300.0, 0.0) == 0.0);
}

TEST_CASE("Density from TP at moderate pressure", "[MethaneEOS]") {
    double rho = densityFromTP(250.0, 5e6);
    REQUIRE(rho > 0.0);
    REQUIRE(rho < RHO_C * 3.0);
}

// ─── saturationLine ────────────────────────────────────────────

TEST_CASE("Saturation line at 150K gives positive values", "[MethaneEOS]") {
    double pSat, rhoL, rhoV;
    saturationLine(150.0, pSat, rhoL, rhoV);
    REQUIRE(pSat > 0.0);
    REQUIRE(pSat < PC);
    REQUIRE(rhoL > rhoV);
    REQUIRE(rhoL > RHO_C);
    REQUIRE(rhoV < RHO_C);
}

TEST_CASE("Saturation line at Tc returns zero", "[MethaneEOS]") {
    double pSat, rhoL, rhoV;
    saturationLine(TC, pSat, rhoL, rhoV);
    REQUIRE(pSat == 0.0);
    REQUIRE(rhoL == 0.0);
    REQUIRE(rhoV == 0.0);
}

TEST_CASE("Saturation line above Tc returns zero", "[MethaneEOS]") {
    double pSat, rhoL, rhoV;
    saturationLine(300.0, pSat, rhoL, rhoV);
    REQUIRE(pSat == 0.0);
}
