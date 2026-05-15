#include <catch2/catch_all.hpp>
#include "utils/TransientSolver.h"
#include "utils/NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }

    // Create a pipeline and solve the steady state for transient analysis
    struct TestPipeline {
        BlockScene* scene = nullptr;
        NetworkSolution steady;

        TestPipeline() {
            auto& factory = ComponentFactory::instance();
            scene = new BlockScene(&factory);
            auto* tank1 = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
            auto* pipe  = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
            auto* valve = scene->addBlock(ComponentDescriptor::createBallValve(), QPointF(300, 0));
            auto* tank2 = scene->addBlock(ComponentDescriptor::createBufferTank(), QPointF(450, 0));

            scene->addConnection(tank1->outputPorts().first(), pipe->inputPorts().first());
            scene->addConnection(pipe->outputPorts().first(), valve->inputPorts().first());
            scene->addConnection(valve->outputPorts().first(), tank2->inputPorts().first());

            steady = solveNetwork(scene, 1e6, 10.0);
        }

        ~TestPipeline() { delete scene; }
    };
}

// ─── computeWaveSpeed ──────────────────────────────────────────

TEST_CASE("Wave speed positive for valid segment", "[TransientSolver]") {
    TransientSolver ts;
    PipeSegment seg{1.0, 0.0254, 0.001, 2e11, 0.000045, 1141.0, 1.96e-4, 1e9};
    double c = ts.computeWaveSpeed(seg);
    REQUIRE(c > 0.0);
    REQUIRE(c < 10000.0);
}

TEST_CASE("Wave speed zero for zero bulk modulus", "[TransientSolver]") {
    TransientSolver ts;
    PipeSegment seg{1.0, 0.0254, 0.001, 2e11, 0.000045, 1141.0, 1.96e-4, 0.0};
    double c = ts.computeWaveSpeed(seg);
    REQUIRE(approx(c, 0.0));
}

// ─── frictionSlope ─────────────────────────────────────────────

TEST_CASE("Friction slope positive for nonzero velocity", "[TransientSolver]") {
    TransientSolver ts;
    double slope = ts.frictionSlope(5.0, 0.0254, 0.000045, 1141.0, 1.96e-4);
    REQUIRE(slope > 0.0);
}

TEST_CASE("Friction slope zero for zero velocity", "[TransientSolver]") {
    TransientSolver ts;
    double slope = ts.frictionSlope(0.0, 0.0254, 0.000045, 1141.0, 1.96e-4);
    REQUIRE(approx(slope, 0.0));
}

TEST_CASE("Friction slope zero for zero diameter", "[TransientSolver]") {
    TransientSolver ts;
    double slope = ts.frictionSlope(5.0, 0.0, 0.000045, 1141.0, 1.96e-4);
    REQUIRE(approx(slope, 0.0));
}

// ─── computeAdaptiveDt ─────────────────────────────────────────

TEST_CASE("Adaptive dt positive", "[TransientSolver]") {
    TransientSolver ts;
    std::vector<double> vels = {5.0, 4.5, 4.0};
    double dt = ts.computeAdaptiveDt(1200.0, 0.1, vels);
    REQUIRE(dt > 0.0);
    REQUIRE(dt < 0.1);
}

TEST_CASE("Adaptive dt no velocities uses wave speed only", "[TransientSolver]") {
    TransientSolver ts;
    std::vector<double> empty;
    double dt = ts.computeAdaptiveDt(1200.0, 0.1, empty);
    REQUIRE(dt > 0.0);
}

// ─── targetCourant getter/setter ───────────────────────────────

TEST_CASE("Target Courant number default is 0.9", "[TransientSolver]") {
    TransientSolver ts;
    REQUIRE(approx(ts.targetCourant(), 0.9));
}

TEST_CASE("Target Courant number setter works", "[TransientSolver]") {
    TransientSolver ts;
    ts.setTargetCourant(0.5);
    REQUIRE(approx(ts.targetCourant(), 0.5));
}

// ─── simulateWaterHammer ───────────────────────────────────────

TEST_CASE("Water hammer null scene returns error", "[TransientSolver]") {
    TransientSolver ts;
    NetworkSolution dummy;
    auto result = ts.simulateWaterHammer(dummy, nullptr, 0.1, 50);
    REQUIRE(!result.message.isEmpty());
    REQUIRE(result.history.empty());
}

TEST_CASE("Water hammer on pipeline produces pressure surge", "[TransientSolver]") {
    TestPipeline tp;
    REQUIRE(tp.steady.converged == true);

    TransientSolver ts;
    auto result = ts.simulateWaterHammer(tp.steady, tp.scene, 0.1, 50);
    REQUIRE(result.maxPressure > 1e6);  // should exceed inlet pressure
    REQUIRE(result.spatialNodes == 50);
    REQUIRE(result.timeSteps > 0);
    REQUIRE(!result.history.empty());
}

TEST_CASE("Water hammer history has increasing time", "[TransientSolver]") {
    TestPipeline tp;

    TransientSolver ts;
    auto result = ts.simulateWaterHammer(tp.steady, tp.scene, 0.05, 50);
    REQUIRE(result.history.size() >= 2);
    for (size_t i = 1; i < result.history.size(); ++i) {
        REQUIRE(result.history[i].time >= result.history[i - 1].time);
    }
}

TEST_CASE("Water hammer with user-specified time step", "[TransientSolver]") {
    TestPipeline tp;

    TransientSolver ts;
    auto result = ts.simulateWaterHammer(tp.steady, tp.scene, 0.1, 50, 0.001);
    REQUIRE(result.maxPressure > 1e6);
    REQUIRE(result.timeSteps > 0);
}
