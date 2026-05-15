#include <catch2/catch_all.hpp>
#include "utils/PropellantProperties.h"
#include <cmath>

using namespace PropellantProperties;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Critical Data ────────────────────────────────────────

TEST_CASE("LOX critical constants match known values", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(approx(cd.Tc, 154.58));
    REQUIRE(approx(cd.Pc, 5.043e6, 1e3));
    REQUIRE(approx(cd.Mw, 32.0));
    REQUIRE(approx(cd.omega, 0.022));
}

TEST_CASE("Methane critical constants match known values", "[PropellantProperties]") {
    auto cd = criticalMethane();
    REQUIRE(approx(cd.Tc, 190.56));
    REQUIRE(approx(cd.Pc, 4.599e6, 1e3));
    REQUIRE(approx(cd.Mw, 16.04));
}

TEST_CASE("LH2 critical constants match known values", "[PropellantProperties]") {
    auto cd = criticalLH2();
    REQUIRE(approx(cd.Tc, 33.145));
    REQUIRE(approx(cd.Mw, 2.016));
    REQUIRE(approx(cd.omega, -0.219));
}

TEST_CASE("Water critical constants match known values", "[PropellantProperties]") {
    auto cd = criticalWater();
    REQUIRE(approx(cd.Tc, 647.14));
    REQUIRE(approx(cd.Tb, 373.15));
}

// ─── Wagner Vapor Pressure ────────────────────────────────

TEST_CASE("Wagner vapor pressure at Tb in ballpark of 1 atm", "[PropellantProperties]") {
    auto wc = wagnerLOX();
    double Psat = wagnerVaporPressure(90.19, wc);
    // Wagner coefficients are approximate; expect within factor of 3
    REQUIRE(Psat > 20000.0);
    REQUIRE(Psat < 300000.0);
}

TEST_CASE("Wagner at Tc returns zero", "[PropellantProperties]") {
    auto wc = wagnerLOX();
    REQUIRE(wagnerVaporPressure(154.58, wc) == 0.0);
}

TEST_CASE("Wagner monotonic with temperature", "[PropellantProperties]") {
    auto wc = wagnerLOX();
    double P1 = wagnerVaporPressure(100.0, wc);
    double P2 = wagnerVaporPressure(120.0, wc);
    REQUIRE(P2 > P1);
}

TEST_CASE("Wagner methane vapor pressure positive at Tb", "[PropellantProperties]") {
    auto wc = wagnerMethane();
    double Psat = wagnerVaporPressure(111.6, wc);
    // Approximate; within factor of 3 of atmospheric
    REQUIRE(Psat > 20000.0);
    REQUIRE(Psat < 300000.0);
}

TEST_CASE("Wagner water at 373K returns positive pressure", "[PropellantProperties]") {
    auto wc = wagnerWater();
    double Psat = wagnerVaporPressure(373.15, wc);
    REQUIRE(Psat > 0.0);
    REQUIRE(Psat < 500000.0);
}

// ─── Rackett Density ──────────────────────────────────────

TEST_CASE("Rackett saturated liquid density LOX at Tb plausible", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double rho = rackettDensity(90.19, cd);
    // LOX saturated liquid at 90K ≈ 1140 kg/m3; allow wide range
    REQUIRE(rho > 200.0);
    REQUIRE(rho < 3000.0);
}

TEST_CASE("Rackett at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(rackettDensity(154.58, cd) == 0.0);
}

TEST_CASE("Rackett density decreases with temperature", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double rho90 = rackettDensity(90.19, cd);
    double rho120 = rackettDensity(120.0, cd);
    REQUIRE(rho90 > rho120);
}

// ─── Yamada-Gunn Density ──────────────────────────────────

TEST_CASE("Yamada-Gunn density in plausible range", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double rho = yamadaGunnDensity(90.19, cd);
    REQUIRE(rho > 200.0);
    REQUIRE(rho < 3000.0);
}

