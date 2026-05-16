#include "PaintChartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PaintChartWidget::PaintChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void PaintChartWidget::setChartType(ChartType type) { m_chartType = type; update(); }

void PaintChartWidget::setData(const QVector<QPointF>& series) { m_data = series; update(); }

void PaintChartWidget::setMultiSeries(const QVector<QVector<QPointF>>& series,
                                      const QStringList& labels)
{
    m_multiData = series;
    m_seriesLabels = labels;
    update();
}

void PaintChartWidget::setTornadoData(const QStringList& labels,
                                      const QVector<double>& negImpacts,
                                      const QVector<double>& posImpacts)
{
    m_tornadoLabels = labels;
    m_tornadoNegImpacts = negImpacts;
    m_tornadoPosImpacts = posImpacts;
    update();
}

void PaintChartWidget::setXLabel(const QString& label) { m_xLabel = label; update(); }
void PaintChartWidget::setYLabel(const QString& label) { m_yLabel = label; update(); }
void PaintChartWidget::setTitle(const QString& title)  { m_title = title; update(); }

void PaintChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(255, 255, 255));

    QRect plotArea = rect().adjusted(kLeftMargin, kTopMargin, -kRightMargin, -kBottomMargin);
    if (plotArea.width() <= 0 || plotArea.height() <= 0) return;

    // Determine data ranges
    double xMin = 0, xMax = 1, yMin = 0, yMax = 1;

    if (m_chartType == TornadoChart && !m_tornadoNegImpacts.isEmpty()) {
        // Tornado uses its own painting path — no axes needed
        drawTornadoChart(p, plotArea);
        if (!m_title.isEmpty()) {
            p.setPen(Qt::black);
            QFont titleFont = font();
            titleFont.setBold(true);
            titleFont.setPointSize(titleFont.pointSize() + 1);
            p.setFont(titleFont);
            p.drawText(QRect(rect().left(), 2, rect().width(), kTopMargin - 2),
                       Qt::AlignHCenter | Qt::AlignVCenter, m_title);
        }
        return;
    } else if (m_chartType == BarChart && !m_data.isEmpty()) {
        xMin = 0.0;
        xMax = static_cast<double>(m_data.size());
        yMax = 0.0;
        for (const auto& pt : m_data) {
            if (pt.y() > yMax) yMax = pt.y();
        }
        yMax *= 1.12;  // 12% headroom for labels
        if (yMax <= 0.0) yMax = 1.0;
    } else if (m_chartType == LineChart) {
        auto findRanges = [&](const QVector<QPointF>& series) {
            if (!series.isEmpty()) {
                for (const auto& pt : series) {
                    if (pt.x() < xMin) xMin = pt.x();
                    if (pt.x() > xMax) xMax = pt.x();
                    if (pt.y() < yMin) yMin = pt.y();
                    if (pt.y() > yMax) yMax = pt.y();
                }
            }
        };
        if (!m_multiData.isEmpty()) {
            for (const auto& series : m_multiData) findRanges(series);
        } else if (!m_data.isEmpty()) {
            findRanges(m_data);
        }
        // Pad ranges
        double xPad = (xMax - xMin) * 0.05;
        double yPad = (yMax - yMin) * 0.10;
        if (xPad <= 0.0) xPad = 0.5;
        if (yPad <= 0.0) yPad = 1.0;
        xMin -= xPad;
        xMax += xPad;
        yMin -= yPad;
        yMax += yPad;
    }

    drawAxes(p, plotArea, xMin, xMax, yMin, yMax);

    if (m_chartType == BarChart)
        drawBarChart(p, plotArea, xMin, xMax, yMin, yMax);
    else
        drawLineChart(p, plotArea, xMin, xMax, yMin, yMax);

    // Title
    if (!m_title.isEmpty()) {
        p.setPen(Qt::black);
        QFont titleFont = font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 1);
        p.setFont(titleFont);
        p.drawText(QRect(rect().left(), 2, rect().width(), kTopMargin - 2),
                   Qt::AlignHCenter | Qt::AlignVCenter, m_title);
    }
}

