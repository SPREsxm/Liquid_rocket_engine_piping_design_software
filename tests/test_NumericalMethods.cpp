#include <catch2/catch_all.hpp>
#include "utils/NumericalMethods.h"
#include <cmath>
#include <vector>

using namespace NumericalMethods;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
    bool approxRel(double a, double b, double relTol) {
        return std::abs(a - b) < relTol * std::max(std::abs(b), 1.0);
    }
}

// ─── Central Difference ────────────────────────────────────

TEST_CASE("Central diff on f(x)=x^2 gives 2x", "[NumericalMethods]") {
    double x = 3.0, h = 0.001;
    double fm = (x - h) * (x - h);
    double fp = (x + h) * (x + h);
    double deriv = centralDiff(fm, fp, h);
    REQUIRE(approxRel(deriv, 2.0 * x, 1e-6));
}

TEST_CASE("Central diff 2 on f(x)=x^3 gives 6x", "[NumericalMethods]") {
    double x = 2.0, h = 0.001;
    double fm = (x - h) * (x - h) * (x - h);
    double f0 = x * x * x;
    double fp = (x + h) * (x + h) * (x + h);
    double d2 = centralDiff2(fm, f0, fp, h);
    REQUIRE(approxRel(d2, 6.0 * x, 1e-5));
}

// ─── Upwind Difference ─────────────────────────────────────

TEST_CASE("Upwind diff (positive sign) on f(x)=x gives 1", "[NumericalMethods]") {
    double x = 5.0, h = 0.001;
    double deriv = upwindDiff2(x, x - h, x - 2*h, h, 1);
    REQUIRE(approxRel(deriv, 1.0, 1e-4));
}

TEST_CASE("Upwind diff (negative sign) on f(x)=x gives 1", "[NumericalMethods]") {
    double x = 5.0, h = 0.001;
    // Negative sign: points to the right (upwind from opposite direction)
    double deriv = upwindDiff2(x, x + h, x + 2*h, h, -1);
    REQUIRE(approxRel(deriv, 1.0, 1e-4));
}

// ─── Adams-Bashforth 2 ─────────────────────────────────────

TEST_CASE("Adams-Bashforth 2 on exponential y'=y", "[NumericalMethods]") {
    double y0 = 1.0;           // y(0) = 1
    double dt = 0.01;
    double f0 = y0;            // f(0,y0) = y0
    double y1 = y0 + dt * f0;  // Euler step to start
    double f1 = y1;            // f(dt,y1) = y1
    double y2 = adamsBashforth2(y1, f1, f0, dt);
    double exact = std::exp(2.0 * dt);
    REQUIRE(approxRel(y2, exact, 1e-4));
}

// ─── Crank-Nicolson ────────────────────────────────────────

TEST_CASE("Crank-Nicolson unconditionally stable for λ<0", "[NumericalMethods]") {
    double yn = 1.0;
    double lambda = -100.0;
    double dt = 0.1;
    double yn1 = crankNicolsonLinear(yn, lambda, dt);
    REQUIRE(std::abs(yn1) < 1.0);  // stable, no blow-up
}

TEST_CASE("Crank-Nicolson exact for exponential growth", "[NumericalMethods]") {
    double yn = 1.0;
    double lambda = 1.0;
    double dt = 0.01;
    double yn1 = crankNicolsonLinear(yn, lambda, dt);
    // Exact: e^(λ*dt) ≈ 1 + λ*dt + 0.5*λ²*dt²
    double exact = std::exp(lambda * dt);
    REQUIRE(approxRel(yn1, exact, 1e-4));
}

// ─── Lax-Wendroff ──────────────────────────────────────────

TEST_CASE("Lax-Wendroff preserves Gaussian shape", "[NumericalMethods]") {
    int n = 100;
    std::vector<double> u(n, 0.0);
    // Gaussian pulse
    for (int j = 0; j < n; ++j) {
        double x = (j - n/2.0) / 10.0;
        u[j] = std::exp(-x * x);
    }
    std::vector<double> un = laxWendroffStep(u, 0.5, n);
    double sumOld = 0.0, sumNew = 0.0;
    for (int j = 0; j < n; ++j) {
        sumOld += u[j];
        sumNew += un[j];
    }
    // Mass conservation should be approximately preserved
    REQUIRE(approxRel(sumNew, sumOld, 0.01));
}

TEST_CASE("Lax-Wendroff constant field stays constant", "[NumericalMethods]") {
    int n = 50;
    std::vector<double> u(n, 1.5);
    auto un = laxWendroffStep(u, 0.8, n);
    for (int j = 0; j < n; ++j) {
        REQUIRE(approx(un[j], 1.5));
    }
}

// ─── Minmod ────────────────────────────────────────────────

TEST_CASE("Minmod opposite signs returns zero", "[NumericalMethods]") {
    REQUIRE(approx(minmod(1.0, -1.0), 0.0));
}

TEST_CASE("Minmod same sign returns smaller absolute", "[NumericalMethods]") {
    REQUIRE(approx(minmod(0.5, 1.0), 0.5));
    REQUIRE(approx(minmod(-0.5, -1.0), -0.5));
}

TEST_CASE("Minmod3 zero crossing returns zero", "[NumericalMethods]") {
    REQUIRE(approx(minmod3(1.0, -0.5, 2.0), 0.0));
}

// ─── MUSCL ─────────────────────────────────────────────────

TEST_CASE("MUSCL reconstruction for constant field", "[NumericalMethods]") {
    std::vector<double> u = {1.0, 1.0, 1.0, 1.0, 1.0};
    double uL, uR;
    musclReconstruct(u, 2, uL, uR);
    REQUIRE(approx(uL, 1.0));
    REQUIRE(approx(uR, 1.0));
}

TEST_CASE("MUSCL reconstruction for linear ramp", "[NumericalMethods]") {
    std::vector<double> u = {0.0, 1.0, 2.0, 3.0, 4.0};
    double uL, uR;
    musclReconstruct(u, 2, uL, uR);
    // Linear ramp: limited slope = 1.0, uL at j+1/2 = 2.5, uR = 2.5
    REQUIRE(uL > 2.0);
    REQUIRE(uR < 3.5);
}

// ─── RK2 Midpoint ──────────────────────────────────────────

TEST_CASE("RK2 on y'=y gives 2nd order accuracy", "[NumericalMethods]") {
    auto f = [](double, double y) { return y; };
    double y = 1.0;
    double dt = 0.01;
    double y1 = rk2Midpoint(0.0, y, dt, f);
    double exact = std::exp(dt);
    REQUIRE(approxRel(y1, exact, 1e-5));
}

// ─── Courant Number ────────────────────────────────────────

TEST_CASE("Courant number positive", "[NumericalMethods]") {
    double cfl = courantNumber(10.0, 0.01, 0.1);
    REQUIRE(cfl > 0.0);
    REQUIRE(approx(cfl, 1.0));
}

TEST_CASE("Courant number zero dx returns large value", "[NumericalMethods]") {
    double cfl = courantNumber(10.0, 0.01, 0.0);
    REQUIRE(cfl > 1e9);
}
