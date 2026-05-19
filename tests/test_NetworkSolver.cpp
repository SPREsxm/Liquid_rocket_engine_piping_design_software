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
    // Short pipe (0.1m) must carry more flow than long pipe (50m) — verify direction
    REQUIRE(flowShort > flowLong);

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

// ─── SolverSettings passthrough tests (Phase 20 B6) ───────────

TEST_CASE("SolverSettings SST toggle: both modes converge", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    SolverSettings sstOn, sstOff;
    sstOn.useSSTTurbulence = true;
    sstOff.useSSTTurbulence = false;

    auto solOn  = solveNetworkAuto(scene, sstOn, 1e6, 10.0);
    auto solOff = solveNetworkAuto(scene, sstOff, 1e6, 10.0);
    REQUIRE(solOn.converged == true);
    REQUIRE(solOff.converged == true);

    delete scene;
}

TEST_CASE("SolverSettings with custom pipe roughness converges", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    SolverSettings settings;
    settings.pipeRoughness = 1e-3;  // very rough pipe
    auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    delete scene;
}

TEST_CASE("SolverSettings with custom Youngs modulus converges", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    SolverSettings settings;
    settings.pipeYoungsModulus = 7e10;  // aluminum
    auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    delete scene;
}

TEST_CASE("SolverSettings with different FluidType converges", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    SolverSettings settings;
    settings.fluidType = FluidType::Water;
    auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    delete scene;
}

TEST_CASE("SolverSettings all fluid types converge", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    for (auto ft : {FluidType::LOX, FluidType::RP1, FluidType::CH4, FluidType::LH2, FluidType::Water}) {
        SolverSettings settings;
        settings.fluidType = ft;
        auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
        REQUIRE(sol.converged == true);
    }
    delete scene;
}

TEST_CASE("SolverSettings with full material properties converges", "[NetworkSolver][SolverSettings]") {
    auto* scene = createPipeline();
    SolverSettings settings;
    settings.pipeRoughness = 1.5e-4;
    settings.pipeYoungsModulus = 1.1e11;
    settings.pipeWallThickness = 0.002;
    settings.fluidType = FluidType::LOX;
    auto sol = solveNetworkAuto(scene, settings, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    delete scene;
}

// ─── Thrust integration tests (Phase 21 B1) ───────────────────

namespace {
    BlockScene* createNozzlePipeline() {
        auto& factory = ComponentFactory::instance();
        auto* scene = new BlockScene(&factory);
        auto* tank  = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
        auto* pipe  = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
        auto* nozzle = scene->addBlock(ComponentDescriptor::createNozzle(), QPointF(300, 0));

        scene->addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
        scene->addConnection(pipe->outputPorts().first(), nozzle->inputPorts().first());

        return scene;
    }
}

TEST_CASE("Pipeline with nozzle produces thrust results", "[NetworkSolver][Thrust]") {
    auto* scene = createNozzlePipeline();
    auto sol = solveNetworkAuto(scene, 5.0e6, 8.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.hasThrustResults == true);
    REQUIRE(sol.thrustResult.thrust_N > 0.0);
    REQUIRE(sol.thrustResult.specificImpulse_s > 0.0);

    delete scene;
}

TEST_CASE("Pipeline without nozzle has no thrust results", "[NetworkSolver][Thrust]") {
    auto* scene = createPipeline();
    auto sol = solveNetworkAuto(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.hasThrustResults == false);

    delete scene;
}

TEST_CASE("Nozzle with zero throat diameter handled gracefully", "[NetworkSolver][Thrust]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tank   = scene.addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
    auto* pipe   = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
    auto* nozzle = scene.addBlock(ComponentDescriptor::createNozzle(), QPointF(300, 0));
    nozzle->setPropertyValue("throatDiameter", 0.0);

    scene.addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
    scene.addConnection(pipe->outputPorts().first(), nozzle->inputPorts().first());

    auto sol = solveNetworkAuto(&scene, 5.0e6, 8.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.thrustResult.thrust_N >= 0.0);
}

TEST_CASE("NetworkSolution minPressure and maxPressure", "[NetworkSolver]") {
    auto* scene = createPipeline();
    auto sol = solveNetworkAuto(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.maxPressure() > 0.0);
    REQUIRE(sol.minPressure() >= 0.0);
    REQUIRE(sol.maxPressure() >= sol.minPressure());

    delete scene;
}

// ─── Outlet boundary condition tests (Phase 27) ─────────────────

namespace {
    BlockScene* createOutletPipeline() {
        auto& factory = ComponentFactory::instance();
        auto* scene = new BlockScene(&factory);
        auto* tank   = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
        auto* pipe   = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
        auto* outlet = scene->addBlock(ComponentDescriptor::createFuelOutlet(), QPointF(300, 0));

        scene->addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
        scene->addConnection(pipe->outputPorts().first(), outlet->inputPorts().first());

        return scene;
    }
}

TEST_CASE("Outlet pipeline: BFS solver clamps outlet pressure", "[NetworkSolver][Outlet]") {
    auto* scene = createOutletPipeline();
    auto sol = solveNetwork(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);
    REQUIRE(sol.nodes.size() == 3);

    // Find the outlet node and verify its pressure is clamped
    bool foundOutlet = false;
    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.fuelOutlet") {
            foundOutlet = true;
            REQUIRE(node.pressure == Catch::Approx(7.0e6).margin(1.0));
        }
    }
    REQUIRE(foundOutlet);
    delete scene;
}

