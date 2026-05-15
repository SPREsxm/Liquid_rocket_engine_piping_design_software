#pragma once

#include "TransientSolver.h"
#include <QString>
#include <vector>
#include <cmath>
#include <algorithm>

// Grid independence verification framework.
// Per architecture doc §2.4: grid independence error ≤ 2% (GCI ≤ 0.02).

namespace GridRefinement {

struct RefinementResult {
    int nCoarse, nMedium, nFine;
    double maxPressureCoarse, maxPressureMedium, maxPressureFine;
    double relativeError_MC;   // |medium - coarse| / medium
    double relativeError_FM;   // |fine - medium| / fine
    double gci;                // Grid Convergence Index (Roache 1994)
    bool passed;               // GCI ≤ 0.02
    QString message;
};

// Grid Convergence Index (Roache, 1994)
// GCI_fine = Fs·|ε| / (r^p - 1)
// ε = (f_fine - f_medium) / f_fine
// r = refinement ratio (typically 2.0)
// p = formal order of accuracy (2.0 for MOC second-order)
// Fs = factor of safety (1.25 for 3 grids, 3.0 for 2 grids)
inline double computeGCI(double /*fCoarse*/, double fMedium, double fFine,
                         double refinementRatio = 2.0,
                         double orderOfAccuracy = 2.0,
                         double factorOfSafety = 1.25)
{
    double epsilon = (fFine - fMedium) / std::max(std::abs(fFine), 1e-30);
    double denom = std::pow(refinementRatio, orderOfAccuracy) - 1.0;
    if (denom <= 0.0) return 1e10;
    return factorOfSafety * std::abs(epsilon) / denom;
}

// Run transient simulation at N, 2N, 4N spatial nodes and check convergence.
inline RefinementResult verifyGridIndependence(
    const NetworkSolution& steady,
    BlockScene* scene,
    double closureTime,
    int baseNodes = 50)
{
    RefinementResult r{};
    TransientSolver solver;

    // Coarse grid (N)
    r.nCoarse = baseNodes;
    auto resultCoarse = solver.simulateWaterHammer(steady, scene, closureTime, r.nCoarse);
    r.maxPressureCoarse = resultCoarse.maxPressure;

    // Medium grid (2N)
    r.nMedium = baseNodes * 2;
    auto resultMedium = solver.simulateWaterHammer(steady, scene, closureTime, r.nMedium);
    r.maxPressureMedium = resultMedium.maxPressure;

    // Fine grid (4N)
    r.nFine = baseNodes * 4;
    auto resultFine = solver.simulateWaterHammer(steady, scene, closureTime, r.nFine);
    r.maxPressureFine = resultFine.maxPressure;

    // Compute error metrics
    r.relativeError_MC = std::abs(r.maxPressureMedium - r.maxPressureCoarse)
                       / std::max(std::abs(r.maxPressureMedium), 1e-30);
    r.relativeError_FM = std::abs(r.maxPressureFine - r.maxPressureMedium)
                       / std::max(std::abs(r.maxPressureFine), 1e-30);
    r.gci = computeGCI(r.maxPressureCoarse, r.maxPressureMedium, r.maxPressureFine);
    r.passed = r.gci <= 0.02;

    r.message = QStringLiteral("Grid: %1/%2/%3 nodes, GCI=%4%, %5")
        .arg(r.nCoarse).arg(r.nMedium).arg(r.nFine)
        .arg(r.gci * 100.0, 0, 'f', 2)
        .arg(r.passed ? QStringLiteral("PASS (≤2%)") : QStringLiteral("FAIL (>2%)"));

    return r;
}

// Richardson extrapolation: estimate exact value
// f_exact ≈ (r^p·f_fine - f_medium) / (r^p - 1)
inline double richardsonExtrapolation(double fMedium, double fFine,
                                       double refinementRatio = 2.0,
                                       double orderOfAccuracy = 2.0)
{
    double rp = std::pow(refinementRatio, orderOfAccuracy);
    return (rp * fFine - fMedium) / (rp - 1.0);
}

} // namespace GridRefinement
