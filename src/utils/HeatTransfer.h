#pragma once

#include <cmath>
#include <algorithm>

// Convective heat transfer correlations for liquid rocket engine applications.
// Covers single-phase internal flow, regenerative cooling, and nozzle heat transfer.

namespace HeatTransfer {

struct FlowConditions {
    double reynoldsNumber;
    double prandtlNumber;
    double thermalConductivity_WpmK;
    double diameter_m;
    double viscosity_bulk;       // Pa·s at bulk temperature
    double viscosity_wall;       // Pa·s at wall temperature
};

// Dittus-Boelter (1930): Nu = 0.023·Re^0.8·Pr^n
// n = 0.4 for heating (T_wall > T_fluid), n = 0.3 for cooling
// Valid: 0.6 ≤ Pr ≤ 160, Re ≥ 10000, L/D ≥ 10
inline double dittusBoelterNusselt(const FlowConditions& fc, bool heating)
{
    if (fc.reynoldsNumber < 10000.0 || fc.prandtlNumber <= 0.0)
        return 0.0;
    double n = heating ? 0.4 : 0.3;
    return 0.023 * std::pow(fc.reynoldsNumber, 0.8) * std::pow(fc.prandtlNumber, n);
}

// Sieder-Tate (1936): Nu = 0.027·Re^0.8·Pr^(1/3)·(μ_bulk/μ_wall)^0.14
// Accounts for viscosity variation across thermal boundary layer
// Valid: 0.7 ≤ Pr ≤ 16700, Re ≥ 10000, L/D ≥ 10
inline double siederTateNusselt(const FlowConditions& fc)
{
    if (fc.reynoldsNumber < 10000.0 || fc.prandtlNumber <= 0.0)
        return 0.0;
    double viscRatio = fc.viscosity_bulk / std::max(fc.viscosity_wall, 1e-10);
    return 0.027 * std::pow(fc.reynoldsNumber, 0.8)
           * std::pow(fc.prandtlNumber, 1.0/3.0)
           * std::pow(viscRatio, 0.14);
}

// Gnielinski (1976): Nu = (f/8)·(Re-1000)·Pr / (1 + 12.7·(f/8)^0.5·(Pr^(2/3)-1))
// More accurate in transition region (2300 ≤ Re ≤ 5e6)
// Valid: 2300 ≤ Re ≤ 5e6, 0.5 ≤ Pr ≤ 2000
inline double gnielinskiNusselt(const FlowConditions& fc, double frictionFactor)
{
    if (fc.reynoldsNumber < 2300.0 || fc.prandtlNumber <= 0.0)
        return 0.0;

    double f8 = frictionFactor / 8.0;
    double sqrt_f8 = std::sqrt(std::max(f8, 0.0));
    double Pr23 = std::pow(fc.prandtlNumber, 2.0/3.0);

    double numerator = f8 * (fc.reynoldsNumber - 1000.0) * fc.prandtlNumber;
    double denominator = 1.0 + 12.7 * sqrt_f8 * (Pr23 - 1.0);

    return numerator / std::max(denominator, 1e-10);
}

// Convert Nusselt number to heat transfer coefficient
// h = Nu·k / D   [W/(m²·K)]
inline double heatTransferCoefficient(double nusselt, double k_WpmK, double D_m)
{
    if (D_m <= 0.0) return 0.0;
    return nusselt * k_WpmK / D_m;
}

// Newton's cooling law: q" = h·(T_wall - T_fluid)   [W/m²]
inline double newtonCoolingHeatFlux(double h_Wpm2K, double T_wall_K, double T_fluid_K)
{
    return h_Wpm2K * (T_wall_K - T_fluid_K);
}

// Total heat transfer: Q = h·A·ΔT   [W]
inline double newtonCoolingTotalHeat(double h_Wpm2K, double area_m2,
                                     double T_wall_K, double T_fluid_K)
{
    return h_Wpm2K * area_m2 * (T_wall_K - T_fluid_K);
}

// Bartz correlation for hot-gas-side heat transfer in rocket nozzles.
// h_g = 0.026/D_throat · (μ^0.2·Cp/Pr^0.6) · (p_c/c*)^0.8 · (D_t/R)^0.1 · (A_t/A)^0.9 · σ
// σ = 1 / [(T_w/T_c)^0.8·(1 + (γ-1)/2·M²)^0.4]
//
// pc: chamber pressure (Pa), cstar: characteristic velocity (m/s)
// Dt: throat diameter (m), R: throat radius of curvature (m)
// At: throat area (m²), A: local cross-sectional area (m²)
// mu: viscosity at wall (Pa·s), Cp: specific heat (J/kg·K), Pr: Prandtl number
// Tw: wall temperature (K), Tc: chamber temperature (K)
// M: local Mach number, gamma: specific heat ratio
inline double bartzHotGasCoefficient(double D_throat_m, double mu_wall_Pas,
                                      double Cp_JkgK, double Pr,
                                      double pc_Pa, double cstar_ms,
                                      double Dt_m, double R_curvature_m,
                                      double At_m2, double A_local_m2,
                                      double Tw_K, double Tc_K,
                                      double mach, double gamma)
{
    if (D_throat_m <= 0.0 || cstar_ms <= 0.0) return 0.0;

    double base = 0.026 / D_throat_m;
    double propFactor = std::pow(mu_wall_Pas, 0.2) * Cp_JkgK / std::pow(Pr, 0.6);
    double flowFactor = std::pow(pc_Pa / cstar_ms, 0.8);
    double geomFactor = std::pow(Dt_m / std::max(R_curvature_m, 1e-6), 0.1)
                      * std::pow(At_m2 / std::max(A_local_m2, 1e-10), 0.9);

    double gm1 = gamma - 1.0;
    double sigma = 1.0 / (std::pow(Tw_K / Tc_K, 0.8)
                   * std::pow(1.0 + 0.5 * gm1 * mach * mach, 0.4));

    return base * propFactor * flowFactor * geomFactor * sigma;
}

// Simplified Bartz for throat only (M=1, A=At)
inline double bartzThroatCoefficient(double Dt_m, double mu_Pas,
                                      double Cp_JkgK, double Pr,
                                      double pc_Pa, double cstar_ms,
                                      double R_curvature_m,
                                      double Tw_K, double Tc_K, double gamma)
{
    return bartzHotGasCoefficient(Dt_m, mu_Pas, Cp_JkgK, Pr,
                                   pc_Pa, cstar_ms, Dt_m, R_curvature_m,
                                   M_PI * Dt_m * Dt_m / 4.0, M_PI * Dt_m * Dt_m / 4.0,
                                   Tw_K, Tc_K, 1.0, gamma);
}

// Regenerative cooling channel heat balance
// Coolant temperature rise per channel length: dT/dx = q"·P / (ṁ·Cp)
// where P = wetted perimeter, ṁ = coolant mass flow rate
inline double coolantTemperatureRise(double heatFlux_Wpm2, double perimeter_m,
                                      double massFlow_kgps, double Cp_JkgK,
                                      double channelLength_m)
{
    if (massFlow_kgps <= 0.0 || Cp_JkgK <= 0.0) return 0.0;
    return heatFlux_Wpm2 * perimeter_m * channelLength_m / (massFlow_kgps * Cp_JkgK);
}

} // namespace HeatTransfer
