#include <catch2/catch_all.hpp>
#include "utils/HeatTransfer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace HeatTransfer;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Dittus-Boelter ───────────────────────────────────────

TEST_CASE("Dittus-Boelter heating returns positive Nu", "[HeatTransfer]") {
    FlowConditions fc{50000.0, 0.7, 0.6, 0.01, 1e-3, 8e-4};
    double nu = dittusBoelterNusselt(fc, true);
    REQUIRE(nu > 0.0);
}

TEST_CASE("Dittus-Boelter cooling Nu differs from heating (Pr<1)", "[HeatTransfer]") {
    FlowConditions fc{50000.0, 0.7, 0.6, 0.01, 1e-3, 8e-4};
    double nuHeat = dittusBoelterNusselt(fc, true);
    double nuCool = dittusBoelterNusselt(fc, false);
    // For Pr<1: Pr^0.4 < Pr^0.3 so nuCool > nuHeat. Both positive.
    REQUIRE(nuHeat > 0.0);
    REQUIRE(nuCool > 0.0);
    REQUIRE(nuHeat != nuCool);
}

TEST_CASE("Dittus-Boelter Re < 10000 returns zero", "[HeatTransfer]") {
    FlowConditions fc{5000.0, 0.7, 0.6, 0.01, 1e-3, 8e-4};
    REQUIRE(dittusBoelterNusselt(fc, true) == 0.0);
}

TEST_CASE("Dittus-Boelter zero Pr returns zero", "[HeatTransfer]") {
    FlowConditions fc{20000.0, 0.0, 0.6, 0.01, 1e-3, 8e-4};
    REQUIRE(dittusBoelterNusselt(fc, true) == 0.0);
}

// ─── Sieder-Tate ──────────────────────────────────────────

TEST_CASE("Sieder-Tate returns positive Nu for valid input", "[HeatTransfer]") {
    FlowConditions fc{50000.0, 5.0, 0.6, 0.01, 1e-3, 8e-4};
    double nu = siederTateNusselt(fc);
    REQUIRE(nu > 0.0);
}

TEST_CASE("Sieder-Tate viscosity ratio changes Nu", "[HeatTransfer]") {
    FlowConditions fcWarm{50000.0, 5.0, 0.6, 0.01, 8e-4, 1e-3};
    FlowConditions fcCool{50000.0, 5.0, 0.6, 0.01, 1e-3, 8e-4};
    double nuWarm = siederTateNusselt(fcWarm);
    double nuCool = siederTateNusselt(fcCool);
    // Viscosity ratio (μ_bulk/μ_wall)^0.14 affects Nu; both positive
    REQUIRE(nuWarm > 0.0);
    REQUIRE(nuCool > 0.0);
    REQUIRE(nuWarm != nuCool);
}

TEST_CASE("Sieder-Tate Re < 10000 returns zero", "[HeatTransfer]") {
    FlowConditions fc{5000.0, 5.0, 0.6, 0.01, 1e-3, 8e-4};
    REQUIRE(siederTateNusselt(fc) == 0.0);
}

// ─── Gnielinski ───────────────────────────────────────────

TEST_CASE("Gnielinski returns positive Nu for turbulent flow", "[HeatTransfer]") {
    FlowConditions fc{10000.0, 1.0, 0.6, 0.01, 1e-3, 8e-4};
    double nu = gnielinskiNusselt(fc, 0.03);
    REQUIRE(nu > 0.0);
}

TEST_CASE("Gnielinski Re < 2300 returns zero", "[HeatTransfer]") {
    FlowConditions fc{2000.0, 1.0, 0.6, 0.01, 1e-3, 8e-4};
    REQUIRE(gnielinskiNusselt(fc, 0.03) == 0.0);
}

TEST_CASE("Gnielinski zero Pr returns zero", "[HeatTransfer]") {
    FlowConditions fc{5000.0, 0.0, 0.6, 0.01, 1e-3, 8e-4};
    REQUIRE(gnielinskiNusselt(fc, 0.03) == 0.0);
}

