#include <catch2/catch_all.hpp>
#include "utils/NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
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

        REQUIRE(tank1->outputPorts().size() > 0);
        REQUIRE(pipe->inputPorts().size() > 0);
        REQUIRE(pipe->outputPorts().size() > 0);
        REQUIRE(valve->inputPorts().size() > 0);
        REQUIRE(valve->outputPorts().size() > 0);
        REQUIRE(tank2->inputPorts().size() > 0);

        scene->addConnection(tank1->outputPorts().first(), pipe->inputPorts().first());
        scene->addConnection(pipe->outputPorts().first(), valve->inputPorts().first());
        scene->addConnection(valve->outputPorts().first(), tank2->inputPorts().first());

        return scene;
    }
}

TEST_CASE("solveNetwork on simple pipeline converges", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetwork(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.nodes.size() == 4);
    REQUIRE(sol.edges.size() == 3);
    REQUIRE(sol.totalPressureDrop >= 0.0);
    delete scene;
}

TEST_CASE("solveNetwork null scene returns error", "[NetworkSolver]") {
    auto sol = solveNetwork(nullptr, 1e6, 10.0);
    REQUIRE(sol.converged == false);
    REQUIRE(!sol.message.isEmpty());
}

TEST_CASE("solveNetwork empty scene returns error", "[NetworkSolver]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto sol = solveNetwork(&scene, 1e6, 10.0);
    REQUIRE(sol.converged == false);
}

TEST_CASE("solveNetworkHardyCross on linear pipeline falls back to BFS", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetworkHardyCross(scene, 1e6, 10.0, 200, 1e-6);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.nodes.size() == 4);
    delete scene;
}

TEST_CASE("solveNetworkMatrix on simple pipeline converges", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetworkMatrix(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.nodes.size() == 4);
    delete scene;
}

TEST_CASE("solveNetworkAuto on simple pipeline converges", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetworkAuto(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    delete scene;
}

TEST_CASE("NetworkSolution nodes have uuids and labels", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetwork(scene, 1e6, 10.0);
    for (const auto& node : sol.nodes) {
        REQUIRE(!node.blockUuid.isNull());
        REQUIRE(!node.blockLabel.isEmpty());
    }
    delete scene;
}

TEST_CASE("NetworkSolution edges have source and dest uuids", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetwork(scene, 1e6, 10.0);
    for (const auto& edge : sol.edges) {
        REQUIRE(!edge.sourceUuid.isNull());
        REQUIRE(!edge.destUuid.isNull());
        REQUIRE(edge.resistance >= 0.0);
    }
    delete scene;
}

TEST_CASE("SolverSettings overload for HardyCross", "[NetworkSolver]") {
    SolverSettings settings;
    settings.hardyCrossMaxIter = 100;
    settings.hardyCrossTolerance = 1e-5;
    auto* scene = createPipeline();
    auto sol = solveNetworkHardyCross(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    delete scene;
}

TEST_CASE("SolverSettings overload for Auto", "[NetworkSolver]") {
    SolverSettings settings;
    auto* scene = createPipeline();
    auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    delete scene;
}

// ─── Impedance-based flow distribution tests (Phase 17 A3) ────────

namespace {
    // Short pipe should have much lower resistance → higher flow
    BlockScene* createAsymmetricTJunction() {
        auto& factory = ComponentFactory::instance();
        auto* scene = new BlockScene(&factory);

        auto* tank  = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
        auto* tee   = scene->addBlock(ComponentDescriptor::createTee(), QPointF(150, 0));
        auto* pipeS = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(300, -60));
        auto* pipeL = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(300, 60));
        auto* sinkS = scene->addBlock(ComponentDescriptor::createBufferTank(), QPointF(450, -60));
        auto* sinkL = scene->addBlock(ComponentDescriptor::createBufferTank(), QPointF(450, 60));

        // Set pipe lengths: short=0.1m, long=50m → K ratio ≈ 500
        pipeS->setPropertyValue("length", 0.1);
        pipeL->setPropertyValue("length", 50.0);
        pipeS->setPropertyValue("diameter", 0.05);
        pipeL->setPropertyValue("diameter", 0.05);

        // Tank → Tee inlet
        scene->addConnection(tank->outputPorts().first(), tee->inputPorts().first());
        // Tee Outlet A → short pipe → sinkS
        scene->addConnection(tee->outputPorts()[0], pipeS->inputPorts().first());
        scene->addConnection(pipeS->outputPorts().first(), sinkS->inputPorts().first());
        // Tee Outlet B → long pipe → sinkL
        scene->addConnection(tee->outputPorts()[1], pipeL->inputPorts().first());
        scene->addConnection(pipeL->outputPorts().first(), sinkL->inputPorts().first());

        return scene;
    }
}

TEST_CASE("Asymmetric T-junction: short pipe gets more flow than long pipe", "[NetworkSolver]") {
    auto* scene = createAsymmetricTJunction();
    auto sol = solveNetwork(scene, 1e6, 5.0);
    REQUIRE(sol.converged == true);

    // Find flow rates through each downstream pipe → sink edge
    double flowShort = -1.0, flowLong = -1.0;
    for (const auto& e : sol.edges) {
        auto* sb = scene->blockByUuid(e.sourceUuid);
        if (sb && sb->typeId() == "pipe.straight") {
            double len = sb->propertyValue("length").toDouble();
            if (len < 1.0)
                flowShort = e.massFlowRate;
            else
                flowLong = e.massFlowRate;
        }
    }
    REQUIRE(flowShort > 0.0);
    REQUIRE(flowLong > 0.0);
    // Short pipe (0.1m) must carry more flow than long pipe (50m)
    REQUIRE(flowShort > 2.0 * flowLong);

    delete scene;
}

TEST_CASE("Asymmetric T-junction: total flow is conserved", "[NetworkSolver]") {
    auto* scene = createAsymmetricTJunction();
    auto sol = solveNetwork(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    double totalOut = 0.0;
    for (const auto& node : sol.nodes) {
        auto* b = scene->blockByUuid(node.blockUuid);
        if (b && b->typeId() == "tank.buffer")
            totalOut += node.inletFlow;
    }
    // Total flow into sinks ≈ inlet flow (within 1%)
    REQUIRE(std::abs(totalOut - 10.0) < 0.1);

    delete scene;
}

TEST_CASE("Asymmetric T-junction: pressure drop in long pipe > short pipe", "[NetworkSolver]") {
    auto* scene = createAsymmetricTJunction();
    auto sol = solveNetwork(scene, 1e6, 5.0);
    REQUIRE(sol.converged == true);

    // Collect edges with pressure drop
    double dpShort = 0.0, dpLong = 0.0;
    for (const auto& e : sol.edges) {
        for (auto* b : scene->allBlocks()) {
            if (b->typeId() == "pipe.straight" && e.sourceUuid == b->uuid()) {
                double len = b->propertyValue("length").toDouble();
                if (len < 1.0)
                    dpShort = e.pressureDrop;
                else
                    dpLong = e.pressureDrop;
            }
        }
    }
    // Long pipe should have higher total pressure drop
    REQUIRE(dpLong > dpShort);

    delete scene;
}
