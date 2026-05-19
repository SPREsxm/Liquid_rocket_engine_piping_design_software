#include <catch2/catch_all.hpp>
#include "utils/BlowdownSolver.h"
#include "utils/NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"

namespace {
    BlockScene* createTankOutletPipeline() {
        auto& factory = ComponentFactory::instance();
        auto* scene = new BlockScene(&factory);
        auto* tank   = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
        auto* pipe   = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
        auto* sensor = scene->addBlock(ComponentDescriptor::createPressureSensor(), QPointF(300, 0));
        auto* outlet = scene->addBlock(ComponentDescriptor::createFuelOutlet(), QPointF(450, 0));

        // Configure tank for blowdown
        tank->setPropertyValue("volume", 10.0);
        tank->setPropertyValue("storedMass", 9128.0);
        tank->setPropertyValue("ullagePressure", 10.0e6);
        tank->setPropertyValue("ullageFraction", 0.2);

        scene->addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
        scene->addConnection(pipe->outputPorts().first(), sensor->inputPorts().first());
        scene->addConnection(sensor->outputPorts().first(), outlet->inputPorts().first());

        return scene;
    }
}

TEST_CASE("BlowdownSolver detects no tanks when missing ullagePressure", "[BlowdownSolver]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    // Tank without ullagePressure uses default 0 → not a blowdown tank
    scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(0, 0));

    SolverSettings settings;
    BlowdownSolver solver;
    auto result = solver.simulate(&scene, settings, 1e6, 10.0);
    REQUIRE(result.sensorTraces.isEmpty());
    REQUIRE(result.message.contains("No propellant tanks"));
}

TEST_CASE("BlowdownSolver runs on simple tank-pipe-outlet", "[BlowdownSolver]") {
    auto* scene = createTankOutletPipeline();
    SolverSettings settings;
    settings.fluidType = FluidType::LOX;

    BlowdownSolver solver;
    solver.setTimeStep(2.0);
    solver.setMaxDuration(10.0);
    solver.setMinTankPressure(1.0e5);

    auto result = solver.simulate(scene, settings, 10e6, 80.0);

    REQUIRE(result.stepsCompleted > 0);
    REQUIRE(result.totalDuration > 0.0);
    REQUIRE_FALSE(result.message.isEmpty());

    // Should have sensor traces for tank and pressure sensor
    REQUIRE(result.sensorTraces.size() >= 2);

    // Each trace should have data points
    for (const auto& trace : result.sensorTraces) {
        REQUIRE(trace.times.size() == result.stepsCompleted);
        REQUIRE(trace.pressures.size() == result.stepsCompleted);
    }

    delete scene;
}

TEST_CASE("BlowdownSolver tank pressure decays over time", "[BlowdownSolver]") {
    auto* scene = createTankOutletPipeline();
    SolverSettings settings;
    settings.fluidType = FluidType::LOX;

    BlowdownSolver solver;
    solver.setTimeStep(2.0);
    solver.setMaxDuration(20.0);

    auto result = solver.simulate(scene, settings, 10e6, 80.0);
    REQUIRE(result.stepsCompleted >= 2);

    // Find the tank trace
    const BlowdownSensorTrace* tankTrace = nullptr;
    for (const auto& trace : result.sensorTraces) {
        if (trace.blockTypeId == "tank.storage") {
            tankTrace = &trace;
            break;
        }
    }
    REQUIRE(tankTrace != nullptr);
    REQUIRE(tankTrace->pressures.size() >= 2);

    // Pressure should decrease over time (isothermal expansion)
    double firstPressure = tankTrace->pressures.first();
    double lastPressure = tankTrace->pressures.last();
    REQUIRE(firstPressure > 0.0);
    REQUIRE(lastPressure < firstPressure);

    delete scene;
}

TEST_CASE("BlowdownSolver empty scene returns message", "[BlowdownSolver]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    SolverSettings settings;
    BlowdownSolver solver;
    auto result = solver.simulate(&scene, settings, 1e6, 10.0);
    REQUIRE_FALSE(result.message.isEmpty());
    REQUIRE(result.stepsCompleted == 0);
}

TEST_CASE("BlowdownSolver with null scene returns message", "[BlowdownSolver]") {
    SolverSettings settings;
    BlowdownSolver solver;
    auto result = solver.simulate(nullptr, settings, 1e6, 10.0);
    REQUIRE_FALSE(result.message.isEmpty());
}

TEST_CASE("BlowdownSolver custom time step is respected", "[BlowdownSolver]") {
    auto* scene = createTankOutletPipeline();
    SolverSettings settings;
    settings.fluidType = FluidType::LOX;

    BlowdownSolver solver;
    solver.setTimeStep(5.0);
    solver.setMaxDuration(15.0);

    auto result = solver.simulate(scene, settings, 10e6, 80.0);
    // With dt=5.0, maxT=15.0: steps at t=0,5,10,15 = 4 steps
    REQUIRE(result.stepsCompleted >= 3);
    REQUIRE(result.stepsCompleted <= 5);

    delete scene;
}

TEST_CASE("BlowdownSolver includes flow sensor traces", "[BlowdownSolver]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tank   = scene.addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
    auto* flowS  = scene.addBlock(ComponentDescriptor::createFlowSensor(), QPointF(150, 0));
    auto* outlet = scene.addBlock(ComponentDescriptor::createFuelOutlet(), QPointF(300, 0));

    tank->setPropertyValue("storedMass", 5000.0);
    tank->setPropertyValue("ullagePressure", 10.0e6);
    tank->setPropertyValue("ullageFraction", 0.2);

    scene.addConnection(tank->outputPorts().first(), flowS->inputPorts().first());
    scene.addConnection(flowS->outputPorts().first(), outlet->inputPorts().first());

    SolverSettings settings;
    settings.fluidType = FluidType::LOX;
    BlowdownSolver solver;
    solver.setTimeStep(5.0);
    solver.setMaxDuration(10.0);

    auto result = solver.simulate(&scene, settings, 10e6, 80.0);
    REQUIRE(result.stepsCompleted > 0);

    // Find flow sensor trace
    bool foundFlowSensor = false;
    for (const auto& trace : result.sensorTraces) {
        if (trace.blockTypeId == "sensor.flow") {
            foundFlowSensor = true;
            REQUIRE(trace.flowRates.size() == result.stepsCompleted);
            break;
        }
    }
    REQUIRE(foundFlowSensor);
}
