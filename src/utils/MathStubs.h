#pragma once

// Placeholder calculation functions for MVP.
// These will be replaced with real ExprTk/Eigen implementations in future versions.

namespace MathStubs {

double calculatePressureDrop(double length, double diameter,
                             double roughness, double flowRate,
                             double density, double viscosity);

double calculateReynoldsNumber(double velocity, double diameter,
                               double density, double viscosity);

double calculateFrictionFactor(double reynolds, double roughness, double diameter);

double calculateWaveSpeed(double bulkModulus, double density,
                          double youngsModulus, double diameter,
                          double wallThickness);

} // namespace MathStubs
