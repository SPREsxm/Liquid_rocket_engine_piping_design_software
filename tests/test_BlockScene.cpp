#include <catch2/catch_all.hpp>
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

TEST_CASE("BlockScene addBlock creates block with correct type", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(100, 200));
    REQUIRE(block != nullptr);
    REQUIRE(block->typeId() == "pipe.straight");
    REQUIRE(block->pos().x() > 0.0);
    REQUIRE(block->pos().y() > 0.0);
}

TEST_CASE("BlockScene removeBlock removes item", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* block = scene.addBlock(ComponentDescriptor::createElbow(), {});
    QUuid id = block->uuid();
    scene.removeBlock(block);
    REQUIRE(scene.blockByUuid(id) == nullptr);
    REQUIRE(scene.allBlocks().isEmpty());
}

TEST_CASE("BlockScene canConnect validates output-to-input", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* src = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(0, 0));
    auto* dst = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(200, 0));
    REQUIRE(src->outputPorts().size() > 0);
    REQUIRE(dst->inputPorts().size() > 0);
    REQUIRE(scene.canConnect(src->outputPorts().first(),
                              dst->inputPorts().first()));
}

TEST_CASE("BlockScene cannot connect output-to-output", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* src = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(0, 0));
    auto* dst = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(200, 0));
    REQUIRE_FALSE(scene.canConnect(src->outputPorts().first(),
                                    dst->outputPorts().first()));
}

TEST_CASE("BlockScene cannot connect same block ports", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* block = scene.addBlock(ComponentDescriptor::createFlowSensor(),
                                 QPointF(0, 0));
    // FlowSensor has input(Fluid) -> output(Signal)
    // Can't connect input to output on same block
    REQUIRE_FALSE(scene.canConnect(block->outputPorts().first(),
                                    block->inputPorts().first()));
}

TEST_CASE("BlockScene addConnection creates valid connection", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* src = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(0, 0));
    auto* dst = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(200, 0));
    auto* conn = scene.addConnection(src->outputPorts().first(),
                                      dst->inputPorts().first());
    REQUIRE(conn != nullptr);
    REQUIRE(conn->sourcePort()->isConnected());
    REQUIRE(conn->destPort()->isConnected());
    REQUIRE(scene.allConnections().size() == 1);
}

TEST_CASE("BlockScene clearScene removes everything", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    scene.addBlock(ComponentDescriptor::createStraightPipe(), {});
    scene.addBlock(ComponentDescriptor::createElbow(), QPointF(100, 0));
    scene.clearScene();
    REQUIRE(scene.allBlocks().isEmpty());
    REQUIRE(scene.allConnections().isEmpty());
}

TEST_CASE("BlockScene blockByUuid finds correct block", "[BlockScene]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    auto* b1 = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                              QPointF(0, 0));
    scene.addBlock(ComponentDescriptor::createElbow(), QPointF(200, 0));
    auto* found = scene.blockByUuid(b1->uuid());
    REQUIRE(found != nullptr);
    REQUIRE(found->typeId() == "pipe.straight");
}
