#include "LegendWidget.h"
#include "BlockAppearance.h"

#include <QPainter>
#include <QLocale>

LegendWidget::LegendWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(180, 100);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void LegendWidget::setFlowRange(double minFlow, double maxFlow)
{
    m_minFlow = minFlow;
    m_maxFlow = maxFlow;
    update();
}

void LegendWidget::setPressureRange(double minPressure, double maxPressure)
{
    m_minPressure = minPressure;
    m_maxPressure = maxPressure;
    update();
}

void LegendWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int barHeight = 14;
    const int labelW = 48;
    const int margin = 8;
    const int barStartX = margin + labelW + 4;
    const int barWidth = w - barStartX - margin;

    // Flow color bar (top half)
    int flowY = margin + 4;
    QRect flowBarRect(barStartX, flowY, barWidth, barHeight);
    drawColorBar(p, flowBarRect, BlockAppearance::flowColor(0.0),
                 BlockAppearance::flowColor(1.0),
                 QStringLiteral("Flow"), m_minFlow, m_maxFlow);
    p.drawText(QRect(margin, flowY, labelW, barHeight),
               Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("Flow"));

    // Pressure color bar (bottom half)
    int pressY = flowY + barHeight + 14;
    QRect pressBarRect(barStartX, pressY, barWidth, barHeight);
    drawColorBar(p, pressBarRect, BlockAppearance::flowColor(0.0),
                 BlockAppearance::flowColor(1.0),
                 QStringLiteral("Press"), m_minPressure, m_maxPressure);
    p.drawText(QRect(margin, pressY, labelW, barHeight),
               Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("Press"));
}

void LegendWidget::drawColorBar(QPainter& p, const QRect& barRect,
                                const QColor& lowColor, const QColor& highColor,
                                const QString& /*label*/, double minVal, double maxVal)
{
    QLinearGradient grad(barRect.left(), 0, barRect.right(), 0);
    grad.setColorAt(0.0, lowColor);
    grad.setColorAt(1.0, highColor);
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRoundedRect(barRect, 3, 3);

    // Border
    p.setPen(QColor("#9E9E9E"));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(barRect, 3, 3);

    // Min/Max labels
    p.setPen(QColor("#212121"));
    QFont f("Segoe UI", 7);
    p.setFont(f);

    QString minText = QStringLiteral("0");
    QString maxText;
    if (maxVal > 0.0) {
        if (maxVal >= 1.0) {
            minText = QString::number(minVal, 'f', 1);
            maxText = QString::number(maxVal, 'f', 1);
        } else {
            minText = QString::number(minVal, 'g', 2);
            maxText = QString::number(maxVal, 'g', 2);
        }
    }

    QRect minRect(barRect.left() + 4, barRect.bottom() + 2, 60, 14);
    QRect maxRect(barRect.right() - 56, barRect.bottom() + 2, 60, 14);
    p.drawText(minRect, Qt::AlignLeft | Qt::AlignTop, minText);
    p.drawText(maxRect, Qt::AlignRight | Qt::AlignTop, maxText);
}
