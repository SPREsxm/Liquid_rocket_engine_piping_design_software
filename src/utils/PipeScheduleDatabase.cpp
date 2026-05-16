#include "PipeScheduleDatabase.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    constexpr double kSteelDensity = 8000.0; // kg/m³ for 304L/316L SS

    double computeWeight(double od_mm, double t_mm) {
        // W = π × (OD - t) × t × ρ / 10⁶  kg/m
        return M_PI * (od_mm - t_mm) * t_mm * kSteelDensity * 1e-6;
    }
}

const PipeScheduleDatabase& PipeScheduleDatabase::instance()
{
    static PipeScheduleDatabase db;
    return db;
}

PipeScheduleDatabase::PipeScheduleDatabase()
{
    using S = std::pair<QString, double>;

    // ── ASME B36.19 Stainless Steel Pipe ─────────────────────

    addSize(0.25,  "DN8",   13.7,  {{"5S",1.65}, {"10S",1.85}, {"40S",2.24}, {"80S",3.02}});
    addSize(0.375, "DN10",  17.1,  {{"5S",1.65}, {"10S",1.85}, {"40S",2.31}, {"80S",3.20}});
    addSize(0.5,   "DN15",  21.3,  {{"5S",1.65}, {"10S",2.11}, {"40S",2.77}, {"80S",3.73}});
    addSize(0.75,  "DN20",  26.7,  {{"5S",1.65}, {"10S",2.11}, {"40S",2.87}, {"80S",3.91}});
    addSize(1.0,   "DN25",  33.4,  {{"5S",1.65}, {"10S",2.77}, {"40S",3.38}, {"80S",4.55}});
    addSize(1.25,  "DN32",  42.2,  {{"5S",1.65}, {"10S",2.77}, {"40S",3.56}, {"80S",4.85}});
    addSize(1.5,   "DN40",  48.3,  {{"5S",1.65}, {"10S",2.77}, {"40S",3.68}, {"80S",5.08}});
    addSize(2.0,   "DN50",  60.3,  {{"5S",1.65}, {"10S",2.77}, {"40S",3.91}, {"80S",5.54}});
    addSize(2.5,   "DN65",  73.0,  {{"5S",2.11}, {"10S",3.05}, {"40S",5.16}, {"80S",7.01}});
    addSize(3.0,   "DN80",  88.9,  {{"5S",2.11}, {"10S",3.05}, {"40S",5.49}, {"80S",7.62}});

    // ── ASME B36.10 Carbon/Alloy Steel (additional schedules) ─
    // 1/4" through 3" with STD/XS/XXS for reference
    // We insert only schedules not already covered by B36.19

    auto it = std::find_if(m_sizes.begin(), m_sizes.end(),
                           [](auto& s) { return s.nominalDiameter == 0.25; });
    if (it != m_sizes.end()) {
        it->schedules.push_back({"STD", 2.24,  9.22, computeWeight(13.7, 2.24)});
        it->schedules.push_back({"XS",  3.02,  7.66, computeWeight(13.7, 3.02)});
        it->schedules.push_back({"XXS", 4.78,  4.14, computeWeight(13.7, 4.78)});
    }
    it = std::find_if(m_sizes.begin(), m_sizes.end(),
                      [](auto& s) { return s.nominalDiameter == 0.5; });
    if (it != m_sizes.end()) {
        it->schedules.push_back({"STD", 2.77, 15.76, computeWeight(21.3, 2.77)});
        it->schedules.push_back({"XS",  3.73, 13.84, computeWeight(21.3, 3.73)});
        it->schedules.push_back({"XXS", 7.47,  6.36, computeWeight(21.3, 7.47)});
    }
    it = std::find_if(m_sizes.begin(), m_sizes.end(),
                      [](auto& s) { return s.nominalDiameter == 1.0; });
    if (it != m_sizes.end()) {
        it->schedules.push_back({"STD", 3.38, 26.64, computeWeight(33.4, 3.38)});
        it->schedules.push_back({"XS",  4.55, 24.30, computeWeight(33.4, 4.55)});
        it->schedules.push_back({"XXS", 9.09, 15.22, computeWeight(33.4, 9.09)});
    }
    it = std::find_if(m_sizes.begin(), m_sizes.end(),
                      [](auto& s) { return s.nominalDiameter == 2.0; });
    if (it != m_sizes.end()) {
        it->schedules.push_back({"STD", 3.91, 52.48, computeWeight(60.3, 3.91)});
        it->schedules.push_back({"XS",  5.54, 49.22, computeWeight(60.3, 5.54)});
        it->schedules.push_back({"XXS", 11.07,38.16, computeWeight(60.3, 11.07)});
    }
}

