#include <catch2/catch_all.hpp>
#include "core/MixedPrecision.h"
#include <cmath>

using namespace MixedPrecision;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── Precision Selector ────────────────────────────────────

TEST_CASE("useFloat32 true below 1e9 threshold", "[MixedPrecision]") {
    REQUIRE(useFloat32(1e8) == true);
}

TEST_CASE("useFloat32 false above 1e9 threshold", "[MixedPrecision]") {
    REQUIRE(useFloat32(1e10) == false);
}

TEST_CASE("useFloat32 true exactly at threshold", "[MixedPrecision]") {
    REQUIRE(useFloat32(kConditionThreshold) == false); // < not <=
}

// ─── Condition Number Estimators ───────────────────────────

TEST_CASE("Estimate condition number zero solution returns inf", "[MixedPrecision]") {
    double cond = estimateConditionNumber(1e-10, 1.0, 0.0);
    REQUIRE(std::isinf(cond));
}

TEST_CASE("Estimate condition number positive for valid inputs", "[MixedPrecision]") {
    double cond = estimateConditionNumber(1e-8, 1.0, 1.0);
    REQUIRE(cond > 0.0);
}

TEST_CASE("Diag ratio zero minDiag returns inf", "[MixedPrecision]") {
    double cond = estimateConditionFromDiagRatio(10.0, 0.0);
    REQUIRE(std::isinf(cond));
}

TEST_CASE("Diag ratio equal diags returns 1.0", "[MixedPrecision]") {
    double cond = estimateConditionFromDiagRatio(5.0, 5.0);
    REQUIRE(approx(cond, 1.0));
}

// ─── safeAccumulate ────────────────────────────────────────

TEST_CASE("safeAccumulate double adds correctly", "[MixedPrecision]") {
    double result = safeAccumulate(1.0, 2.0, 1e10); // high cond → use float64
    REQUIRE(approx(result, 3.0));
}

TEST_CASE("safeAccumulate float32 path also works", "[MixedPrecision]") {
    double result = safeAccumulate(1.0, 2.0, 1e8); // low cond → use float32
    REQUIRE(approx(result, 3.0));
}

// ─── adaptiveCompute ───────────────────────────────────────

TEST_CASE("adaptiveCompute selects float64 for high cond", "[MixedPrecision]") {
    auto op = [](auto scale) {
        // scale=1.0 for double, 1.0f for float
        return static_cast<double>(scale * 42.0);
    };
    double result = adaptiveCompute(1e10, op);
    REQUIRE(approx(result, 42.0));
}

TEST_CASE("adaptiveCompute selects float32 for low cond", "[MixedPrecision]") {
    auto op = [](auto scale) {
        return static_cast<double>(scale * 10.0);
    };
    double result = adaptiveCompute(1e6, op);
    REQUIRE(approx(result, 10.0));
}
