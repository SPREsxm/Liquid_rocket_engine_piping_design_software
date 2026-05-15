#pragma once

#include <cmath>
#include <algorithm>
#include <QString>

// Fluid-structure interaction for pipe systems.
// Covers stress analysis, Korteweg coupled wave speed, and safety factors.

namespace FluidStructureInteraction {

struct PipeMechanics {
    double innerDiameter_m;
    double wallThickness_m;
    double youngsModulus_Pa;
    double poissonRatio;
    double yieldStrength_Pa;
    double materialDensity_kgpm3;
};

struct FluidCoupling {
    double pressure_Pa;
    double bulkModulus_Pa;
    double density_kgpm3;
};

struct StressResult {
    double hoopStress_Pa;
    double longitudinalStress_Pa;
    double vonMisesStress_Pa;
    double safetyFactor;
    double kortevegWaveSpeed_mps;
    double radialDeflection_m;
    bool yieldExceeded;
};

// Hoop stress (thin-walled cylinder, Barlow's formula)
// σ_h = P·D / (2·e)
inline double hoopStress(double pressure_Pa, double diameter_m, double wallThickness_m)
{
    if (wallThickness_m <= 0.0) return 1e30;
    return pressure_Pa * diameter_m / (2.0 * wallThickness_m);
}

// Longitudinal stress from pressure end-cap load
// σ_l = P·D / (4·e)
inline double longitudinalStress(double pressure_Pa, double diameter_m,
                                  double wallThickness_m)
{
    if (wallThickness_m <= 0.0) return 1e30;
    return pressure_Pa * diameter_m / (4.0 * wallThickness_m);
}

// Von Mises equivalent stress (plane stress, σ_z ≈ 0)
// σ_vm = sqrt(σ_h² + σ_l² - σ_h·σ_l)
inline double vonMisesPlaneStress(double sigmaHoop_Pa, double sigmaLong_Pa)
{
    double h2 = sigmaHoop_Pa * sigmaHoop_Pa;
    double l2 = sigmaLong_Pa * sigmaLong_Pa;
    double hl = sigmaHoop_Pa * sigmaLong_Pa;
    return std::sqrt(h2 + l2 - hl);
}

// Safety factor against yield
inline double safetyFactorAgainstYield(double yieldStrength_Pa, double vonMises_Pa)
{
    if (vonMises_Pa <= 0.0) return 1e10;
    return yieldStrength_Pa / vonMises_Pa;
}

// Korteweg coupled wave speed (water hammer with fluid-structure interaction)
// c = sqrt(K/ρ) / sqrt(1 + (K/E)·(D/e)·C)
// where C = 1 - ν² for anchored pipe (thick-walled correction),
//       C = 1 - ν/2 for anchored against longitudinal movement,
//       C = 1 for free expansion
inline double kortevegWaveSpeed(double bulkModulus_Pa, double fluidDensity_kgpm3,
                                 double youngsModulus_Pa, double innerDiameter_m,
                                 double wallThickness_m,
                                 double poissonRatio = 0.3,
                                 bool anchoredPipe = true)
{
    if (fluidDensity_kgpm3 <= 0.0 || bulkModulus_Pa <= 0.0
        || youngsModulus_Pa <= 0.0 || wallThickness_m <= 0.0)
        return 0.0;

    double c0 = std::sqrt(bulkModulus_Pa / fluidDensity_kgpm3); // uncoupled wave speed
    double D_over_e = innerDiameter_m / wallThickness_m;
    double K_over_E = bulkModulus_Pa / youngsModulus_Pa;

    // Anchoring condition factor
    double C;
    if (anchoredPipe) {
        C = 1.0 - poissonRatio * poissonRatio;  // anchored at both ends, no axial strain
    } else {
        C = 1.0 - poissonRatio / 2.0;           // anchored at one end only
    }

    double coupling = 1.0 + K_over_E * D_over_e * C;
    return c0 / std::sqrt(coupling);
}

// Radial deflection due to internal pressure (thin-walled)
// δ = P·D²·(1 - ν/2) / (2·E·e)
inline double radialDeflection(double pressure_Pa, double diameter_m,
                                double wallThickness_m, double youngsModulus_Pa,
                                double poissonRatio)
{
    if (youngsModulus_Pa <= 0.0 || wallThickness_m <= 0.0) return 0.0;
    double D2 = diameter_m * diameter_m;
    double factor = 1.0 - poissonRatio / 2.0;
    return pressure_Pa * D2 * factor / (2.0 * youngsModulus_Pa * wallThickness_m);
}

// Compute all stresses and coupling in one call
inline StressResult computeStresses(const PipeMechanics& pipe,
                                     const FluidCoupling& fluid)
{
    StressResult r{};

    r.hoopStress_Pa = hoopStress(fluid.pressure_Pa, pipe.innerDiameter_m,
                                  pipe.wallThickness_m);
    r.longitudinalStress_Pa = longitudinalStress(fluid.pressure_Pa, pipe.innerDiameter_m,
                                                   pipe.wallThickness_m);
    r.vonMisesStress_Pa = vonMisesPlaneStress(r.hoopStress_Pa, r.longitudinalStress_Pa);
    r.safetyFactor = safetyFactorAgainstYield(pipe.yieldStrength_Pa, r.vonMisesStress_Pa);
    r.yieldExceeded = r.vonMisesStress_Pa > pipe.yieldStrength_Pa;

    r.kortevegWaveSpeed_mps = kortevegWaveSpeed(
        fluid.bulkModulus_Pa, fluid.density_kgpm3,
        pipe.youngsModulus_Pa, pipe.innerDiameter_m,
        pipe.wallThickness_m, pipe.poissonRatio, true);

    r.radialDeflection_m = radialDeflection(
        fluid.pressure_Pa, pipe.innerDiameter_m,
        pipe.wallThickness_m, pipe.youngsModulus_Pa, pipe.poissonRatio);

    return r;
}

// Material property database for common rocket engine materials
struct MaterialProps {
    double youngsModulus_Pa;
    double yieldStrength_Pa;
    double poissonRatio;
    double density_kgpm3;
};

inline MaterialProps material316L()   { return {2.0e11,  2.9e8,  0.30, 8000.0}; }
inline MaterialProps materialAl2219() { return {7.1e10,  3.5e8,  0.33, 2840.0}; }
inline MaterialProps materialInconel718() { return {2.05e11, 1.1e9, 0.29, 8190.0}; }
inline MaterialProps materialCopper()  { return {1.17e11, 2.1e8,  0.34, 8960.0}; }
inline MaterialProps materialTitanium(){ return {1.10e11, 8.8e8,  0.34, 4430.0}; }

// Look up material by name string
inline MaterialProps materialByName(const QString& name)
{
    QString n = name.toLower();
    if (n.contains("316") || n.contains("316l")) return material316L();
    if (n.contains("2219") || n.contains("aluminum")) return materialAl2219();
    if (n.contains("inconel") || n.contains("718")) return materialInconel718();
    if (n.contains("copper") || n.contains("cu")) return materialCopper();
    if (n.contains("titanium") || n.contains("ti")) return materialTitanium();
    return material316L(); // default
}

} // namespace FluidStructureInteraction