void PipeScheduleDatabase::addSize(double nps, const QString& dn, double od,
                                   const std::vector<std::pair<QString, double>>& schedules)
{
    PipeSizeEntry entry;
    entry.nominalDiameter = nps;
    entry.dn = dn;
    entry.outerDiameter = od;

    for (const auto& [sch, wall] : schedules) {
        PipeScheduleEntry se;
        se.schedule      = sch;
        se.wallThickness = wall;
        se.innerDiameter = od - 2.0 * wall;
        se.weightPerMeter = computeWeight(od, wall);
        entry.schedules.push_back(se);
    }
    m_sizes.push_back(entry);
}

QStringList PipeScheduleDatabase::availableSizes() const
{
    QStringList list;
    for (const auto& s : m_sizes) {
        list.append(QString::number(s.nominalDiameter, 'g', 4));
    }
    return list;
}

QStringList PipeScheduleDatabase::availableSizeNames() const
{
    QStringList list;
    for (const auto& s : m_sizes)
        list.append(npsToDisplayName(s.nominalDiameter));
    return list;
}

QString PipeScheduleDatabase::npsToDisplayName(double nps) const
{
    // Convert decimal NPS to fraction/string display
    if (std::abs(nps - 0.25)  < 0.01) return "1/4\"";
    if (std::abs(nps - 0.375) < 0.01) return "3/8\"";
    if (std::abs(nps - 0.5)   < 0.01) return "1/2\"";
    if (std::abs(nps - 0.75)  < 0.01) return "3/4\"";
    if (std::abs(nps - 1.0)   < 0.01) return "1\"";
    if (std::abs(nps - 1.25)  < 0.01) return "1-1/4\"";
    if (std::abs(nps - 1.5)   < 0.01) return "1-1/2\"";
    if (std::abs(nps - 2.0)   < 0.01) return "2\"";
    if (std::abs(nps - 2.5)   < 0.01) return "2-1/2\"";
    if (std::abs(nps - 3.0)   < 0.01) return "3\"";
    return QString::number(nps, 'g', 4) + "\"";
}

QStringList PipeScheduleDatabase::schedulesForSize(double nps) const
{
    QStringList list;
    const auto* entry = sizeEntry(nps);
    if (!entry) return list;
    for (const auto& sch : entry->schedules)
        list.append(sch.schedule);
    return list;
}

QString PipeScheduleDatabase::defaultSchedule(double nps) const
{
    // For rocket engine piping, 10S is typical for low-pressure
    // and 40S for moderate pressure. Return 40S as safe default.
    const auto* entry = sizeEntry(nps);
    if (!entry) return {};
    for (const auto& sch : entry->schedules) {
        if (sch.schedule == "40S") return "40S";
    }
    return entry->schedules.empty() ? QString{} : entry->schedules.front().schedule;
}

const PipeSizeEntry* PipeScheduleDatabase::sizeEntry(double nps) const
{
    for (const auto& s : m_sizes) {
        if (std::abs(s.nominalDiameter - nps) < 0.001)
            return &s;
    }
    return nullptr;
}

const PipeScheduleEntry* PipeScheduleDatabase::lookup(double nps, const QString& schedule) const
{
    const auto* entry = sizeEntry(nps);
    if (!entry) return nullptr;
    for (const auto& sch : entry->schedules) {
        if (sch.schedule == schedule) return &sch;
    }
    return nullptr;
}

std::optional<double> PipeScheduleDatabase::outerDiameter(double nps, const QString& /*schedule*/) const
{
    const auto* entry = sizeEntry(nps);
    if (!entry) return {};
    return entry->outerDiameter;
}

std::optional<double> PipeScheduleDatabase::innerDiameter(double nps, const QString& schedule) const
{
    const auto* sch = lookup(nps, schedule);
    if (!sch) return {};
    return sch->innerDiameter;
}

std::optional<double> PipeScheduleDatabase::wallThickness(double nps, const QString& schedule) const
{
    const auto* sch = lookup(nps, schedule);
    if (!sch) return {};
    return sch->wallThickness;
}