TEST_CASE("Outlet pipeline: Hardy-Cross solver clamps outlet pressure", "[NetworkSolver][Outlet]") {
    auto* scene = createOutletPipeline();
    auto sol = solveNetworkHardyCross(scene, 1e6, 10.0, 200, 1e-6);
    REQUIRE(sol.converged == true);

    bool foundOutlet = false;
    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.fuelOutlet") {
            foundOutlet = true;
            REQUIRE(node.pressure == Catch::Approx(7.0e6).margin(1.0));
        }
    }
    REQUIRE(foundOutlet);
    delete scene;
}

TEST_CASE("Outlet pipeline: Matrix solver clamps outlet pressure", "[NetworkSolver][Outlet]") {
    auto* scene = createOutletPipeline();
    auto sol = solveNetworkMatrix(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    bool foundOutlet = false;
    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.fuelOutlet") {
            foundOutlet = true;
            REQUIRE(node.pressure == Catch::Approx(7.0e6).margin(1.0));
        }
    }
    REQUIRE(foundOutlet);
    delete scene;
}

TEST_CASE("Outlet pipeline: Auto solver clamps outlet pressure", "[NetworkSolver][Outlet]") {
    auto* scene = createOutletPipeline();
    auto sol = solveNetworkAuto(scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    bool foundOutlet = false;
    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.fuelOutlet") {
            foundOutlet = true;
            REQUIRE(node.pressure == Catch::Approx(7.0e6).margin(1.0));
        }
    }
    REQUIRE(foundOutlet);
    delete scene;
}

TEST_CASE("Outlet pipeline: OxidizerOutlet default pressure is 7 MPa", "[NetworkSolver][Outlet]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tank   = scene.addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
    auto* pipe   = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
    auto* outlet = scene.addBlock(ComponentDescriptor::createOxidizerOutlet(), QPointF(300, 0));

    scene.addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
    scene.addConnection(pipe->outputPorts().first(), outlet->inputPorts().first());

    auto sol = solveNetworkAuto(&scene, 10e6, 80.0);
    REQUIRE(sol.converged == true);

    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.oxidizerOutlet") {
            REQUIRE(node.pressure == Catch::Approx(7.0e6).margin(1.0));
        }
    }
}

TEST_CASE("Outlet pipeline: custom environment pressure is respected", "[NetworkSolver][Outlet]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tank   = scene.addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
    auto* pipe   = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
    auto* outlet = scene.addBlock(ComponentDescriptor::createFuelOutlet(), QPointF(300, 0));
    outlet->setPropertyValue("outletEnvironmentPressure", 5.0e6);

    scene.addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
    scene.addConnection(pipe->outputPorts().first(), outlet->inputPorts().first());

    auto sol = solveNetworkAuto(&scene, 8e6, 30.0);
    REQUIRE(sol.converged == true);

    for (const auto& node : sol.nodes) {
        if (node.blockTypeId == "chamber.fuelOutlet") {
            REQUIRE(node.pressure == Catch::Approx(5.0e6).margin(1.0));
        }
    }
}
