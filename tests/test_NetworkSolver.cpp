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
