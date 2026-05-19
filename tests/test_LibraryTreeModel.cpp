#include <catch2/catch_all.hpp>
#include "ui/library/LibraryTreeModel.h"
#include "components/ComponentFactory.h"

TEST_CASE("LibraryTreeModel has 6 category rows") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);
    REQUIRE(model.rowCount() == 6);
}

TEST_CASE("LibraryTreeModel category names") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);

    QStringList cats;
    for (int i = 0; i < model.rowCount(); ++i)
        cats.append(model.item(i)->text());
    REQUIRE(cats.contains("Pipes"));
    REQUIRE(cats.contains("Valves"));
    REQUIRE(cats.contains("Pumps"));
    REQUIRE(cats.contains("Sensors"));
    REQUIRE(cats.contains("Tanks"));
    REQUIRE(cats.contains("Combustion"));
}

TEST_CASE("LibraryTreeModel leaf items have TypeIdRole") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);
    int typeIdRole = LibraryTreeModel::TypeIdRole;

    int leafCount = 0;
    for (int i = 0; i < model.rowCount(); ++i) {
        auto* catItem = model.item(i);
        REQUIRE(catItem != nullptr);
        for (int j = 0; j < catItem->rowCount(); ++j) {
            auto* leaf = catItem->child(j);
            REQUIRE(leaf != nullptr);
            QVariant typeId = leaf->data(typeIdRole);
            REQUIRE(!typeId.isNull());
            REQUIRE(!typeId.toString().isEmpty());
            ++leafCount;
        }
    }
    REQUIRE(leafCount == 50);
}

TEST_CASE("LibraryTreeModel known typeIds present") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);
    int typeIdRole = LibraryTreeModel::TypeIdRole;

    // Collect all typeIds
    QStringList allTypeIds;
    for (int i = 0; i < model.rowCount(); ++i) {
        auto* catItem = model.item(i);
        for (int j = 0; j < catItem->rowCount(); ++j) {
            allTypeIds.append(catItem->child(j)->data(typeIdRole).toString());
        }
    }

    REQUIRE(allTypeIds.contains("pipe.straight"));
    REQUIRE(allTypeIds.contains("valve.ball"));
    REQUIRE(allTypeIds.contains("pump.centrifugal"));
    REQUIRE(allTypeIds.contains("sensor.pressure"));
    REQUIRE(allTypeIds.contains("tank.storage"));
    REQUIRE(allTypeIds.contains("chamber.nozzle"));
    REQUIRE(allTypeIds.contains("pump.turbopump"));
    REQUIRE(allTypeIds.contains("valve.relief"));
}

TEST_CASE("LibraryTreeModel category item flags prevent selection") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);

    for (int i = 0; i < model.rowCount(); ++i) {
        auto* catItem = model.item(i);
        REQUIRE_FALSE(catItem->flags() & Qt::ItemIsSelectable);
    }
}

TEST_CASE("LibraryTreeModel leaf items are draggable and selectable") {
    auto& factory = ComponentFactory::instance();
    LibraryTreeModel model(&factory);

    // Check first leaf of Pipes category
    auto* pipesCat = model.item(0);
    REQUIRE(pipesCat != nullptr);
    REQUIRE(pipesCat->rowCount() > 0);

    auto* leaf = pipesCat->child(0);
    REQUIRE(leaf->flags() & Qt::ItemIsSelectable);
    REQUIRE(leaf->flags() & Qt::ItemIsDragEnabled);
}
