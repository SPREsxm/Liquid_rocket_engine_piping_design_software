#include <catch2/catch_all.hpp>
#include "ui/actions/UndoCommands.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include <QUndoStack>

#include <cmath>

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

TEST_CASE("AddBlockCommand redo adds block", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;

    auto* cmd = new AddBlockCommand(&scene,
        ComponentDescriptor::createStraightPipe(), QPointF(100, 200));
    stack.push(cmd);

    REQUIRE(scene.allBlocks().size() == 1);
    REQUIRE(scene.allBlocks().first()->typeId() == "pipe.straight");
}

TEST_CASE("AddBlockCommand undo removes block", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;

    stack.push(new AddBlockCommand(&scene,
        ComponentDescriptor::createElbow(), QPointF(50, 50)));
    REQUIRE(scene.allBlocks().size() == 1);

    stack.undo();
    REQUIRE(scene.allBlocks().isEmpty());
}

TEST_CASE("AddBlockCommand undo then redo restores", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;

    stack.push(new AddBlockCommand(&scene,
        ComponentDescriptor::createBallValve(), QPointF(0, 0)));
    stack.undo();
    REQUIRE(scene.allBlocks().isEmpty());
    stack.redo();
    REQUIRE(scene.allBlocks().size() == 1);
    REQUIRE(scene.allBlocks().first()->typeId() == "valve.ball");
}

TEST_CASE("MoveBlockCommand moves block", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    stack.push(new MoveBlockCommand(&scene, id,
                                     QPointF(0, 0), QPointF(100, 50)));
    auto* found = scene.blockByUuid(id);
    REQUIRE(found->pos().x() > 0.0);
    REQUIRE(found->pos().y() > 0.0);
}

TEST_CASE("MoveBlockCommand undo reverts position", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    stack.push(new MoveBlockCommand(&scene, id,
                                     QPointF(0, 0), QPointF(200, 200)));
    REQUIRE(scene.blockByUuid(id)->pos() != QPointF(0, 0));

    stack.undo();
    // Position should be back near origin (snapped to grid)
    auto* undone = scene.blockByUuid(id);
    REQUIRE(undone->pos().x() < 100.0);
    REQUIRE(undone->pos().y() < 100.0);
}

TEST_CASE("MoveBlockCommand mergeWith combines successive moves", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    stack.push(new MoveBlockCommand(&scene, id,
                                     QPointF(0, 0), QPointF(100, 0)));
    stack.push(new MoveBlockCommand(&scene, id,
                                     QPointF(100, 0), QPointF(200, 0)));

    // One undo should go back to (0,0) if merge worked (move id = 1001)
    stack.undo();
    auto* undone = scene.blockByUuid(id);
    REQUIRE(undone->pos().x() < 100.0);
}

TEST_CASE("ChangePropertyCommand changes value", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    double oldVal = block->propertyValue("length").toDouble();
    double newVal = oldVal + 1.0;

    stack.push(new ChangePropertyCommand(&scene, id, "length",
                                          QVariant(oldVal), QVariant(newVal)));
    REQUIRE(approx(block->propertyValue("length").toDouble(), newVal, 0.01));
}

TEST_CASE("ChangePropertyCommand undo reverts value", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createStraightPipe(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    double oldVal = block->propertyValue("length").toDouble();
    double newVal = oldVal + 2.0;

    stack.push(new ChangePropertyCommand(&scene, id, "length",
                                          QVariant(oldVal), QVariant(newVal)));
    stack.undo();
    REQUIRE(approx(block->propertyValue("length").toDouble(), oldVal, 0.01));
}

TEST_CASE("RemoveBlockCommand redo removes block", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createTee(),
                                 QPointF(0, 0));
    QUuid id = block->uuid();

    stack.push(new RemoveBlockCommand(&scene, id));
    REQUIRE(scene.blockByUuid(id) == nullptr);
}

TEST_CASE("RemoveBlockCommand undo restores block", "[UndoCommands]") {
    auto& factory = ComponentFactory::instance();
    BlockScene scene(&factory);
    QUndoStack stack;
    auto* block = scene.addBlock(ComponentDescriptor::createTee(),
                                 QPointF(50, 50));
    QUuid id = block->uuid();

    stack.push(new RemoveBlockCommand(&scene, id));
    REQUIRE(scene.blockByUuid(id) == nullptr);

    stack.undo();
    // After undo, block should be restored from saved scene state
    REQUIRE(scene.allBlocks().size() > 0);
}
