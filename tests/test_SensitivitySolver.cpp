#include <catch2/catch_test_macros.hpp>

#include "utils/SensitivitySolver.h"
#include "utils/NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

namespace {
    BlockScene* createPipeline() {
        auto& factory = ComponentFactory::instance();
        auto* scene = new BlockScene(&factory);
        auto* tank1 = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
        auto* pipe  = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
        auto* valve = scene->addBlock(ComponentDescriptor::createBallValve(), QPointF(300, 0));
        auto* tank2 = scene->addBlock(ComponentDescriptor::createBufferTank(), QPointF(450, 0));

        scene->addConnection(tank1->outputPorts().first(), pipe->inputPorts().first());
        scene->addConnection(pipe->outputPorts().first(), valve->inputPorts().first());
        scene->addConnection(valve->outputPorts().first(), tank2->inputPorts().first());

        return scene;
    }
}

TEST_CASE("runParameterSweep on simple pipeline", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    SolverSettings s;
    s.fluidDensity = 1141.0;
    s.fluidViscosity = 1.96e-4;

    SensitivityResult result = runParameterSweep(
        scene, s, "inletPressurePa", 0.5e6, 5.0e6, 5, 1.0e6, 10.0);

    REQUIRE(result.points.size() == 5);
    REQUIRE(result.sweptParamName == "inletPressurePa");
    REQUIRE(result.sweptParamUnit == "Pa");

    // All solutions should converge
    for (const auto& pt : result.points)
        REQUIRE(pt.solution.converged == true);

    // First point paramValue should be min, last should be max
    REQUIRE(result.points.first().paramValue == 0.5e6);
    REQUIRE(result.points.last().paramValue == 5.0e6);

    delete scene;
}

TEST_CASE("runParameterSweep with nullptr scene returns empty", "[SensitivitySolver]")
{
    SolverSettings s;
    SensitivityResult result = runParameterSweep(
        nullptr, s, "inletPressurePa", 0.5e6, 5.0e6, 5);
    REQUIRE(result.points.isEmpty());
}

TEST_CASE("runParameterSweep with too few steps returns empty", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    SolverSettings s;
    // steps < 2 is invalid
    SensitivityResult result = runParameterSweep(
        scene, s, "fluidDensity", 500.0, 1500.0, 1);
    // steps should be clamped to at least 2
    REQUIRE(result.points.size() >= 2);
    delete scene;
}

TEST_CASE("runParameterSweep sweeps fluid density", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    SolverSettings s;
    s.fluidDensity = 1141.0;
    s.fluidViscosity = 1.96e-4;

    SensitivityResult result = runParameterSweep(
        scene, s, "fluidDensity", 500.0, 1500.0, 3);

    REQUIRE(result.points.size() == 3);
    REQUIRE(result.sweptParamUnit == QStringLiteral("kg/m³"));
    delete scene;
}

TEST_CASE("SensitivityResult series helpers", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    SolverSettings s;

    SensitivityResult result = runParameterSweep(
        scene, s, "inletPressurePa", 1e6, 3e6, 3, 1e6, 10.0);

    auto dpSeries = result.totalPressureDropSeries();
    REQUIRE(dpSeries.size() == 3);
    // Higher inlet pressure → higher total pressure drop through the system
    // (larger ΔP from inlet to outlet = inlet - outlet, outlet rises less than inlet)

    auto maxPSeries = result.maxPressureSeries();
    REQUIRE(maxPSeries.size() == 3);

    delete scene;
}

TEST_CASE("runMultiCondition on simple pipeline", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    QVector<SolverSettings> conditions;
    {
        SolverSettings s1; s1.fluidDensity = 1141.0;  // LOX
        conditions.append(s1);
    }
    {
        SolverSettings s2; s2.fluidDensity = 810.0;   // RP-1
        conditions.append(s2);
    }
    {
        SolverSettings s3; s3.fluidDensity = 70.9;    // LH2
        conditions.append(s3);
    }

    SensitivityResult result = runMultiCondition(scene, conditions, 1e6, 10.0);
    REQUIRE(result.points.size() == 3);
    REQUIRE(result.sweptParamName == QStringLiteral("Condition"));

    delete scene;
}

TEST_CASE("computeTornado returns sorted bars", "[SensitivitySolver]")
{
    auto* scene = createPipeline();
    SolverSettings s;
    s.fluidDensity = 1141.0;
    s.fluidViscosity = 1.96e-4;

    QStringList params = {
        "inletPressurePa", "inletMassFlow",
        "fluidDensity", "pipeRoughness"
    };

    auto bars = computeTornado(scene, s, params, "totalPressureDrop", 1e6, 10.0);
    REQUIRE(bars.size() == 4);

    // Bars should be sorted by absolute impact (largest first)
    for (int i = 1; i < bars.size(); ++i) {
        double prevMax = qMax(qAbs(bars[i-1].negativeImpact), qAbs(bars[i-1].positiveImpact));
        double curMax  = qMax(qAbs(bars[i].negativeImpact), qAbs(bars[i].positiveImpact));
        REQUIRE(prevMax >= curMax);
    }

    // Each bar should have valid impact values (not NaN/Inf)
    for (const auto& bar : bars) {
        REQUIRE(std::isfinite(bar.negativeImpact));
        REQUIRE(std::isfinite(bar.positiveImpact));
    }

    delete scene;
}

TEST_CASE("computeTornado nullptr scene returns empty", "[SensitivitySolver]")
{
    SolverSettings s;
    auto bars = computeTornado(nullptr, s, {"fluidDensity"}, "totalPressureDrop");
    REQUIRE(bars.isEmpty());
}