// ─── Heat Transfer Coefficient ────────────────────────────

TEST_CASE("Heat transfer coefficient basic conversion", "[HeatTransfer]") {
    double h = heatTransferCoefficient(100.0, 0.6, 0.01);
    REQUIRE(approx(h, 6000.0));
}

TEST_CASE("Heat transfer coefficient zero diameter returns zero", "[HeatTransfer]") {
    REQUIRE(heatTransferCoefficient(100.0, 0.6, 0.0) == 0.0);
}

// ─── Newton Cooling ──────────────────────────────────────

TEST_CASE("Newton cooling heat flux sign convention", "[HeatTransfer]") {
    double q = newtonCoolingHeatFlux(100.0, 400.0, 300.0);
    REQUIRE(q > 0.0);
}

TEST_CASE("Newton cooling total heat calculation", "[HeatTransfer]") {
    double Q = newtonCoolingTotalHeat(100.0, 0.5, 400.0, 300.0);
    REQUIRE(approx(Q, 5000.0));
}

// ─── Bartz Correlation ────────────────────────────────────

TEST_CASE("Bartz hot gas coefficient positive for typical engine", "[HeatTransfer]") {
    double hg = bartzHotGasCoefficient(0.05, 8e-5, 2500.0, 0.7,
                                       1e7, 1800.0, 0.05, 0.1,
                                       M_PI*0.025*0.025, M_PI*0.025*0.025,
                                       800.0, 3500.0, 1.0, 1.2);
    REQUIRE(hg > 0.0);
    REQUIRE(hg < 1e6);
}

TEST_CASE("Bartz zero throat diameter returns zero", "[HeatTransfer]") {
    double hg = bartzHotGasCoefficient(0.0, 8e-5, 2500.0, 0.7,
                                       1e7, 1800.0, 0.05, 0.1,
                                       M_PI*0.025*0.025, M_PI*0.025*0.025,
                                       800.0, 3500.0, 1.0, 1.2);
    REQUIRE(hg == 0.0);
}

TEST_CASE("Bartz zero cstar returns zero", "[HeatTransfer]") {
    double hg = bartzHotGasCoefficient(0.05, 8e-5, 2500.0, 0.7,
                                       1e7, 0.0, 0.05, 0.1,
                                       M_PI*0.025*0.025, M_PI*0.025*0.025,
                                       800.0, 3500.0, 1.0, 1.2);
    REQUIRE(hg == 0.0);
}

TEST_CASE("Bartz throat convenience matches full formula at M=1", "[HeatTransfer]") {
    double Dt = 0.05;
    double At = M_PI * Dt * Dt / 4.0;
    double hg = bartzHotGasCoefficient(Dt, 8e-5, 2500.0, 0.7,
                                       1e7, 1800.0, Dt, 0.1, At, At,
                                       800.0, 3500.0, 1.0, 1.2);
    double ht = bartzThroatCoefficient(Dt, 8e-5, 2500.0, 0.7,
                                       1e7, 1800.0, 0.1, 800.0, 3500.0, 1.2);
    REQUIRE(approx(hg, ht));
}

// ─── Coolant Temperature Rise ─────────────────────────────

TEST_CASE("Coolant temperature rise for typical channel", "[HeatTransfer]") {
    double dT = coolantTemperatureRise(1e6, 0.01, 0.5, 4000.0, 0.1);
    REQUIRE(dT > 0.0);
    REQUIRE(approx(dT, 0.5));
}

TEST_CASE("Coolant temperature rise zero mass flow returns zero", "[HeatTransfer]") {
    double dT = coolantTemperatureRise(1e6, 0.01, 0.0, 4000.0, 0.1);
    REQUIRE(dT == 0.0);
}

TEST_CASE("Coolant temperature rise zero Cp returns zero", "[HeatTransfer]") {
    double dT = coolantTemperatureRise(1e6, 0.01, 0.5, 0.0, 0.1);
    REQUIRE(dT == 0.0);
}
