#pragma once

namespace FluidDynamics {

// ─── Propellant types ────────────────────────────────────────

enum class Propellant {
    LOX,     // Liquid oxygen (~90 K)
    RP1,     // Kerosene RP-1 (~298 K)
    Methane, // Liquid methane (~111 K)
    LH2,     // Liquid hydrogen (~20 K)
    Water    // Reference fluid (~293 K)
};

struct PropellantProperties {
    double density;      // kg/m^3
    double viscosity;    // Pa*s (dynamic)
    double bulkModulus;  // Pa
};

PropellantProperties propellantProperties(Propellant p, double temperatureK = -1);

// ─── Core fluid dynamics ─────────────────────────────────────

// Darcy-Weisbach pressure drop: Δp = λ·(L/d)·(ρ·v²/2)
// flowRate is mass flow rate in kg/s; velocity is computed internally.
double calculateDarcyWeisbachPressureDrop(double length, double diameter,
                                          double roughness, double flowRate,
                                          double density, double viscosity);

// Colebrook-White friction factor (iterative).
// For Re < 2300 returns 64/Re (laminar).
// Initial guess from Swamee-Jain explicit formula.
double calculateColebrookWhiteFrictionFactor(double reynolds,
                                             double roughness, double diameter,
                                             int maxIter = 100);

// Local resistance (minor loss): Δp = ζ·(ρ·v²/2)
double calculateLocalResistanceLoss(double zeta, double velocity, double density);

// Joukowsky water-hammer pressure surge: Δp = ρ·c·Δv
double calculateJoukowskyWaterhammer(double velocityChange,
                                     double waveSpeed, double density);

// Equivalent length: Le = (Le/D) * D
double calculateEquivalentLength(double diameter, double leOverD);

// Convenience: Darcy-Weisbach using a propellant type (auto-lookup properties)
double calculatePipePressureDrop(double length, double diameter,
                                 double roughness, double massFlowRate,
                                 Propellant propellant);

// ─── Gas dynamics (isentropic flow) ────────────────────────────

namespace GasDynamics {

// Speed coefficient: λ = v / a*  (a* = critical speed of sound)
double speedCoefficient(double mach, double gamma = 1.4);

// Mach number from speed coefficient
double machFromSpeedCoefficient(double lambda, double gamma = 1.4);

// Pressure ratio: π(λ) = p / p₀
double pressureRatio(double mach, double gamma = 1.4);

// Temperature ratio: τ(λ) = T / T₀
double temperatureRatio(double mach, double gamma = 1.4);

// Density ratio: ε(λ) = ρ / ρ₀
double densityRatio(double mach, double gamma = 1.4);

// Dimensionless mass flow density: q(λ) = A*/A
double flowFunction(double mach, double gamma = 1.4);

// Area ratio for isentropic nozzle: A/A* from Mach number
double areaRatio(double mach, double gamma = 1.4);

// Mach number from area ratio A/A* (subsonic or supersonic branch)
double machFromAreaRatio(double areaRatio, bool supersonic = true, double gamma = 1.4);

// Critical pressure ratio: (2/(γ+1))^(γ/(γ-1))
double criticalPressureRatio(double gamma = 1.4);

// Thrust coefficient of a nozzle
// pc: chamber pressure (Pa), pe: exit pressure (Pa), pa: ambient pressure (Pa)
// Ae: exit area (m²), At: throat area (m²), gamma: specific heat ratio
double thrustCoefficient(double pc, double pe, double pa,
                         double Ae, double At, double gamma = 1.2);

// Mass flow through a nozzle throat (choked flow)
// pc: chamber pressure (Pa), At: throat area (m²), Tc: chamber temperature (K)
// gamma: specific heat ratio, R: gas constant (J/kg·K)
double chokedMassFlow(double pc, double At, double Tc,
                      double gamma = 1.2, double R = 355.0);

} // namespace GasDynamics

} // namespace FluidDynamics
