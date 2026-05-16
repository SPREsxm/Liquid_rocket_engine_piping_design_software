#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QStringList>

// Lightweight QPainter-based chart widget.
// No dependency on QtCharts — can be swapped to QChart later.
class PaintChartWidget : public QWidget {
    Q_OBJECT
public:
    enum ChartType { BarChart, LineChart, TornadoChart };

    explicit PaintChartWidget(QWidget* parent = nullptr);

    void setChartType(ChartType type);
    void setData(const QVector<QPointF>& series);            // single series
    void setMultiSeries(const QVector<QVector<QPointF>>& series,
                        const QStringList& labels);          // overlaid lines
    void setTornadoData(const QStringList& labels,
                        const QVector<double>& negImpacts,
                        const QVector<double>& posImpacts);  // tornado chart
    void setXLabel(const QString& label);
    void setYLabel(const QString& label);
    void setTitle(const QString& title);

    QSize minimumSizeHint() const override { return QSize(200, 150); }
    QSize sizeHint() const override { return QSize(400, 250); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void drawBarChart(QPainter& p, const QRect& plotArea,
                      double xMin, double xMax, double yMin, double yMax);
    void drawLineChart(QPainter& p, const QRect& plotArea,
                       double xMin, double xMax, double yMin, double yMax);
    void drawTornadoChart(QPainter& p, const QRect& plotArea);
    void drawAxes(QPainter& p, const QRect& plotArea,
                  double xMin, double xMax, double yMin, double yMax);

    ChartType m_chartType = BarChart;
    QVector<QPointF> m_data;
    QVector<QVector<QPointF>> m_multiData;
    QStringList m_seriesLabels;
    QStringList m_tornadoLabels;
    QVector<double> m_tornadoNegImpacts;
    QVector<double> m_tornadoPosImpacts;
    QString m_xLabel, m_yLabel, m_title;

    static constexpr int kLeftMargin = 56;
    static constexpr int kBottomMargin = 36;
    static constexpr int kTopMargin = 28;
    static constexpr int kRightMargin = 16;
};
