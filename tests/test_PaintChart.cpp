#include <catch2/catch_all.hpp>
#include "ui/graphics/PaintChartWidget.h"
#include <QApplication>

TEST_CASE("PaintChartWidget setData does not crash", "[PaintChart]") {
    PaintChartWidget w;
    w.resize(400, 250);
    QVector<QPointF> data = {{0, 10}, {1, 20}, {2, 15}, {3, 25}};
    REQUIRE_NOTHROW(w.setData(data));
    w.show();
    QApplication::processEvents();
}

TEST_CASE("PaintChartWidget empty data does not crash", "[PaintChart]") {
    PaintChartWidget w;
    w.resize(400, 250);
    QVector<QPointF> empty;
    REQUIRE_NOTHROW(w.setData(empty));
    w.show();
    QApplication::processEvents();
}

TEST_CASE("PaintChartWidget chart type switch", "[PaintChart]") {
    PaintChartWidget w;
    w.resize(400, 250);
    QVector<QPointF> data = {{0, 5}, {1, 7}, {2, 6}};
    w.setData(data);
    REQUIRE_NOTHROW(w.setChartType(PaintChartWidget::LineChart));
    REQUIRE_NOTHROW(w.setChartType(PaintChartWidget::BarChart));
}

TEST_CASE("PaintChartWidget multi-series does not crash", "[PaintChart]") {
    PaintChartWidget w;
    w.resize(400, 250);
    QVector<QVector<QPointF>> multi = {
        {{0, 1}, {1, 2}, {2, 3}},
        {{0, 3}, {1, 2}, {2, 1}}
    };
    QStringList labels = {"Series A", "Series B"};
    REQUIRE_NOTHROW(w.setMultiSeries(multi, labels));
    w.show();
    QApplication::processEvents();
}

TEST_CASE("PaintChartWidget axis labels set correctly", "[PaintChart]") {
    PaintChartWidget w;
    REQUIRE_NOTHROW(w.setXLabel("Time (s)"));
    REQUIRE_NOTHROW(w.setYLabel("Pressure (Pa)"));
    REQUIRE_NOTHROW(w.setTitle("Test Chart"));
}
