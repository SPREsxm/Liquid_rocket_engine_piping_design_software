#include "BomGenerator.h"
#include "PipeScheduleDatabase.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentDescriptor.h"

#include <QFile>
#include <QTextStream>

// Default unit weights for common component types (kg/pc, approximate)
static double defaultWeight(const QString& typeId)
{
    if (typeId.startsWith("valve.")) {
        if (typeId.contains("main") || typeId.contains("gate") || typeId.contains("globe")) return 8.0;
        if (typeId.contains("ball") || typeId.contains("butterfly")) return 3.5;
        if (typeId.contains("check") || typeId.contains("solenoid")) return 2.0;
        if (typeId.contains("relief") || typeId.contains("purge")) return 1.5;
        if (typeId.contains("vent") || typeId.contains("fill")) return 0.8;
        return 2.0;
    }
    if (typeId.startsWith("pump.") || typeId.startsWith("turbopump."))
        return 15.0;
    if (typeId.startsWith("tank.") || typeId.startsWith("press."))
        return 25.0;
    if (typeId.startsWith("sensor."))
        return 0.5;
    if (typeId.startsWith("chamber.") || typeId.startsWith("engine."))
        return 10.0;
    if (typeId.startsWith("pipe.") && !typeId.contains("straight"))
        return 1.0; // fittings
    if (typeId.startsWith("transfer."))
        return 0.8;
    if (typeId.startsWith("safety."))
        return 0.6;
    if (typeId.contains("heat_exchanger"))
        return 20.0;
    return 1.0;
}

static QString buildSpec(BlockItem* block)
{
    QStringList parts;
    QVariant mat = block->propertyValue("material");
    if (mat.isValid() && !mat.toString().isEmpty())
        parts.append(mat.toString());

    // Try NPS + Schedule for pipes
    QVariant nps = block->propertyValue("nps");
    QVariant sch = block->propertyValue("schedule");
    if (nps.isValid() && sch.isValid()) {
        double npsVal = nps.toDouble();
        QString schStr = sch.toString();
        if (npsVal > 0.0 && schStr != "Custom") {
            parts.append(PipeScheduleDatabase::instance().npsToDisplayName(npsVal));
            parts.append("SCH-" + schStr);
        }
    }

    return parts.join(", ");
}

BomResult generateBom(BlockScene* scene)
{
    BomResult result;
    if (!scene) return result;

    // Aggregate by (category, typeId, specification)
    struct Key {
        QString cat, typeId, spec;
        bool operator<(const Key& o) const {
            if (cat != o.cat) return cat < o.cat;
            if (typeId != o.typeId) return typeId < o.typeId;
            return spec < o.spec;
        }
    };
    struct Agg {
        QString displayName;
        int count = 0;
        double lengthSum = 0.0;  // for pipes
        double weightSum = 0.0;
    };
    QMap<Key, Agg> groups;

    for (auto* block : scene->allBlocks()) {
        QString spec = buildSpec(block);
        Key key{block->category(), block->typeId(), spec};
        auto& agg = groups[key];
        agg.displayName = block->displayName();
        agg.count++;

        bool isPipe = block->typeId() == "pipe.straight";

        if (isPipe) {
            QVariant lenVar = block->propertyValue("length");
            double length = lenVar.isValid() ? lenVar.toDouble() : 1.0;
            agg.lengthSum += length;

            // Weight from PipeScheduleDatabase
            QVariant nps = block->propertyValue("nps");
            QVariant sch = block->propertyValue("schedule");
            double kgPerM = 0.0;
            if (nps.isValid() && sch.isValid()) {
                double npsVal = nps.toDouble();
                QString schStr = sch.toString();
                if (npsVal > 0.0 && schStr != "Custom") {
                    auto* entry = PipeScheduleDatabase::instance().lookup(npsVal, schStr);
                    if (entry) kgPerM = entry->weightPerMeter;
                }
            }
            agg.weightSum += kgPerM * length;
        } else {
            agg.weightSum += defaultWeight(block->typeId());
        }
    }

    // Convert to sorted BomLines
    QList<Key> sortedKeys = groups.keys();
    std::sort(sortedKeys.begin(), sortedKeys.end());

    for (const auto& key : sortedKeys) {
        const auto& agg = groups[key];
        BomLine line;
        line.category = key.cat;
        line.typeId = key.typeId;
        line.displayName = agg.displayName;
        line.specification = key.spec;
        line.quantity = agg.count;

        bool isPipe = key.typeId == "pipe.straight";
        if (isPipe) {
            line.unit = "m";
            if (agg.lengthSum > 0.0)
                line.unitWeight = agg.weightSum / agg.lengthSum; // average kg/m
            line.totalWeight = agg.weightSum;
            line.remark = QString("Total length: %1 m").arg(agg.lengthSum, 0, 'f', 2);
        } else {
            line.unit = "pcs";
            line.unitWeight = agg.weightSum / agg.count; // average kg/pc
            line.totalWeight = agg.weightSum;
        }

        result.lines.append(line);
        result.totalWeight += line.totalWeight;
    }

    return result;
}

bool exportBomCsv(const BomResult& bom, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "Category,Type ID,Name,Qty,Unit,Unit Weight (kg),Total Weight (kg),Specification,Remark\n";

    for (const auto& line : bom.lines) {
        auto esc = [](const QString& s) {
            if (s.contains(',') || s.contains('"'))
                return '"' + QString(s).replace('"', "\"\"") + '"';
            return s;
        };
        out << esc(line.category) << ','
            << esc(line.typeId) << ','
            << esc(line.displayName) << ','
            << line.quantity << ','
            << esc(line.unit) << ','
            << line.unitWeight << ','
            << line.totalWeight << ','
            << esc(line.specification) << ','
            << esc(line.remark) << '\n';
    }

    out << "\nTotal Weight (kg)," << bom.totalWeight << '\n';
    file.close();
    return true;
}

QString bomToHtmlTable(const BomResult& bom)
{
    QString html;
    QTextStream out(&html);
    out << "<h2>Bill of Materials</h2>\n";
    out << "<table class=\"bom-table\">\n";
    out << "<thead><tr><th>Category</th><th>Component</th><th>Qty</th>"
        << "<th>Unit</th><th>Unit Wt (kg)</th><th>Total Wt (kg)</th>"
        << "<th>Specification</th><th>Remark</th></tr></thead>\n<tbody>\n";

    for (const auto& line : bom.lines) {
        out << "<tr>"
            << "<td>" << line.category << "</td>"
            << "<td>" << line.displayName << "</td>"
            << "<td class=\"num\">" << line.quantity << "</td>"
            << "<td>" << line.unit << "</td>"
            << "<td class=\"num\">" << line.unitWeight << "</td>"
            << "<td class=\"num\">" << line.totalWeight << "</td>"
            << "<td>" << line.specification << "</td>"
            << "<td>" << line.remark << "</td>"
            << "</tr>\n";
    }

    out << "</tbody>\n<tfoot><tr><td colspan=\"5\"><strong>Total</strong></td>"
        << "<td class=\"num\"><strong>" << bom.totalWeight << "</strong></td>"
        << "<td colspan=\"2\">kg</td></tr></tfoot>\n</table>\n";

    return html;
}
