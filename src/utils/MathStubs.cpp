#include "MathStubs.h"
#include "FluidDynamics.h"
#include <QtMath>

namespace MathStubs {

double calculatePressureDrop(double length, double diameter,
                             double roughness, double flowRate,
                             double density, double viscosity)
{
    return FluidDynamics::calculateDarcyWeisbachPressureDrop(
        length, diameter, roughness, flowRate, density, viscosity);
}

double calculateReynoldsNumber(double velocity, double diameter,
                               double density, double viscosity)
{
    if (viscosity <= 0.0) return 0.0;
    return density * velocity * diameter / viscosity;
}

double calculateFrictionFactor(double reynolds, double roughness, double diameter)
{
    return FluidDynamics::calculateColebrookWhiteFrictionFactor(
        reynolds, roughness, diameter);
}

double calculateWaveSpeed(double bulkModulus, double density,
                          double youngsModulus, double diameter,
                          double wallThickness)
{
    if (density <= 0.0 || wallThickness <= 0.0) return 0.0;
    double denom = 1.0 + (bulkModulus / youngsModulus) * (diameter / wallThickness);
    return qSqrt((bulkModulus / density) / denom);
}

} // namespace MathStubs
