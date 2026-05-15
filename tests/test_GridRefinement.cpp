#include <catch2/catch_all.hpp>
#include "utils/GridRefinement.h"
#include <cmath>

using namespace GridRefinement;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── computeGCI ────────────────────────────────────────────────

TEST_CASE("GCI zero for perfectly converged solution", "[GridRefinement]") {
    double gci = computeGCI(100.0, 100.0, 100.0);
    REQUIRE(approx(gci, 0.0));
}

TEST_CASE("GCI positive for converging sequence", "[GridRefinement]") {
    // Coarse=100, Medium=99, Fine=98.5 → converging
    double gci = computeGCI(100.0, 99.0, 98.5);
    REQUIRE(gci > 0.0);
    REQUIRE(gci < 0.1);
}

TEST_CASE("GCI positive for diverging sequence", "[GridRefinement]") {
    double gci = computeGCI(100.0, 102.0, 105.0);
    REQUIRE(gci > 0.0);
}

TEST_CASE("GCI zero-order returns large value", "[GridRefinement]") {
    // r^p - 1 = 0 when p=0 → denominator zero
    double gci = computeGCI(100.0, 99.0, 98.5, 2.0, 0.0);
    REQUIRE(gci > 1e9);
}

TEST_CASE("GCI refinement ratio 1 returns large value", "[GridRefinement]") {
    double gci = computeGCI(100.0, 99.0, 98.5, 1.0);
    REQUIRE(gci > 1e9);
}

TEST_CASE("GCI higher order gives smaller GCI", "[GridRefinement]") {
    double gci2 = computeGCI(100.0, 99.0, 98.5, 2.0, 2.0);
    double gci4 = computeGCI(100.0, 99.0, 98.5, 2.0, 4.0);
    REQUIRE(gci4 < gci2);
}

TEST_CASE("GCI zero fine value safe denominator", "[GridRefinement]") {
    double gci = computeGCI(100.0, 99.0, 0.0);
    REQUIRE(std::isfinite(gci));
    REQUIRE(gci > 0.0);
}

// ─── richardsonExtrapolation ───────────────────────────────────

TEST_CASE("Richardson extrapolation constant field", "[GridRefinement]") {
    double fexact = richardsonExtrapolation(100.0, 100.0);
    REQUIRE(approx(fexact, 100.0));
}

TEST_CASE("Richardson extrapolation converging sequence", "[GridRefinement]") {
    // With p=2, r=2: fexact = (4*ffine - fmedium) / 3
    double fexact = richardsonExtrapolation(99.0, 98.5);
    REQUIRE(approx(fexact, (4.0 * 98.5 - 99.0) / 3.0));
}

TEST_CASE("Richardson extrapolation higher order", "[GridRefinement]") {
    double f4 = richardsonExtrapolation(99.0, 98.5, 2.0, 4.0);
    double f2 = richardsonExtrapolation(99.0, 98.5, 2.0, 2.0);
    REQUIRE(f4 > f2);
}

TEST_CASE("Richardson extrapolation refinement ratio 3", "[GridRefinement]") {
    // r=3, p=2: fexact = (9*ffine - fmedium) / 8
    double fexact = richardsonExtrapolation(99.0, 98.2, 3.0, 2.0);
    double expected = (9.0 * 98.2 - 99.0) / 8.0;
    REQUIRE(approx(fexact, expected));
}
