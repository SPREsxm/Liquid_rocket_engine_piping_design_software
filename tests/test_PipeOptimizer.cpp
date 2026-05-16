#include <catch2/catch_test_macros.hpp>

#include "utils/PipeOptimizer.h"
#include "utils/NetworkSolver.h"
#include "core/Types.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "ui/graphics/BlockScene.h"

TEST_CASE("PipeOptimizer returns empty on nullptr scene", "[PipeOptimizer]")
{
    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;

    auto result = optimizePipeSchedules(nullptr, settings);
    REQUIRE(result.selections.isEmpty());
    REQUIRE(result.iterationsRun == 0);
    REQUIRE(result.originalTotalWeight_kg == 0.0);
    REQUIRE(result.optimizedTotalWeight_kg == 0.0);
    REQUIRE(result.weightSaved_kg == 0.0);
    REQUIRE_FALSE(result.allConstraintsSatisfied);
}

TEST_CASE("PipeOptimizer handles scene with no pipe blocks", "[PipeOptimizer]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;

    auto result = optimizePipeSchedules(&scene, settings);
    REQUIRE(result.selections.isEmpty());
    REQUIRE(result.iterationsRun == 0);
    REQUIRE(result.originalTotalWeight_kg == 0.0);
    REQUIRE(result.optimizedTotalWeight_kg == 0.0);
}

TEST_CASE("OptimizationResult::PipeSelection default state", "[PipeOptimizer]")
{
    OptimizationResult::PipeSelection ps;
    REQUIRE(ps.blockUuid.isNull());
    REQUIRE(ps.blockLabel.isEmpty());
    REQUIRE(ps.oldNPS == 0.0);
    REQUIRE(ps.oldSchedule.isEmpty());
    REQUIRE(ps.newNPS == 0.0);
    REQUIRE(ps.newSchedule.isEmpty());
    REQUIRE(ps.oldWeight_kg == 0.0);
    REQUIRE(ps.newWeight_kg == 0.0);
    REQUIRE_FALSE(ps.changed);
}

TEST_CASE("OptimizationResult default state", "[PipeOptimizer]")
{
    OptimizationResult r;
    REQUIRE(r.selections.isEmpty());
    REQUIRE(r.originalTotalWeight_kg == 0.0);
    REQUIRE(r.optimizedTotalWeight_kg == 0.0);
    REQUIRE(r.weightSaved_kg == 0.0);
    REQUIRE(r.iterationsRun == 0);
    REQUIRE_FALSE(r.allConstraintsSatisfied);
    REQUIRE(r.violatedConstraints.isEmpty());
}

TEST_CASE("PipeOptimizer respects disabled pressure drop budget", "[PipeOptimizer]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    settings.fluidDensity = 1141.0;

    // maxPressureDropPa = -1 disables the budget check
    auto result = optimizePipeSchedules(&scene, settings, 1.0e6, 10.0, -1.0);
    REQUIRE(result.iterationsRun == 0);
}

TEST_CASE("PipeOptimizer with custom inlet conditions", "[PipeOptimizer]")
{
    ComponentFactory& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    SolverSettings settings;
    settings.fluidType = FluidType::LH2;
    settings.fluidDensity = 70.9;

    auto result = optimizePipeSchedules(&scene, settings, 15.0e6, 50.0, 2.0e6);
    REQUIRE(result.iterationsRun == 0);
}