TEST_CASE("Yamada-Gunn at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(yamadaGunnDensity(154.58, cd) == 0.0);
}

// ─── Daubert Density ──────────────────────────────────────

TEST_CASE("Daubert RP1 density approximate at room temp", "[PropellantProperties]") {
    double rho = daubertDensityRP1(298.0);
    // Daubert-Danner for RP-1 is approximate; expect 500-900 kg/m3
    REQUIRE(rho > 500.0);
    REQUIRE(rho < 950.0);
}

// ─── Joback Cp ────────────────────────────────────────────

TEST_CASE("Joback methane Cp in plausible range at 300K", "[PropellantProperties]") {
    double cp = methaneIdealGasCp(300.0);
    // Joback group contribution method is approximate
    REQUIRE(cp > 1000.0);
    REQUIRE(cp < 3000.0);
}

TEST_CASE("Joback Cp increases with temperature", "[PropellantProperties]") {
    double cp200 = methaneIdealGasCp(200.0);
    double cp400 = methaneIdealGasCp(400.0);
    REQUIRE(cp400 > cp200);
}

// ─── Squires Viscosity ────────────────────────────────────

TEST_CASE("Squires viscosity for LOX at Tb in plausible range", "[PropellantProperties]") {
    auto cd = criticalLOX();
    auto wc = wagnerLOX();
    double Psat = wagnerVaporPressure(90.19, wc);
    double eta = squiresViscosity(90.19, cd, Psat);
    REQUIRE(eta > 1e-5);
    REQUIRE(eta < 1e-2);
}

TEST_CASE("Squires at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    auto wc = wagnerLOX();
    double Psat = wagnerVaporPressure(150.0, wc);
    REQUIRE(squiresViscosity(154.58, cd, Psat) == 0.0);
}

// ─── Pitzer Heat of Vaporization ──────────────────────────

TEST_CASE("Pitzer LOX at Tb gives plausible Hvap", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double hvap = pitzerHeatOfVaporization(90.19, cd);
    // LOX real ΔHvap at Tb ≈ 213 kJ/kg; Pitzer estimate nearby
    REQUIRE(hvap > 100.0);
    REQUIRE(hvap < 500.0);
}

TEST_CASE("Pitzer at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(pitzerHeatOfVaporization(154.58, cd) == 0.0);
}

// ─── Nicola Thermal Conductivity ──────────────────────────

TEST_CASE("Nicola LOX thermal conductivity at Tb", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double lambda = nicolaThermalConductivity(90.19, cd);
    // Nicola correlation gives order-of-magnitude estimate
    REQUIRE(lambda > 0.01);
    REQUIRE(lambda < 5.0);
}

TEST_CASE("Nicola at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(nicolaThermalConductivity(154.58, cd) == 0.0);
}

// ─── Sastri-Rao Surface Tension ───────────────────────────

TEST_CASE("Sastri-Rao LOX surface tension at Tb", "[PropellantProperties]") {
    auto cd = criticalLOX();
    double sigma = sastriRaoSurfaceTension(90.19, cd);
    REQUIRE(sigma > 1e-4);
    REQUIRE(sigma < 1.0);
}

TEST_CASE("Sastri-Rao at Tc returns zero", "[PropellantProperties]") {
    auto cd = criticalLOX();
    REQUIRE(sastriRaoSurfaceTension(154.58, cd) == 0.0);
}

// ─── Composite Property Getter ────────────────────────────

TEST_CASE("computeAllProperties returns sensible values for LOX at Tb", "[PropellantProperties]") {
    auto cd = criticalLOX();
    auto wc = wagnerLOX();
    auto props = computeAllProperties(90.19, cd, wc);
    REQUIRE(props.density > 500.0);
    REQUIRE(props.viscosity > 1e-5);
    REQUIRE(props.vaporPressure > 0.0);
    REQUIRE(props.heatOfVaporization > 0.0);
    REQUIRE(props.thermalConductivity > 0.0);
    REQUIRE(props.surfaceTension > 0.0);
}
