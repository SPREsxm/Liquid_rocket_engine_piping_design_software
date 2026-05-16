#pragma once

#include "utils/NetworkSolver.h"

#include <QVector>
#include <QString>

// Per-edge thermal and structural post-processing results.
struct ThermalStressEdge {
    QUuid  sourceUuid;
    QUuid  destUuid;
    double reynoldsNumber = 0.0;
    double prandtlNumber = 0.0;
    double nusseltNumber = 0.0;
    double heatTransferCoeff_Wpm2K = 0.0; // W/(m²·K)
    double hoopStress_Pa = 0.0;
    double longitudinalStress_Pa = 0.0;
    double vonMisesStress_Pa = 0.0;
    double safetyFactor = 999.0;
    bool   yieldExceeded = false;
    double kortevegWaveSpeed_mps = 0.0;
    QString materialUsed;
};

struct ThermalStressResult {
    QVector<ThermalStressEdge> edges;
    double minSafetyFactor = 999.0;
    int    edgesWithYieldExceeded = 0;
    double avgHeatTransferCoeff = 0.0;
};

// Compute convective heat transfer + FSI stress for every edge in the solution.
// Requires a solved network (solution.converged == true).
ThermalStressResult computeThermalStress(
    class BlockScene* scene,
    const NetworkSolution& solution,
    const struct SolverSettings& settings);
