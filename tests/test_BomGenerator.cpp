#include <catch2/catch_test_macros.hpp>

#include "utils/BomGenerator.h"
#include "utils/PipeScheduleDatabase.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

#include <QApplication>
#include <QTemporaryFile>

// Helper to set up a scene with blocks for BOM testing
struct BomTestFixture {
    BlockScene scene{&ComponentFactory::instance()};

    BlockItem* addPipe(double length = 2.5, double nps = 1.0, const QString& sch = "40S")
    {
        auto desc = ComponentDescriptor::createStraightPipe();
        auto inst = ComponentInstance::create(desc, QPointF(0, 0));
        auto* block = new BlockItem(inst, desc);
        scene.addItem(block);
        block->setPropertyValue("length", length);
        block->setPropertyValue("nps", nps);
        block->setPropertyValue("schedule", sch);
        return block;
    }

    BlockItem* addValve(const QString& type = "valve.gate")
    {
        const auto* desc = ComponentFactory::instance().descriptorForType(type);
        if (!desc) return nullptr;
        auto inst = ComponentInstance::create(*desc, QPointF(0, 0));
        auto* block = new BlockItem(inst, *desc);
        scene.addItem(block);
        return block;
    }
};

TEST_CASE("generateBom with empty scene returns empty result", "[BomGenerator]")
{
    BomTestFixture fix;
    BomResult bom = generateBom(&fix.scene);
    REQUIRE(bom.lines.isEmpty());
    REQUIRE(bom.totalWeight == 0.0);
}

TEST_CASE("generateBom with nullptr returns empty", "[BomGenerator]")
{
    BomResult bom = generateBom(nullptr);
    REQUIRE(bom.lines.isEmpty());
}

TEST_CASE("generateBom counts single pipe correctly", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(3.0, 1.0, "40S"); // 1" SCH-40S

    BomResult bom = generateBom(&fix.scene);
    REQUIRE(bom.lines.size() == 1);

    const auto& line = bom.lines[0];
    REQUIRE(line.typeId == "pipe.straight");
    REQUIRE(line.quantity == 1);
    REQUIRE(line.unit == "m");
    REQUIRE(line.remark.contains("3")); // length 3.0 m
}

TEST_CASE("generateBom groups identical pipes", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(2.0, 1.0, "40S");
    fix.addPipe(3.0, 1.0, "40S");
    fix.addPipe(1.5, 1.0, "40S");

    BomResult bom = generateBom(&fix.scene);
    REQUIRE(bom.lines.size() == 1);
    REQUIRE(bom.lines[0].quantity == 3); // 3 blocks, same spec
    REQUIRE(bom.lines[0].remark.contains("6.5")); // total length 6.5 m
}

TEST_CASE("generateBom separates different schedules", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(2.0, 1.0, "40S");
    fix.addPipe(3.0, 1.0, "80S");

    BomResult bom = generateBom(&fix.scene);
    // Different schedules should be separate BOM lines
    REQUIRE(bom.lines.size() >= 2);
}

TEST_CASE("generateBom includes valves as pcs", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addValve("valve.gate");
    fix.addValve("valve.ball");

    BomResult bom = generateBom(&fix.scene);
    REQUIRE(bom.lines.size() == 2);
    for (const auto& line : bom.lines) {
        REQUIRE(line.unit == "pcs");
    }
}

TEST_CASE("generateBom mixed pipe and valve", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(1.0, 0.5, "40S");
    fix.addValve("valve.gate");

    BomResult bom = generateBom(&fix.scene);
    REQUIRE(bom.lines.size() == 2);

    auto cats = bom.byCategory();
    REQUIRE(cats.contains("Pipes"));
    REQUIRE(cats.contains("Valves"));
}

TEST_CASE("exportBomCsv writes valid CSV", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(1.0, 0.5, "40S");
    fix.addValve("valve.ball");

    BomResult bom = generateBom(&fix.scene);

    QTemporaryFile tmp;
    tmp.setFileTemplate(tmp.fileTemplate() + ".csv");
    REQUIRE(tmp.open());
    QString path = tmp.fileName();
    tmp.close();

    bool ok = exportBomCsv(bom, path);
    REQUIRE(ok);

    QFile verify(path);
    REQUIRE(verify.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(verify.readAll());
    REQUIRE(content.contains("Category"));
    REQUIRE(content.contains("pipe.straight"));
    REQUIRE(content.contains("valve.ball"));
    REQUIRE(content.contains("Total Weight"));
    verify.close();
    verify.remove();
}

TEST_CASE("bomToHtmlTable returns HTML", "[BomGenerator]")
{
    BomTestFixture fix;
    fix.addPipe(2.0, 1.0, "40S");

    BomResult bom = generateBom(&fix.scene);
    QString html = bomToHtmlTable(bom);

    REQUIRE(html.contains("<table"));
    REQUIRE(html.contains("<th>Category</th>"));
    REQUIRE(html.contains("Straight Pipe"));
    REQUIRE(html.contains("<strong>Total</strong>"));
}

TEST_CASE("BomResult byCategory grouping", "[BomGenerator]")
{
    BomResult bom;
    bom.lines.append({"Pipes", "pipe.straight", "Straight Pipe", 2, 0, 10.0, "m", "1\" SCH-40S"});
    bom.lines.append({"Valves", "valve.gate", "Gate Valve", 1, 8.0, 8.0, "pcs", "304L SS"});
    bom.lines.append({"Pipes", "pipe.elbow", "90deg Elbow", 3, 0.5, 1.5, "pcs", "304L SS"});

    auto grouped = bom.byCategory();
    REQUIRE(grouped["Pipes"].size() == 2);
    REQUIRE(grouped["Valves"].size() == 1);
}
