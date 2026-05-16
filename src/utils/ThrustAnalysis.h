#pragma once

#include <cmath>
#include <algorithm>

// Axial thrust computation with error propagation.
// Per architecture doc §2.4: axial thrust mechanical error ≤ 0.15%.

namespace ThrustAnalysis {

struct ThrustInputs {
    double chamberPressure_Pa;
    double exitPressure_Pa;
    double ambientPressure_Pa;
    double exitArea_m2;
    double throatArea_m2;
    double massFlow_kgPerS;
    double gamma;
    // Measurement uncertainties (1-sigma)
    double pcUncertainty_Pa  = 1.0e4;    // ±0.01 MPa typical
    double peUncertainty_Pa  = 1.0e3;
    double AeUncertainty_m2  = 1.0e-7;
    double AtUncertainty_m2  = 1.0e-7;
    double mdotUncertainty   = 0.01;     // ±0.01 kg/s
};

struct ThrustResult {
    double thrust_N;
    double specificImpulse_s;
    double thrustCoefficient;
    double relativeError;           // root-sum-square propagated error
    double thrustUncertainty_N;     // absolute error bar
    bool withinSpec;                // true if relativeError ≤ 0.0015
    double momentumThrust_N;        // ṁ·v_e component
    double pressureThrust_N;        // (P_e - P_a)·A_e component
};

// Calculate thrust: F = ṁ·v_e + (P_e - P_a)·A_e
// v_e = M_e·a_e where a_e = sqrt(γ·R·T_e) and M_e from A_e/A_t
// Simplified: uses isentropic relations to compute v_e from pressure ratio
inline ThrustResult calculateThrust(const ThrustInputs& in)
{
    ThrustResult r{};

    if (in.throatArea_m2 <= 0.0 || in.massFlow_kgPerS <= 0.0) return r;

    double gm1 = in.gamma - 1.0;
    double gp1 = in.gamma + 1.0;

    // Exit Mach number from pressure ratio (isentropic)
    double pr = in.exitPressure_Pa / std::max(in.chamberPressure_Pa, 1e-10);
    double pr_exp = std::pow(pr, gm1 / in.gamma);
    // Me_sq and cstar computed for reference; thrust uses Cf formulation directly

    // Thrust coefficient
    double Cf_momentum = std::sqrt(2.0 * in.gamma * in.gamma / gm1
                         * std::pow(2.0 / gp1, gp1 / gm1)
                         * (1.0 - pr_exp));
    double Cf_pressure = (in.exitPressure_Pa - in.ambientPressure_Pa)
                       / in.chamberPressure_Pa * in.exitArea_m2 / in.throatArea_m2;
    r.thrustCoefficient = Cf_momentum + Cf_pressure;

    // Thrust
    double Cf = std::max(r.thrustCoefficient, 0.0);
    r.thrust_N = Cf * in.chamberPressure_Pa * in.throatArea_m2;
    r.momentumThrust_N = Cf_momentum * in.chamberPressure_Pa * in.throatArea_m2;
    r.pressureThrust_N = Cf_pressure * in.chamberPressure_Pa * in.throatArea_m2;

    // Specific impulse: Isp = F / (ṁ·g₀)
    double g0 = 9.80665;
    r.specificImpulse_s = r.thrust_N / (in.massFlow_kgPerS * g0);

    // Error propagation via root-sum-square (RSS):
    // ε_F² = Σ (∂F/∂x_i · σ_xi)² / F²
    double dF_dpc = in.throatArea_m2 * Cf;
    double dF_dAt = in.chamberPressure_Pa * Cf;
    double dF_dpe = in.exitArea_m2;
    double dF_dAe = (in.exitPressure_Pa - in.ambientPressure_Pa);

    double varF = dF_dpc * dF_dpc * in.pcUncertainty_Pa * in.pcUncertainty_Pa
                + dF_dAt * dF_dAt * in.AtUncertainty_m2 * in.AtUncertainty_m2
                + dF_dpe * dF_dpe * in.peUncertainty_Pa * in.peUncertainty_Pa
                + dF_dAe * dF_dAe * in.AeUncertainty_m2 * in.AeUncertainty_m2;

    r.thrustUncertainty_N = std::sqrt(varF);
    r.relativeError = r.thrustUncertainty_N / std::max(std::abs(r.thrust_N), 1e-10);
    r.withinSpec = r.relativeError <= 0.0015;

    return r;
}

// Thrust error from chamber pressure uncertainty
inline double thrustErrorFromChamberPressure(double /*pc*/, double pcUncertainty,
                                              double At, double Cf)
{
    double dF_dpc = At * Cf;
    return dF_dpc * pcUncertainty;
}

// Thrust error from area ratio uncertainty
inline double thrustErrorFromAreaRatio(double Ae, double At,
                                        double AeUncertainty, double AtUncertainty,
                                        double pc)
{
    double dF_dAe = pc * (Ae / At) * (1.0 / At);
    double dF_dAt = -pc * (Ae / (At * At)) * Ae;
    return std::sqrt(std::pow(dF_dAe * AeUncertainty, 2.0)
                   + std::pow(dF_dAt * AtUncertainty, 2.0));
}

// Nozzle efficiency from thrust coefficient
inline double nozzleEfficiency(double Cf_actual, double Cf_ideal, double /*gamma*/)
{
    if (Cf_ideal <= 0.0) return 0.0;
    return Cf_actual / Cf_ideal;
}

} // namespace ThrustAnalysis