void PaintChartWidget::drawAxes(QPainter& p, const QRect& plotArea,
                                 double xMin, double xMax, double yMin, double yMax)
{
    p.setPen(QPen(QColor(80, 80, 80), 1));

    // Y axis
    p.drawLine(plotArea.topLeft(), plotArea.bottomLeft());
    // X axis
    p.drawLine(plotArea.bottomLeft(), plotArea.bottomRight());

    QFont tickFont = font();
    tickFont.setPointSize(qMax(tickFont.pointSize() - 2, 6));
    p.setFont(tickFont);

    // Y axis ticks and labels
    int yTicks = qMin(6, plotArea.height() / 30);
    if (yTicks < 2) yTicks = 2;
    for (int i = 0; i <= yTicks; ++i) {
        double val = yMin + (yMax - yMin) * i / yTicks;
        int y = plotArea.bottom() - static_cast<int>(plotArea.height() * i / yTicks);
        p.drawLine(plotArea.left() - 4, y, plotArea.left(), y);

        QString label;
        if (qAbs(val) < 1e-9)
            label = QStringLiteral("0");
        else if (qAbs(val) >= 1e6)
            label = QString::number(val / 1e6, 'f', 1) + 'M';
        else if (qAbs(val) >= 1e3)
            label = QString::number(val / 1e3, 'f', 1) + 'k';
        else if (qAbs(val) < 0.01)
            label = QString::number(val, 'e', 1);
        else
            label = QString::number(val, 'f', 2);

        p.setPen(QColor(120, 120, 120));
        p.drawText(QRect(0, y - 10, plotArea.left() - 6, 20),
                   Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // X axis ticks and labels
    double xRange = xMax - xMin;
    int xTicks = qMin(8, plotArea.width() / 50);
    if (xTicks < 2) xTicks = 2;
    for (int i = 0; i <= xTicks; ++i) {
        double val = xMin + xRange * i / xTicks;
        int x = plotArea.left() + static_cast<int>(plotArea.width() * i / xTicks);
        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawLine(x, plotArea.bottom(), x, plotArea.bottom() + 4);

        QString label = QString::number(val, 'g', 3);
        p.setPen(QColor(120, 120, 120));
        p.drawText(QRect(x - 25, plotArea.bottom() + 5, 50, kBottomMargin - 5),
                   Qt::AlignHCenter | Qt::AlignTop, label);
    }

    // Axis labels
    if (!m_yLabel.isEmpty()) {
        p.save();
        p.setPen(QColor(60, 60, 60));
        QFont labelFont = font();
        labelFont.setBold(true);
        p.setFont(labelFont);
        p.translate(12, plotArea.center().y());
        p.rotate(-90);
        p.drawText(QRect(-plotArea.height() / 2, -10, plotArea.height(), 20),
                   Qt::AlignCenter, m_yLabel);
        p.restore();
    }
    if (!m_xLabel.isEmpty()) {
        p.setPen(QColor(60, 60, 60));
        QFont labelFont = font();
        labelFont.setBold(true);
        p.setFont(labelFont);
        p.drawText(QRect(plotArea.left(), plotArea.bottom() + kBottomMargin - 14,
                         plotArea.width(), 14),
                   Qt::AlignCenter, m_xLabel);
    }
}

void PaintChartWidget::drawBarChart(QPainter& p, const QRect& plotArea,
                                     double /*xMin*/, double /*xMax*/, double yMin, double yMax)
{
    if (m_data.isEmpty()) return;

    int n = m_data.size();
    double barWidth = static_cast<double>(plotArea.width()) / n * 0.7;
    double gap = static_cast<double>(plotArea.width()) / n * 0.3;
    double yRange = yMax - yMin;
    if (yRange <= 0.0) yRange = 1.0;

    for (int i = 0; i < n; ++i) {
        double ratio = (m_data[i].y() - yMin) / yRange;
        int barH = static_cast<int>(ratio * plotArea.height());
        int x = plotArea.left() + static_cast<int>(i * (barWidth + gap) + gap / 2.0);
        int y = plotArea.bottom() - barH;
        if (barH < 2) barH = 2;

        // Color gradient: blue (low) to red (high)
        double colorRatio = qBound(0.0, ratio, 1.0);
        QColor barColor(
            static_cast<int>(21 + colorRatio * (198 - 21)),     // R: 21→198
            static_cast<int>(101 + (1.0 - colorRatio) * (101 - 40)), // G: 101→40
            static_cast<int>(192 + (1.0 - colorRatio) * (192 - 40))  // B: 192→40
        );

        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(x, y, static_cast<int>(barWidth), barH, 3, 3);

        // Value label above bar
        p.setPen(QColor(60, 60, 60));
        QFont valFont = font();
        valFont.setPointSize(qMax(valFont.pointSize() - 3, 6));
        p.setFont(valFont);
        QString valText;
        double v = m_data[i].y();
        if (qAbs(v) >= 1e6)
            valText = QString::number(v / 1e6, 'f', 2) + "M";
        else if (qAbs(v) >= 1e3)
            valText = QString::number(v / 1e3, 'f', 1) + "k";
        else
            valText = QString::number(v, 'f', 2);
        p.drawText(QRect(x, y - 16, static_cast<int>(barWidth), 14),
                   Qt::AlignHCenter | Qt::AlignBottom, valText);

        // X label (index or point label)
        p.drawText(QRect(x, plotArea.bottom() + 2, static_cast<int>(barWidth), 12),
                   Qt::AlignHCenter | Qt::AlignTop, QString::number(i + 1));
    }
}

void PaintChartWidget::drawLineChart(QPainter& p, const QRect& plotArea,
                                      double xMin, double xMax, double yMin, double yMax)
{
    auto mapPoint = [&](const QPointF& pt) -> QPointF {
        double px = plotArea.left() + (pt.x() - xMin) / (xMax - xMin) * plotArea.width();
        double py = plotArea.bottom() - (pt.y() - yMin) / (yMax - yMin) * plotArea.height();
        return QPointF(px, py);
    };

    // Fixed color palette for multi-series
    static const QColor kPalette[] = {
        QColor(21, 101, 192),   // blue
        QColor(198, 40, 40),    // red
        QColor(46, 125, 50),    // green
        QColor(230, 81, 0),     // orange
        QColor(106, 27, 154),   // purple
        QColor(0, 131, 143),    // teal
    };
    static constexpr int kPaletteSize = 6;

    // Determine which data to draw
    QVector<QVector<QPointF>> allSeries;
    if (!m_multiData.isEmpty())
        allSeries = m_multiData;
    else if (!m_data.isEmpty())
        allSeries.append(m_data);

    for (int s = 0; s < allSeries.size(); ++s) {
        const auto& series = allSeries[s];
        if (series.isEmpty()) continue;

        QColor color = kPalette[s % kPaletteSize];
        p.setPen(QPen(color, 2));
        p.setBrush(Qt::NoBrush);

        QPainterPath path;
        bool first = true;
        for (const auto& pt : series) {
            QPointF mapped = mapPoint(pt);
            if (first) {
                path.moveTo(mapped);
                first = false;
            } else {
                path.lineTo(mapped);
            }
        }
        p.drawPath(path);

        // Circle markers
        p.setPen(Qt::NoPen);
        p.setBrush(color.lighter(130));
        for (const auto& pt : series) {
            QPointF mapped = mapPoint(pt);
            p.drawEllipse(mapped, 2.5, 2.5);
        }
    }

    // Legend (multi-series only)
    if (m_multiData.size() > 1) {
        QFont legendFont = font();
        legendFont.setPointSize(qMax(legendFont.pointSize() - 2, 6));
        p.setFont(legendFont);

        int legendX = plotArea.right() - 120;
        int legendY = plotArea.top() + 4;
        for (int s = 0; s < m_multiData.size() && s < m_seriesLabels.size(); ++s) {
            p.setPen(Qt::NoPen);
            p.setBrush(kPalette[s % kPaletteSize]);
            p.drawRect(legendX, legendY + s * 16, 12, 10);
            p.setPen(QColor(60, 60, 60));
            p.drawText(legendX + 16, legendY + s * 16, 100, 10,
                       Qt::AlignLeft | Qt::AlignVCenter, m_seriesLabels[s]);
        }
    }
}

void PaintChartWidget::drawTornadoChart(QPainter& p, const QRect& plotArea)
{
    int n = qMin(m_tornadoLabels.size(),
                 qMin(m_tornadoNegImpacts.size(), m_tornadoPosImpacts.size()));
    if (n == 0) return;

    // Find max absolute impact for scaling
    double maxImpact = 1.0;
    for (int i = 0; i < n; ++i) {
        double a = qMax(qAbs(m_tornadoNegImpacts[i]), qAbs(m_tornadoPosImpacts[i]));
        if (a > maxImpact) maxImpact = a;
    }
    maxImpact *= 1.15; // 15% margin

    const int barHeight = qMin(22, (plotArea.height() - 8) / n);
    const int gap = 4;
    const double centerX = plotArea.center().x();
    const double scaleX = (plotArea.width() * 0.5 - 40) / maxImpact;

    QFont barFont = font();
    barFont.setPointSize(qMax(barFont.pointSize() - 2, 7));
    p.setFont(barFont);

    for (int i = 0; i < n; ++i) {
        int y = plotArea.top() + i * (barHeight + gap);

        double neg = m_tornadoNegImpacts[i];
        double pos = m_tornadoPosImpacts[i];

        // Negative bar (left of center)
        {
            int barW = static_cast<int>(qAbs(neg) * scaleX);
            QColor color = neg < 0 ? QColor(21, 101, 192) : QColor(198, 40, 40);
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawRoundedRect(static_cast<int>(centerX) - barW, y, barW, barHeight, 2, 2);
        }

        // Positive bar (right of center)
        {
            int barW = static_cast<int>(qAbs(pos) * scaleX);
            QColor color = pos > 0 ? QColor(198, 40, 40) : QColor(21, 101, 192);
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawRoundedRect(static_cast<int>(centerX), y, barW, barHeight, 2, 2);
        }

        // Center line
        p.setPen(QPen(QColor(80, 80, 80), 1, Qt::DashLine));
        p.drawLine(QPointF(centerX, plotArea.top()),
                   QPointF(centerX, plotArea.bottom()));

        // Value labels
        p.setPen(QColor(60, 60, 60));
        QString negLabel = QString::number(neg, 'g', 3);
        QString posLabel = QString::number(pos, 'g', 3);
        p.drawText(QRect(static_cast<int>(centerX) + 4, y, 80, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, posLabel);
        p.drawText(QRect(static_cast<int>(centerX) - 84, y, 80, barHeight),
                   Qt::AlignRight | Qt::AlignVCenter, negLabel);

        // Parameter name (left margin)
        p.setPen(QColor(40, 40, 40));
        QString name = m_tornadoLabels.value(i);
        p.drawText(QRect(plotArea.left(), y, 80, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, name);
    }
}
