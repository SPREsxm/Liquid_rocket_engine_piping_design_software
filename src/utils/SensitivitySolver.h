#pragma once

#include "core/Types.h"
#include "utils/NetworkSolver.h"

#include <QString>
#include <QVector>

struct SweepPoint {
    double paramValue;
    NetworkSolution solution;
};

struct SensitivityResult {
    QString sweptParamName;
    QString sweptParamUnit;
    QVector<SweepPoint> points;

    // Tornado chart data: one bar per parameter showing its impact on output
    struct TornadoBar {
        QString paramName;
        double negativeImpact;  // low-side delta from nominal
        double positiveImpact;  // high-side delta from nominal
    };
    QVector<TornadoBar> tornadoData;
    QString tornadoOutputName;  // "Total Pressure Drop", "Thrust", etc.

    // Extract output series for charting
    QVector<double> totalPressureDropSeries() const;
    QVector<double> maxPressureSeries() const;
    QVector<double> thrustSeries() const;
    QVector<double> ispSeries() const;
};

// Vary a single parameter over [paramMin, paramMax] in `steps` intervals.
// paramKey identifies the parameter to sweep (see SensitivitySolver.cpp).
SensitivityResult runParameterSweep(
    class BlockScene* scene,
    const SolverSettings& baseSettings,
    const QString& paramKey,
    double paramMin, double paramMax,
    int steps = 10,
    double inletPressurePa = 1.0e6,
    double inletMassFlowKgPerS = 10.0);

// Run multiple fixed-condition configurations and compare results side-by-side.
SensitivityResult runMultiCondition(
    class BlockScene* scene,
    const QVector<SolverSettings>& conditions,
    double inletPressurePa = 1.0e6,
    double inletMassFlowKgPerS = 10.0);

// Compute tornado sensitivity data for the given output metric.
// Returns a QVector of TornadoBar sorted by absolute impact (largest first).
QVector<SensitivityResult::TornadoBar> computeTornado(
    class BlockScene* scene,
    const SolverSettings& baseSettings,
    const QStringList& paramKeys,
    const QString& outputMetric,  // "totalPressureDrop", "thrust", "isp"
    double inletPressurePa = 1.0e6,
    double inletMassFlowKgPerS = 10.0);
