#include <catch2/catch_all.hpp>
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "ui/graphics/PortItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <cmath>

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

TEST_CASE("BlockScene toJson produces valid structure", "[Serialization]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    scene.addBlock(ComponentDescriptor::createStraightPipe(), QPointF(0, 0));
    scene.addBlock(ComponentDescriptor::createElbow(), QPointF(200, 0));

    QJsonObject json = scene.toJson();
    REQUIRE(json.contains("version"));
    REQUIRE(json.contains("blocks"));
    REQUIRE(json.contains("connections"));
    QJsonArray blocks = json["blocks"].toArray();
    REQUIRE(blocks.size() == 2);
}

TEST_CASE("BlockScene toJson/fromJson round-trip preserves blocks", "[Serialization]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    auto* b1 = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                              QPointF(100, 200));
    b1->setPropertyValue("length", 3.5);
    b1->setCustomLabel("Pipe A");

    scene.addBlock(ComponentDescriptor::createGateValve(), QPointF(300, 200));

    QJsonObject json = scene.toJson();
    scene.clearScene();
    scene.fromJson(json);

    REQUIRE(scene.allBlocks().size() == 2);

    BlockItem* restored = nullptr;
    for (auto* b : scene.allBlocks()) {
        if (b->typeId() == "pipe.straight") {
            restored = b;
            break;
        }
    }
    REQUIRE(restored != nullptr);
    double len = restored->propertyValue("length").toDouble();
    REQUIRE(approx(len, 3.5, 0.01));
    REQUIRE(restored->customLabel() == "Pipe A");
}

TEST_CASE("BlockScene toJson/fromJson preserves connections", "[Serialization]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    auto* src = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(0, 0));
    auto* dst = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                               QPointF(200, 0));
    scene.addConnection(src->outputPorts().first(), dst->inputPorts().first());

    REQUIRE(scene.allConnections().size() == 1);

    QJsonObject json = scene.toJson();
    scene.clearScene();
    scene.fromJson(json);

    REQUIRE(scene.allConnections().size() == 1);
    ConnectionItem* restored = scene.allConnections().first();
    REQUIRE(restored->sourcePort() != nullptr);
    REQUIRE(restored->destPort() != nullptr);
    REQUIRE(restored->sourcePort()->parentBlock()->typeId() == "pipe.straight");
    REQUIRE(restored->destPort()->parentBlock()->typeId() == "pipe.straight");
}

TEST_CASE("BlockScene toJson empty scene", "[Serialization]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    QJsonObject json = scene.toJson();
    QJsonArray blocks = json["blocks"].toArray();
    REQUIRE(blocks.size() == 0);
    QJsonArray conns = json["connections"].toArray();
    REQUIRE(conns.size() == 0);
}

TEST_CASE("BlockScene fromJson with missing optional fields is tolerant", "[Serialization]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);

    QJsonObject json;
    json["version"] = 1;
    json["blocks"] = QJsonArray();
    QJsonArray conns;
    json["connections"] = conns;

    REQUIRE_NOTHROW(scene.fromJson(json));
}
