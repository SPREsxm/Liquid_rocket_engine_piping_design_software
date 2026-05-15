#pragma once

// Analytical helper functions. Delegates to FluidDynamics for fluid-specific calculations.

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
