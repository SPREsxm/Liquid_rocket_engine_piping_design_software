#pragma once

#include <QList>
#include <QMap>
#include <QString>

class BlockScene;

struct BomLine {
    QString category;
    QString typeId;
    QString displayName;
    int     quantity = 0;
    double  unitWeight = 0.0;   // kg per unit or kg/m for pipes
    double  totalWeight = 0.0;  // kg
    QString unit;               // "m", "pcs"
    QString specification;
    QString remark;
};

struct BomResult {
    QList<BomLine> lines;
    double totalWeight = 0.0;   // kg

    QMap<QString, QList<BomLine>> byCategory() const {
        QMap<QString, QList<BomLine>> map;
        for (const auto& line : lines)
            map[line.category].append(line);
        return map;
    }
};

BomResult generateBom(BlockScene* scene);
bool exportBomCsv(const BomResult& bom, const QString& filePath);
QString bomToHtmlTable(const BomResult& bom);
