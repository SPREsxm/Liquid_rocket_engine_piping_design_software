#pragma once

#include <QWidget>

// Simple color legend showing flow-rate and pressure color scales.
class LegendWidget : public QWidget {
    Q_OBJECT
public:
    explicit LegendWidget(QWidget* parent = nullptr);

    void setFlowRange(double minFlow, double maxFlow);
    void setPressureRange(double minPressure, double maxPressure);

    QSize minimumSizeHint() const override { return QSize(180, 100); }
    QSize sizeHint() const override { return QSize(200, 120); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void drawColorBar(QPainter& p, const QRect& barRect,
                      const QColor& lowColor, const QColor& highColor,
                      const QString& label, double minVal, double maxVal);

    double m_minFlow = 0.0, m_maxFlow = 0.0;
    double m_minPressure = 0.0, m_maxPressure = 0.0;
};
