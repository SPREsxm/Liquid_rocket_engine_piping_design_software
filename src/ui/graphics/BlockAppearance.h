#pragma once

#include <QColor>
#include <QString>
#include <Qt>

namespace BlockAppearance {

constexpr qreal BLOCK_WIDTH       = 140.0;
constexpr qreal BLOCK_MIN_HEIGHT  = 60.0;
constexpr qreal HEADER_HEIGHT     = 24.0;
constexpr qreal BLOCK_CORNER      = 6.0;
constexpr qreal PORT_RADIUS       = 6.0;
constexpr qreal PORT_HOVER_RADIUS = 8.0;
constexpr qreal CONNECTION_WIDTH  = 2.5;
constexpr qreal GRID_SIZE         = 20.0;

inline QColor bodyColor()          { return QColor("#FAFAFA"); }
inline QColor borderColor()        { return QColor("#BDBDBD"); }
inline QColor selectedBorderColor(){ return QColor("#1976D2"); }
inline QColor textColor()          { return QColor("#212121"); }
inline QColor headerTextColor()    { return QColor("#FFFFFF"); }

inline QColor headerColor(const QString& category)
{
    if (category == "Pipes")    return QColor("#1565C0");   // Blue
    if (category == "Valves")   return QColor("#C62828");   // Red
    if (category == "Pumps")    return QColor("#2E7D32");   // Green
    if (category == "Sensors")  return QColor("#E65100");   // Orange
    if (category == "Tanks")    return QColor("#6A1B9A");   // Purple
    return QColor("#546E7A");                               // Grey
}

inline QColor portFillColor()
{
    return QColor("#FFFFFF");
}

inline QColor portBorderColor()
{
    return QColor("#424242");
}

inline QColor portHighlightColor()
{
    return QColor("#00C853");   // Green glow when valid connection target
}

inline QColor connectionColor()
{
    return QColor("#424242");
}

inline QColor tempConnectionColor()
{
    return QColor("#9E9E9E");
}

// Flow-color gradient: blue (low) → cyan → green → yellow → red (high)
inline QColor flowColor(double ratio)
{
    ratio = qBound(0.0, ratio, 1.0);
    if (ratio < 0.33)
        return QColor::fromRgbF(ratio * 3.0, 0.5 + ratio * 1.5, 1.0);
    if (ratio < 0.67)
        return QColor::fromRgbF(1.0, 1.0 - (ratio - 0.33) * 3.0, 0.0);
    return QColor::fromRgbF(1.0 - (ratio - 0.67) * 1.5, 0.0, 0.0);
}

// Flow width: 2.5 (no flow) → 7.5 (max flow)
inline qreal flowWidth(double ratio)
{
    return 2.5 + ratio * 5.0;
}

inline QColor pressureTextColor()
{
    return QColor("#B71C1C");
}

} // namespace BlockAppearance
