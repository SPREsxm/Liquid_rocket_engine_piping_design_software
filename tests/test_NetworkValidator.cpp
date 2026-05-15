#include <catch2/catch_all.hpp>
#include "utils/NetworkValidator.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

TEST_CASE("validateTopology null scene returns error", "[NetworkValidator]") {
    auto result = validateTopology(nullptr);
    REQUIRE(result.hasErrors() == true);
}

TEST_CASE("validateTopology empty scene has no issues", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto result = validateTopology(&scene);
    REQUIRE(result.hasErrors() == false);
    REQUIRE(result.hasWarnings() == false);
}

TEST_CASE("validateTopology orphan block emits warning", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(0, 0));
    auto result = validateTopology(&scene);
    REQUIRE(result.hasWarnings() == true);
    // Should have orphan warning
    bool foundOrphan = false;
    for (const auto& i : result.issues) {
        if (i.message.contains("Orphan block"))
            foundOrphan = true;
    }
    REQUIRE(foundOrphan == true);
}

TEST_CASE("validateTopology mandatory port unconnected emits warning", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tee  = scene.addBlock(ComponentDescriptor::createTee(), QPointF(0, 0));
    auto* pipe = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
    // Connect Tee output → Pipe input, but leave Tee input unconnected
    // Tee is not orphan (has connection), but its input (mandatory Fluid) is unconnected
    scene.addConnection(tee->outputPorts().first(), pipe->inputPorts().first());

    auto result = validateTopology(&scene);
    bool foundUnconnected = false;
    for (const auto& i : result.issues) {
        if (i.message.contains("unconnected"))
            foundUnconnected = true;
    }
    REQUIRE(foundUnconnected == true);
}

TEST_CASE("validateTopology closed loop detects cycle", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* p1 = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(0, 0));
    auto* p2 = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));

    // p1 → p2 and p2 → p1 creates a cycle
    scene.addConnection(p1->outputPorts().first(), p2->inputPorts().first());
    scene.addConnection(p2->outputPorts().first(), p1->inputPorts().first());

    auto result = validateTopology(&scene);
    REQUIRE(result.hasErrors() == true);
    bool foundCycle = false;
    for (const auto& i : result.issues) {
        if (i.message.contains("cycle"))
            foundCycle = true;
    }
    REQUIRE(foundCycle == true);
}

TEST_CASE("validateTopology linear pipeline has no issues", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* tank = scene.addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
    auto* pipe = scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
    auto* valve = scene.addBlock(ComponentDescriptor::createBallValve(), QPointF(300, 0));

    scene.addConnection(tank->outputPorts().first(), pipe->inputPorts().first());
    scene.addConnection(pipe->outputPorts().first(), valve->inputPorts().first());

    auto result = validateTopology(&scene);
    // May have unconnected output warnings but no errors
    REQUIRE(result.hasErrors() == false);
}

TEST_CASE("validateFlowContinuity null scene returns error", "[NetworkValidator]") {
    auto result = validateFlowContinuity(nullptr);
    REQUIRE(result.hasErrors() == true);
}

TEST_CASE("validateFlowContinuity empty scene has no issues", "[NetworkValidator]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto result = validateFlowContinuity(&scene);
    REQUIRE(result.hasErrors() == false);
}
