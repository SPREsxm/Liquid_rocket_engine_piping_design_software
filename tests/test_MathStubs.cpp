#include <catch2/catch_all.hpp>
#include "utils/MathStubs.h"

#include <cmath>

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

TEST_CASE("MathStubs::calculateReynoldsNumber laminar range", "[MathStubs]") {
    double re = MathStubs::calculateReynoldsNumber(0.1, 0.05, 1000.0, 0.001);
    REQUIRE(approx(re, 5000.0, 0.1));
}

TEST_CASE("MathStubs::calculateReynoldsNumber high velocity", "[MathStubs]") {
    double re = MathStubs::calculateReynoldsNumber(10.0, 0.05, 998.0, 0.001);
    REQUIRE(approx(re, 499000.0, 1.0));
}

TEST_CASE("MathStubs::calculateReynoldsNumber zero viscosity returns zero", "[MathStubs]") {
    double re = MathStubs::calculateReynoldsNumber(1.0, 0.05, 1000.0, 0.0);
    REQUIRE(re == 0.0);
}

TEST_CASE("MathStubs::calculateWaveSpeed Korteweg formula", "[MathStubs]") {
    double c = MathStubs::calculateWaveSpeed(2.2e9, 1000.0, 2.0e11, 0.1, 0.005);
    REQUIRE(c > 1000.0);
    REQUIRE(c < 2000.0);
}

TEST_CASE("MathStubs::calculateWaveSpeed zero wall thickness returns zero", "[MathStubs]") {
    double c = MathStubs::calculateWaveSpeed(2.2e9, 1000.0, 2.0e11, 0.1, 0.0);
    REQUIRE(c == 0.0);
}

TEST_CASE("MathStubs::calculatePressureDrop no longer returns stub zero", "[MathStubs]") {
    double dp = MathStubs::calculatePressureDrop(5.0, 0.02, 1e-6, 0.1, 998.0, 0.001);
    REQUIRE(dp > 0.0);
}

TEST_CASE("MathStubs::calculateFrictionFactor turbulent is computed", "[MathStubs]") {
    double f = MathStubs::calculateFrictionFactor(1e5, 4.5e-5, 0.05);
    REQUIRE(f > 0.01);
}

TEST_CASE("MathStubs::calculateFrictionFactor laminar returns 64/Re", "[MathStubs]") {
    double f = MathStubs::calculateFrictionFactor(1000.0, 4.5e-5, 0.05);
    REQUIRE(approx(f, 64.0 / 1000.0));
}

TEST_CASE("MathStubs::calculatePressureDrop zero diameter returns zero", "[MathStubs]") {
    double dp = MathStubs::calculatePressureDrop(5.0, 0.0, 1e-6, 0.1, 998.0, 0.001);
    REQUIRE(dp == 0.0);
}
