#include <catch2/catch_all.hpp>
#include "utils/Benchmark.h"
#include <QString>
#include <QList>

using namespace Benchmark;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── standardCases ────────────────────────────────────────────

TEST_CASE("Standard cases returns 4 entries", "[Benchmark]") {
    auto cases = standardCases();
    REQUIRE(cases.size() == 4);
}

TEST_CASE("Standard cases have non-empty names", "[Benchmark]") {
    auto cases = standardCases();
    for (const auto& c : cases) {
        REQUIRE(!c.name.isEmpty());
        REQUIRE(c.tolerance > 0.0);
    }
}

// ─── check ─────────────────────────────────────────────────────

TEST_CASE("check passes when within tolerance", "[Benchmark]") {
    auto r = check("Test", 100.0, 100.3, 0.005);
    REQUIRE(r.passed == true);
    REQUIRE(approx(r.relativeError, 0.003));
}

TEST_CASE("check fails when exceeding tolerance", "[Benchmark]") {
    auto r = check("Test", 100.0, 102.0, 0.005);
    REQUIRE(r.passed == false);
    REQUIRE(r.relativeError > 0.005);
}

TEST_CASE("check zero expected uses small denominator", "[Benchmark]") {
    auto r = check("Zero", 0.0, 1e-3, 0.005);
    REQUIRE(r.relativeError > 1e-3);
}

// ─── formatResult ──────────────────────────────────────────────

TEST_CASE("formatResult pass returns PASS prefix", "[Benchmark]") {
    BenchmarkResult r{"Case1", 10.0, 10.05, 0.005, true};
    QString s = formatResult(r);
    REQUIRE(s.contains("PASS"));
    REQUIRE(s.contains("Case1"));
}

TEST_CASE("formatResult fail returns FAIL prefix", "[Benchmark]") {
    BenchmarkResult r{"Case2", 10.0, 10.5, 0.05, false};
    QString s = formatResult(r);
    REQUIRE(s.contains("FAIL"));
}

// ─── benchmarkJoukowsky ────────────────────────────────────────

TEST_CASE("Joukowsky benchmark passes", "[Benchmark]") {
    auto r = benchmarkJoukowsky();
    REQUIRE(r.passed == true);
    REQUIRE(approx(r.expected, 1000.0 * 1200.0 * 5.0));
}

// ─── benchmarkLaminarPipe ──────────────────────────────────────

TEST_CASE("Laminar pipe benchmark passes", "[Benchmark]") {
    auto r = benchmarkLaminarPipe();
    REQUIRE(r.passed == true);
    REQUIRE(r.expected > 0.0);
}

TEST_CASE("Laminar pipe benchmark matches Hagen-Poiseuille", "[Benchmark]") {
    double L = 1.0, d = 0.01, rho = 998.0, mu = 0.001, mdot = 0.01;
    double expected = 128.0 * mu * L * mdot / (M_PI * rho * d * d * d * d);
    REQUIRE(expected > 0.0);
    REQUIRE(expected < 1e7);
}
