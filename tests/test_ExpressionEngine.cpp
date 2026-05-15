#ifdef USE_EXPRTK

#include <catch2/catch_all.hpp>
#include <cmath>
#include "utils/ExpressionEngine.h"

using namespace ExpressionEngine;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── Script creation ───────────────────────────────────────────

TEST_CASE("Script constructs with isValid=false", "[ExpressionEngine]") {
    Script s;
    REQUIRE(s.isValid() == false);
    REQUIRE(s.errorString().isEmpty());
}

// ─── addVariable / addConstant ─────────────────────────────────

TEST_CASE("addVariable and compile simple expression", "[ExpressionEngine]") {
    Script s;
    double x = 5.0;
    REQUIRE(s.addVariable("x", x) == true);
    REQUIRE(s.compile("x * 2 + 1") == true);
    REQUIRE(s.isValid() == true);
    REQUIRE(approx(s.evaluate(), 11.0));
}

TEST_CASE("addConstant and use in expression", "[ExpressionEngine]") {
    Script s;
    double y = 0.0;
    REQUIRE(s.addVariable("y", y) == true);
    REQUIRE(s.addConstant("k", 10.0) == true);
    REQUIRE(s.compile("y + k") == true);
    y = 3.0;
    REQUIRE(approx(s.evaluate(), 13.0));
}

TEST_CASE("Expression updates when variable changes", "[ExpressionEngine]") {
    Script s;
    double z = 1.0;
    s.addVariable("z", z);
    s.compile("z * z");
    REQUIRE(approx(s.evaluate(), 1.0));
    z = 4.0;
    REQUIRE(approx(s.evaluate(), 16.0));
}

// ─── compile errors ────────────────────────────────────────────

TEST_CASE("compile invalid expression returns false", "[ExpressionEngine]") {
    Script s;
    double x = 1.0;
    s.addVariable("x", x);
    REQUIRE(s.compile("x +* 2") == false);
    REQUIRE(s.isValid() == false);
    REQUIRE(!s.errorString().isEmpty());
}

TEST_CASE("evaluate without compile returns zero", "[ExpressionEngine]") {
    Script s;
    REQUIRE(approx(s.evaluate(), 0.0));
}

// ─── defineScalarFunction / derivative ─────────────────────────

TEST_CASE("defineScalarFunction and derivative of x^2", "[ExpressionEngine]") {
    Script s;
    REQUIRE(s.defineScalarFunction("x * x") == true);
    double d = s.derivative(3.0, 1e-6);
    REQUIRE(approx(d, 6.0, 1e-5));
}

TEST_CASE("defineScalarFunction and second derivative", "[ExpressionEngine]") {
    Script s;
    s.defineScalarFunction("x * x * x");
    double d2 = s.secondDerivative(2.0, 1e-5);
    // d²(x³)/dx² at x=2 = 6*2 = 12
    REQUIRE(approx(d2, 12.0, 1e-3));
}

TEST_CASE("derivative without function returns zero", "[ExpressionEngine]") {
    Script s;
    double x = 1.0;
    s.addVariable("x", x);
    s.compile("x * 2");
    REQUIRE(approx(s.derivative(1.0), 0.0));
}

// ─── integral ─────────────────────────────────────────────────

TEST_CASE("integral of x from 0 to 1 equals 0.5", "[ExpressionEngine]") {
    Script s;
    s.defineScalarFunction("x");
    double result = s.integral(0.0, 1.0, 10000);
    REQUIRE(approx(result, 0.5, 1e-4));
}

TEST_CASE("integral with zero intervals returns zero", "[ExpressionEngine]") {
    Script s;
    s.defineScalarFunction("x * x");
    REQUIRE(approx(s.integral(0.0, 1.0, 0), 0.0));
}

// ─── addFunction ───────────────────────────────────────────────

TEST_CASE("addFunction unary custom function", "[ExpressionEngine]") {
    Script s;
    double x = 3.0;
    s.addVariable("x", x);
    s.addFunction("cube", [](double v) { return v * v * v; });
    REQUIRE(s.compile("cube(x)") == true);
    REQUIRE(approx(s.evaluate(), 27.0));
}

TEST_CASE("addFunction2 binary custom function", "[ExpressionEngine]") {
    Script s;
    double x = 3.0;
    double y = 4.0;
    s.addVariable("x", x);
    s.addVariable("y", y);
    s.addFunction2("hypot", static_cast<Script::ScalarFunc2>(
        [](double a, double b) { return std::sqrt(a * a + b * b); }));
    REQUIRE(s.compile("hypot(x, y)") == true);
    REQUIRE(approx(s.evaluate(), 5.0));
}

// ─── built-in math functions ───────────────────────────────────

TEST_CASE("ExprTk built-in sin function", "[ExpressionEngine]") {
    Script s;
    double t = M_PI / 2.0;
    s.addVariable("t", t);
    s.compile("sin(t)");
    REQUIRE(approx(s.evaluate(), 1.0));
}

TEST_CASE("ExprTk built-in exp and log", "[ExpressionEngine]") {
    Script s;
    double x = 1.0;
    s.addVariable("x", x);
    s.compile("exp(x) + log(x + 1)");
    REQUIRE(approx(s.evaluate(), std::exp(1.0) + std::log(2.0)));
}

// ─── setMaxLoopIterations ──────────────────────────────────────

TEST_CASE("setMaxLoopIterations configurable", "[ExpressionEngine]") {
    Script s;
    s.setMaxLoopIterations(500);
    s.setMaxRecursionDepth(50);
    // verify it doesn't crash or throw
    double x = 0.0;
    s.addVariable("x", x);
    s.compile("x + 1");
    REQUIRE(approx(s.evaluate(), 1.0));
}

// ─── createFluidDynamicsScript ─────────────────────────────────

TEST_CASE("createFluidDynamicsScript returns valid script", "[ExpressionEngine]") {
    auto s = createFluidDynamicsScript();
    REQUIRE(s != nullptr);
    REQUIRE(s->compile("rho * v * pi * d * d / 4") == true);
}

#endif // USE_EXPRTK
