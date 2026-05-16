#include <catch2/catch_test_macros.hpp>

#include "utils/ThermalSolver.h"
#include "utils/NetworkSolver.h"
#include "core/Types.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "ui/graphics/BlockScene.h"

TEST_CASE("ThermalSolver handles nullptr scene", "[ThermalSolver]")
{
    NetworkSolution sol;
    sol.converged = false;

    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;

    auto result = computeThermalStress(nullptr, sol, settings);
    REQUIRE(result.edges.isEmpty());
    REQUIRE(result.minSafetyFactor == 999.0);
    REQUIRE(result.edgesWithYieldExceeded == 0);
    REQUIRE(result.avgHeatTransferCoeff == 0.0);
}

TEST_CASE("ThermalSolver handles unconverged solution", "[ThermalSolver]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    NetworkSolution sol;
    sol.converged = false;

    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;

    auto result = computeThermalStress(&scene, sol, settings);
    REQUIRE(result.edges.isEmpty());
}

TEST_CASE("ThermalSolver handles empty scene", "[ThermalSolver]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    NetworkSolution sol;
    sol.converged = true;

    SolverSettings settings;
    settings.fluidType = FluidType::LH2;
    settings.fluidDensity = 70.9;

    auto result = computeThermalStress(&scene, sol, settings);
    // No edges in empty scene
    REQUIRE(result.minSafetyFactor == 999.0);
    REQUIRE(result.edgesWithYieldExceeded == 0);
    REQUIRE(result.avgHeatTransferCoeff == 0.0);
}

TEST_CASE("ThermalStressEdge default values", "[ThermalSolver]")
{
    ThermalStressEdge e;
    REQUIRE(e.sourceUuid.isNull());
    REQUIRE(e.destUuid.isNull());
    REQUIRE(e.reynoldsNumber == 0.0);
    REQUIRE(e.prandtlNumber == 0.0);
    REQUIRE(e.nusseltNumber == 0.0);
    REQUIRE(e.heatTransferCoeff_Wpm2K == 0.0);
    REQUIRE(e.hoopStress_Pa == 0.0);
    REQUIRE(e.longitudinalStress_Pa == 0.0);
    REQUIRE(e.vonMisesStress_Pa == 0.0);
    REQUIRE(e.safetyFactor == 999.0);
    REQUIRE_FALSE(e.yieldExceeded);
    REQUIRE(e.kortevegWaveSpeed_mps == 0.0);
    REQUIRE(e.materialUsed.isEmpty());
}

TEST_CASE("ThermalStressResult default values", "[ThermalSolver]")
{
    ThermalStressResult r;
    REQUIRE(r.edges.isEmpty());
    REQUIRE(r.minSafetyFactor == 999.0);
    REQUIRE(r.edgesWithYieldExceeded == 0);
    REQUIRE(r.avgHeatTransferCoeff == 0.0);
}

TEST_CASE("ThermalSolver with different fluid types", "[ThermalSolver]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    NetworkSolution sol;
    sol.converged = true;

    // LOX
    {
        SolverSettings settings;
        settings.fluidType = FluidType::LOX;
        settings.fluidDensity = 1141.0;
        auto result = computeThermalStress(&scene, sol, settings);
        REQUIRE(result.edges.isEmpty());
    }

    // RP1
    {
        SolverSettings settings;
        settings.fluidType = FluidType::RP1;
        settings.fluidDensity = 810.0;
        auto result = computeThermalStress(&scene, sol, settings);
        REQUIRE(result.edges.isEmpty());
    }
}
