#pragma once

#include <QString>
#include <QStringList>
#include <vector>
#include <optional>

// ANSI/ASME B36.10 (Carbon/Alloy Steel) and B36.19 (Stainless Steel)
// pipe schedule database for rocket engine piping design.
// Dimensions from ASME B36.10-2022 and B36.19-2022.

struct PipeScheduleEntry {
    QString   schedule;       // "5S", "10S", "40S", "80S", "STD", "XS", "XXS"
    double    wallThickness;  // mm
    double    innerDiameter;  // mm (derived)
    double    weightPerMeter; // kg/m (304L SS at 8000 kg/m³)
};

struct PipeSizeEntry {
    double    nominalDiameter; // inches (NPS)
    QString   dn;              // DN equivalent
    double    outerDiameter;   // mm
    std::vector<PipeScheduleEntry> schedules;
};

class PipeScheduleDatabase {
public:
    static const PipeScheduleDatabase& instance();

    QStringList availableSizes() const;           // decimal strings like "0.25", "0.5"
    QStringList availableSizeNames() const;       // fraction strings like "1/4\"", "1/2\""
    QString     npsToDisplayName(double nps) const;
    QStringList schedulesForSize(double nps) const;
    QString     defaultSchedule(double nps) const;

    const PipeSizeEntry*     sizeEntry(double nps) const;
    const PipeScheduleEntry* lookup(double nps, const QString& schedule) const;

    // Convenience: get OD/ID/wall for a given NPS+schedule combo
    std::optional<double> outerDiameter(double nps, const QString& schedule) const;
    std::optional<double> innerDiameter(double nps, const QString& schedule) const;
    std::optional<double> wallThickness(double nps, const QString& schedule) const;

private:
    PipeScheduleDatabase();
    void addSize(double nps, const QString& dn, double od,
                 const std::vector<std::pair<QString, double>>& schedules);

    std::vector<PipeSizeEntry> m_sizes;
};
