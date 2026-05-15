#include <catch2/catch_all.hpp>
#include "utils/FluidDynamics.h"
#include "utils/MathStubs.h"

#include <cmath>

using namespace FluidDynamics;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

TEST_CASE("Darcy-Weisbach laminar flow returns positive pressure drop", "[FluidDynamics]") {
    double dp = calculateDarcyWeisbachPressureDrop(5.0, 0.02, 1e-6, 0.1, 998.0, 0.001);
    REQUIRE(dp > 0.0);
    REQUIRE(dp < 100000.0);
}

TEST_CASE("Darcy-Weisbach zero diameter returns zero", "[FluidDynamics]") {
    double dp = calculateDarcyWeisbachPressureDrop(1.0, 0.0, 1e-6, 0.1, 998.0, 0.001);
    REQUIRE(dp == 0.0);
}

TEST_CASE("Colebrook-White laminar returns exact 64/Re", "[FluidDynamics]") {
    double lambda = calculateColebrookWhiteFrictionFactor(500.0, 1e-6, 0.05);
    REQUIRE(approx(lambda, 64.0 / 500.0));
}

TEST_CASE("Colebrook-White turbulent converges for typical values", "[FluidDynamics]") {
    double lambda = calculateColebrookWhiteFrictionFactor(100000.0, 4.5e-5, 0.05);
    REQUIRE(lambda > 0.01);
    REQUIRE(lambda < 0.05);
}

TEST_CASE("Colebrook-White high Reynolds converges", "[FluidDynamics]") {
    double lambda = calculateColebrookWhiteFrictionFactor(1e7, 1e-6, 0.1);
    REQUIRE(lambda > 0.005);
    REQUIRE(lambda < 0.02);
}

TEST_CASE("Local resistance loss basic formula", "[FluidDynamics]") {
    double dp = calculateLocalResistanceLoss(0.75, 10.0, 1000.0);
    REQUIRE(approxRel(dp, 37500.0, 0.001));
}

TEST_CASE("Local resistance loss zero velocity returns zero", "[FluidDynamics]") {
    double dp = calculateLocalResistanceLoss(1.0, 0.0, 1000.0);
    REQUIRE(dp == 0.0);
}

TEST_CASE("Joukowsky water-hammer correct formula", "[FluidDynamics]") {
    double dp = calculateJoukowskyWaterhammer(5.0, 1200.0, 1000.0);
    REQUIRE(approxRel(dp, 6000000.0, 0.001));
}

TEST_CASE("Joukowsky negative velocity returns positive pressure", "[FluidDynamics]") {
    double dp = calculateJoukowskyWaterhammer(-3.0, 1400.0, 998.0);
    REQUIRE(dp > 0.0);
    REQUIRE(approx(dp, 998.0 * 1400.0 * 3.0, 1.0));
}

TEST_CASE("Equivalent length calculation", "[FluidDynamics]") {
    double le = calculateEquivalentLength(0.05, 30.0);
    REQUIRE(approx(le, 1.5));
}

TEST_CASE("Propellant properties return valid values for LOX", "[FluidDynamics]") {
    auto props = propellantProperties(Propellant::LOX);
    REQUIRE(props.density > 500.0);
    REQUIRE(props.viscosity > 1e-6);
    REQUIRE(props.bulkModulus > 0.0);
}

TEST_CASE("Propellant properties for all types", "[FluidDynamics]") {
    auto check = [](Propellant p) {
        auto props = propellantProperties(p);
        REQUIRE(props.density > 0.0);
        REQUIRE(props.viscosity > 0.0);
        REQUIRE(props.bulkModulus > 0.0);
    };
    check(Propellant::LOX);
    check(Propellant::RP1);
    check(Propellant::Methane);
    check(Propellant::LH2);
    check(Propellant::Water);
}

TEST_CASE("Convenience pipe pressure drop with propellant type", "[FluidDynamics]") {
    double dp = calculatePipePressureDrop(2.0, 0.03, 1e-5, 0.5, Propellant::Water);
    REQUIRE(dp > 0.0);
}

TEST_CASE("MathStubs::calculatePressureDrop delegates to FluidDynamics", "[FluidDynamics]") {
    double dp1 = MathStubs::calculatePressureDrop(5.0, 0.02, 1e-6, 0.1, 998.0, 0.001);
    double dp2 = calculateDarcyWeisbachPressureDrop(5.0, 0.02, 1e-6, 0.1, 998.0, 0.001);
    REQUIRE(dp1 == dp2);
}

TEST_CASE("MathStubs::calculateFrictionFactor delegates to FluidDynamics", "[FluidDynamics]") {
    double f1 = MathStubs::calculateFrictionFactor(1e5, 4.5e-5, 0.05);
    double f2 = calculateColebrookWhiteFrictionFactor(1e5, 4.5e-5, 0.05);
    REQUIRE(f1 == f2);
}
