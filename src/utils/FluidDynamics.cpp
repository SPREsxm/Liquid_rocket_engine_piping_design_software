#include "FluidDynamics.h"
#include "MathStubs.h"
#include "PropellantProperties.h"

#include <QtMath>
#include <cmath>

namespace FluidDynamics {

// ─── Propellant properties reference table ───────────────────

PropellantProperties propellantProperties(Propellant p, double temperatureK)
{
    // Helper: select critical data and default T
    auto cd = [&]() -> ::PropellantProperties::CriticalData {
        switch (p) {
        case Propellant::LOX:     return ::PropellantProperties::criticalLOX();
        case Propellant::RP1:     return ::PropellantProperties::criticalRP1();
        case Propellant::Methane: return ::PropellantProperties::criticalMethane();
        case Propellant::LH2:     return ::PropellantProperties::criticalLH2();
        case Propellant::Water:   return ::PropellantProperties::criticalWater();
        default:                  return ::PropellantProperties::criticalWater();
        }
    };

    auto defaultT = [&]() -> double {
        switch (p) {
        case Propellant::LOX:     return 90.0;
        case Propellant::RP1:     return 298.0;
        case Propellant::Methane: return 111.0;
        case Propellant::LH2:     return 20.0;
        case Propellant::Water:   return 293.0;
        default:                  return 293.0;
        }
    };

    auto bulkModulus = [&]() -> double {
        switch (p) {
        case Propellant::LOX:     return 1.0e9;
        case Propellant::RP1:     return 1.3e9;
        case Propellant::Methane: return 1.5e9;
        case Propellant::LH2:     return 0.2e9;
        case Propellant::Water:   return 2.2e9;
        default:                  return 2.2e9;
        }
    };

    // Use temperature-dependent formulas when T is provided (> 0)
    // Otherwise fall back to hardcoded reference values
    if (temperatureK > 0.0 && temperatureK < cd().Tc) {
        double density = ::PropellantProperties::temperatureDependentDensity(temperatureK, cd());
        double viscosity = ::PropellantProperties::temperatureDependentViscosity(temperatureK, cd());
        return {density, viscosity, bulkModulus()};
    }

    // Hardcoded reference values at default operating temperature
    switch (p) {
    case Propellant::LOX:
        return {1141.0, 1.96e-4, 1.0e9};
    case Propellant::RP1:
        return {810.0, 1.64e-3, 1.3e9};
    case Propellant::Methane:
        return {422.0, 1.18e-4, 1.5e9};
    case Propellant::LH2:
        return {71.0, 1.3e-5, 0.2e9};
    case Propellant::Water:
    default:
        return {998.0, 1.002e-3, 2.2e9};
    }
}

// ─── Darcy-Weisbach pressure drop ────────────────────────────

double calculateDarcyWeisbachPressureDrop(double length, double diameter,
                                          double roughness, double flowRate,
                                          double density, double viscosity)
{
    if (diameter <= 0.0 || density <= 0.0 || length <= 0.0)
        return 0.0;

    const double area = M_PI * diameter * diameter / 4.0;
    const double velocity = flowRate / (density * area);

    const double re = MathStubs::calculateReynoldsNumber(velocity, diameter,
                                                         density, viscosity);
    const double lambda = calculateColebrookWhiteFrictionFactor(re, roughness,
                                                                diameter);
    // Δp = λ·(L/d)·(ρ·v²/2)
    return lambda * (length / diameter) * (density * velocity * velocity / 2.0);
}

// ─── Colebrook-White friction factor ─────────────────────────

double calculateColebrookWhiteFrictionFactor(double reynolds,
                                             double roughness, double diameter,
                                             int maxIter)
{
    if (reynolds <= 0.0 || diameter <= 0.0)
        return 0.0;

    // Laminar: exact solution
    if (reynolds < 2300.0)
        return 64.0 / reynolds;

    const double relRough = roughness / diameter;

    // Swamee-Jain explicit approximation as initial guess
    // λ = 0.25 / [log₁₀(ε/(3.7d) + 5.74/Re^0.9)]²
    double lambda = 0.25 / qPow(std::log10(relRough / 3.7 + 5.74 / qPow(reynolds, 0.9)), 2.0);

    // Colebrook-White: 1/√λ = -2·log₁₀(ε/(3.7d) + 2.51/(Re·√λ))
    // Solve via fixed-point iteration on 1/√λ
    double invSqrtLambda = 1.0 / qSqrt(lambda);
    for (int i = 0; i < maxIter; ++i) {
        const double rhs = -2.0 * std::log10(
            relRough / 3.7 + 2.51 * invSqrtLambda / reynolds);
        const double diff = rhs - invSqrtLambda;
        invSqrtLambda = rhs;

        if (qAbs(diff) < 1e-10)
            break;
    }

    lambda = 1.0 / (invSqrtLambda * invSqrtLambda);
    return lambda;
}

// ─── Local resistance loss ───────────────────────────────────

double calculateLocalResistanceLoss(double zeta, double velocity, double density)
{
    // Δp = ζ·(ρ·v²/2)
    return zeta * density * velocity * velocity / 2.0;
}

// ─── Joukowsky water-hammer ──────────────────────────────────

double calculateJoukowskyWaterhammer(double velocityChange,
                                     double waveSpeed, double density)
{
    // Δp = ρ·c·Δv
    if (density <= 0.0 || waveSpeed <= 0.0)
        return 0.0;
    return density * waveSpeed * qAbs(velocityChange);
}

// ─── Equivalent length ───────────────────────────────────────

double calculateEquivalentLength(double diameter, double leOverD)
{
    return leOverD * diameter;
}

// ─── Convenience overload ────────────────────────────────────

double calculatePipePressureDrop(double length, double diameter,
                                 double roughness, double massFlowRate,
                                 Propellant propellant)
{
    const auto props = propellantProperties(propellant);
    return calculateDarcyWeisbachPressureDrop(length, diameter, roughness,
                                              massFlowRate,
                                              props.density, props.viscosity);
}

// ─── Gas dynamics (isentropic flow) ────────────────────────────

namespace GasDynamics {

double speedCoefficient(double mach, double gamma) {
    if (mach < 0.0) return 0.0;
    double gp1 = gamma + 1.0;
    double gm1 = gamma - 1.0;
    return mach * std::sqrt(gp1 / (2.0 + gm1 * mach * mach));
}

double machFromSpeedCoefficient(double lambda, double gamma) {
    if (lambda < 0.0) return 0.0;
    double gp1 = gamma + 1.0;
    double gm1 = gamma - 1.0;
    double lambda2 = lambda * lambda;
    // λ_max = sqrt((γ+1)/(γ-1))
    double lambdaMax2 = gp1 / gm1;
    if (lambda2 >= lambdaMax2 * 0.999) return 1e6; // infinite Mach
    return std::sqrt(2.0 * lambda2 / (gp1 - gm1 * lambda2));
}

double pressureRatio(double mach, double gamma) {
    if (mach < 0.0) return 1.0;
    double gm1 = gamma - 1.0;
    return std::pow(1.0 + 0.5 * gm1 * mach * mach, -gamma / gm1);
}

double temperatureRatio(double mach, double gamma) {
    if (mach < 0.0) return 1.0;
    double gm1 = gamma - 1.0;
    return 1.0 / (1.0 + 0.5 * gm1 * mach * mach);
}

double densityRatio(double mach, double gamma) {
    if (mach < 0.0) return 1.0;
    double gm1 = gamma - 1.0;
    return std::pow(1.0 + 0.5 * gm1 * mach * mach, -1.0 / gm1);
}

double flowFunction(double mach, double gamma) {
    if (mach < 0.0) return 0.0;
    double gm1 = gamma - 1.0;
    double gp1 = gamma + 1.0;
    double term = 1.0 + 0.5 * gm1 * mach * mach;
    double exponent = gp1 / (2.0 * gm1);
    return mach * std::pow(gp1 / (2.0 * term), exponent);
}

double areaRatio(double mach, double gamma) {
    if (mach < 0.01) return 1e6;
    // A/A* = 1/M * [(2/(γ+1))·(1 + (γ-1)/2·M²)]^((γ+1)/(2(γ-1)))
    double gm1 = gamma - 1.0;
    double gp1 = gamma + 1.0;
    double term = 1.0 + 0.5 * gm1 * mach * mach;
    double exponent = gp1 / (2.0 * gm1);
    return (1.0 / mach) * std::pow(2.0 * term / gp1, exponent);
}

double machFromAreaRatio(double aRatio, bool supersonic, double gamma) {
    // Newton iteration: M_{n+1} = M_n - f(M_n)/f'(M_n)
    // where f(M) = A/A*(M) - targetA
    if (aRatio < 1.0) return 0.0; // invalid (A/A* >= 1)

    double gm1 = gamma - 1.0;
    double gp1 = gamma + 1.0;

    // Initial guess
    double mach;
    if (supersonic) {
        // Approximate for large M
        mach = std::pow(aRatio * std::pow(gp1 / 2.0, gp1 / (2.0 * gm1)), gm1 / gp1);
        if (mach < 1.5) mach = 2.0;
    } else {
        // Subsonic: approximate
        mach = 1.0 / (aRatio * std::pow(2.0 / gp1, gp1 / (2.0 * gm1)));
        if (mach > 0.8) mach = 0.5;
    }

    // Newton iteration
    for (int iter = 0; iter < 50; ++iter) {
        double term = 1.0 + 0.5 * gm1 * mach * mach;
        double exponent = gp1 / (2.0 * gm1);
        double f = (1.0 / mach) * std::pow(2.0 * term / gp1, exponent) - aRatio;

        // Derivative: d/dM of A/A*
        double df = std::pow(2.0 * term / gp1, exponent) *
                    (mach * mach * gm1 * exponent / (term) - 1.0 / (mach * mach));

        double dm = f / df;
        mach -= dm;
        if (std::abs(dm) < 1e-10) break;
    }
    return mach;
}

double criticalPressureRatio(double gamma) {
    return std::pow(2.0 / (gamma + 1.0), gamma / (gamma - 1.0));
}

double thrustCoefficient(double pc, double pe, double pa,
                         double Ae, double At, double gamma) {
    // CF = sqrt[ 2γ²/(γ-1) · (2/(γ+1))^((γ+1)/(γ-1)) · (1 - (pe/pc)^((γ-1)/γ)) ]
    //      + (pe - pa)/pc · Ae/At
    if (pc <= 0.0 || At <= 0.0) return 0.0;
    if (Ae < At) return 0.0;

    double gm1 = gamma - 1.0;
    double gp1 = gamma + 1.0;

    double term1 = 2.0 * gamma * gamma / gm1;
    double term2 = std::pow(2.0 / gp1, gp1 / gm1);
    double pr = pe / pc;
    double term3 = 1.0 - std::pow(pr, gm1 / gamma);
    term3 = std::max(term3, 0.0);

    double CfMomentum = std::sqrt(term1 * term2 * term3);
    double CfPressure = (pe - pa) / pc * Ae / At;

    return CfMomentum + CfPressure;
}

double chokedMassFlow(double pc, double At, double Tc,
                      double gamma, double R) {
    if (pc <= 0.0 || At <= 0.0 || Tc <= 0.0) return 0.0;
    double gm1 = gamma - 1.0;
    double gp1 = gamma + 1.0;
    // ṁ = pc·At/√(R·Tc) · √[γ·(2/(γ+1))^((γ+1)/(γ-1))]
    double gammaTerm = std::sqrt(gamma * std::pow(2.0 / gp1, gp1 / gm1));
    return pc * At / std::sqrt(R * Tc) * gammaTerm;
}

} // namespace GasDynamics

} // namespace FluidDynamics
