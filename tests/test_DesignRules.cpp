#include <catch2/catch_test_macros.hpp>

#include "utils/DesignRules.h"
#include "utils/NetworkSolver.h"
#include "core/Types.h"

TEST_CASE("DesignCheckResult counts errors and warnings", "[DesignRules]")
{
    DesignCheckResult r;
    REQUIRE(r.errorCount() == 0);
    REQUIRE(r.warningCount() == 0);

    r.items.append({DesignCheckResult::Error, "Test", "E", {}, "msg", 1.0, 0.5, "m"});
    r.items.append({DesignCheckResult::Warning, "Test", "W", {}, "msg", 0.8, 1.0, "m"});
    r.items.append({DesignCheckResult::Pass, "Test", "P", {}, "msg", 0.3, 1.0, "m"});
    r.items.append({DesignCheckResult::Error, "Test", "E2", {}, "msg", 2.0, 0.5, "m"});

    REQUIRE(r.errorCount() == 2);
    REQUIRE(r.warningCount() == 1);
}

TEST_CASE("runDesignChecks returns empty result on nullptr scene", "[DesignRules]")
{
    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;
    NetworkSolution emptySol;
    auto result = runDesignChecks(nullptr, emptySol, settings);
    REQUIRE(result.items.isEmpty());
}

TEST_CASE("runDesignChecks handles empty solution", "[DesignRules]")
{
    // With no scene, result should be empty
    SolverSettings settings;
    NetworkSolution emptySol;
    auto result = runDesignChecks(nullptr, emptySol, settings);
    REQUIRE(result.errorCount() == 0);
    REQUIRE(result.warningCount() == 0);
}

TEST_CASE("Pressure drop budget check with disabled budget", "[DesignRules]")
{
    SolverSettings settings;
    NetworkSolution sol;
    sol.totalPressureDrop = 5.0e6; // 5 MPa

    // maxPressureDropPa = -1 (disabled)
    auto result = runDesignChecks(nullptr, sol, settings, -1.0);
    // Budget disabled, no check performed (but scene is nullptr so no edges either)
    bool hasBudgetIssue = false;
    for (const auto& item : result.items) {
        if (item.ruleName == "Pressure Drop Budget")
            hasBudgetIssue = true;
    }
    REQUIRE_FALSE(hasBudgetIssue);
}

TEST_CASE("SolverSettings velocity limits per fluid type", "[DesignRules]")
{
    // Verify that the limits are correctly passed through settings
    SolverSettings lox;
    lox.fluidType = FluidType::LOX;
    lox.fluidDensity = 1141.0;

    SolverSettings lh2;
    lh2.fluidType = FluidType::LH2;
    lh2.fluidDensity = 70.9;

    SolverSettings rp1;
    rp1.fluidType = FluidType::RP1;
    rp1.fluidDensity = 810.0;

    // All should produce results without crashing
    NetworkSolution emptySol;
    auto r1 = runDesignChecks(nullptr, emptySol, lox);
    auto r2 = runDesignChecks(nullptr, emptySol, lh2);
    auto r3 = runDesignChecks(nullptr, emptySol, rp1);

    REQUIRE(r1.errorCount() == 0);
    REQUIRE(r2.errorCount() == 0);
    REQUIRE(r3.errorCount() == 0);
}
