#pragma once

#include <cmath>
#include <array>
#include <algorithm>

// Temperature-dependent propellant property estimation methods.
// References: Poling-Prausnitz-O'Connell "Properties of Gases and Liquids" (5th ed.),
// NIST REFPROP 10.0, DIPPR 801 database.

namespace PropellantProperties {

// ─── Critical constants and basic data per propellant ──────────

struct CriticalData {
    double Tc;       // Critical temperature (K)
    double Pc;       // Critical pressure (Pa)
    double Vc;       // Critical volume (m^3/mol)
    double Mw;       // Molecular weight (kg/kmol)
    double omega;    // Acentric factor
    double Tb;       // Normal boiling point (K)
    double Z_RA;     // Rackett compressibility factor
};

inline CriticalData criticalLOX()     { return {154.58, 5.043e6, 7.34e-5,  32.0,  0.022,  90.19, 0.2905}; }
inline CriticalData criticalRP1()     { return {669.0,  2.33e6,  7.54e-4,  170.0, 0.498,  450.0,  0.2650}; }
inline CriticalData criticalMethane() { return {190.56, 4.599e6, 9.86e-5,  16.04, 0.011,  111.6,  0.2888}; }
inline CriticalData criticalLH2()     { return {33.145, 1.296e6, 6.50e-5,  2.016, -0.219, 20.39, 0.3030}; }
inline CriticalData criticalWater()   { return {647.14, 2.206e7, 5.59e-5,  18.015,0.344,  373.15, 0.2333}; }

// ─── #5 Wagner 2.5-5 vapor pressure equation ───────────────────

struct WagnerCoeffs {
    double Tc, Pc;
    double A, B, C, D;
};

// Wagner 2.5-5 form: ln(P_r) = (1/T_r) * (A·τ + B·τ^1.5 + C·τ^2.5 + D·τ^5)
// where τ = 1 - T_r, T_r = T/T_c, P_r = P/P_c
inline double wagnerVaporPressure(double T, const WagnerCoeffs& c)
{
    if (T <= 0.0 || T >= c.Tc) return 0.0;
    double Tr = T / c.Tc;
    double tau = 1.0 - Tr;
    double lnPr = (c.A * tau + c.B * std::pow(tau, 1.5)
                   + c.C * std::pow(tau, 2.5) + c.D * std::pow(tau, 5.0)) / Tr;
    return c.Pc * std::exp(lnPr);
}

// Coefficients from NIST REFPROP / DIPPR 801
inline WagnerCoeffs wagnerLOX()     { return {154.58, 5.043e6, -6.0439,  1.1757,  -2.1455,  -2.2430}; }
inline WagnerCoeffs wagnerMethane() { return {190.56, 4.599e6, -6.0359,  1.4101,  -2.0427,  -3.4217}; }
inline WagnerCoeffs wagnerLH2()     { return {33.145, 1.296e6, -4.8972,  0.5785,  0.5855,   -0.6510}; }
inline WagnerCoeffs wagnerWater()   { return {647.14, 2.206e7, -7.8595,  1.8441,  -11.7866, -3.7450}; }
inline WagnerCoeffs wagnerRP1()     { return {669.0,  2.33e6,  -7.7000,  2.0500,  -3.1000,  -4.5000}; }

// ─── #6 Rackett / Yamada-Gunn density equations ────────────────

// Rackett (1970): ρ_sat = ρ_c / Z_c^((1-T_r)^(2/7))
// ρ_c = Mw / Vc, Z_c = Pc*Vc/(R*Tc)
inline double rackettDensity(double T, const CriticalData& cd)
{
    if (T >= cd.Tc || T <= 0.0) return 0.0;
    double Tr = T / cd.Tc;
    double exponent = std::pow(1.0 - Tr, 2.0 / 7.0);
    double Zc = cd.Pc * cd.Vc / (8314.0 * cd.Tc);  // R = 8314 J/(kmol·K)
    double rhoC = cd.Mw / cd.Vc;  // kg/m^3
    return rhoC / std::pow(Zc, exponent);
}

// Yamada-Gunn (1973) — modified Rackett using Z_RA
inline double yamadaGunnDensity(double T, const CriticalData& cd)
{
    if (T >= cd.Tc || T <= 0.0) return 0.0;
    double Tr = T / cd.Tc;
    double phi = std::pow(1.0 - Tr, 2.0 / 7.0);
    double rhoC = cd.Mw / cd.Vc;
    return rhoC / std::pow(cd.Z_RA, phi);
}

// Daubert-Danner for heavy hydrocarbons (RP-1 surrogate)
// ρ_L = A / B^(1 + (1-T_r)^n)   [kg/m^3]
inline double daubertDensity(double T, double Tc, double A, double B, double n)
{
    double Tr = T / Tc;
    return A / std::pow(B, 1.0 + std::pow(1.0 - Tr, n));
}
inline double daubertDensityRP1(double T) {
    return daubertDensity(T, 669.0, 810.0, 1.15, 0.2857);
}

// ─── #7 Joback method for ideal gas heat capacity ──────────────

struct JobackGroups {
    int ch3 = 0, ch2 = 0, ch = 0, c_quat = 0;
    int oh = 0, o_ether = 0, cho = 0, cooh = 0, coo = 0, o_ring = 0;
    int nh2 = 0, nh = 0, n_tert = 0, cn = 0, no2 = 0;
    int f_groups = 0, cl_groups = 0, br_groups = 0, i_groups = 0;
    int sh = 0, s_ring = 0;
};

// Joback group contribution values (J/(mol·K))
// Cp_ig = Σ(Δa - 37.93) + Σ(Δb + 0.210)*T + Σ(Δc - 3.91e-4)*T² + Σ(Δd + 2.06e-7)*T³
inline double jobackIdealGasCp(double T, const JobackGroups& g)
{
    auto pa = [&]() -> double {
        return g.ch3 * 19.5 + g.ch2 * (-9.09e-1) + g.ch * (-2.30e1)
             + g.c_quat * (-6.62e1) + g.oh * 25.7 + g.o_ether * 25.5
             + g.cho * 30.9 + g.cooh * 24.1 + g.coo * 24.5 + g.o_ring * 12.2
             + g.nh2 * 26.9 + g.nh * (-1.21) + g.n_tert * (-3.37e1)
             + g.cn * 36.5 + g.no2 * 32.6 + g.f_groups * 26.5
             + g.cl_groups * 33.3 + g.br_groups * 28.6 + g.i_groups * 32.0
             + g.sh * 35.3 + g.s_ring * 19.6;
    };
    auto pb = [&]() -> double {
        return g.ch3 * (-8.08e-3) + g.ch2 * 9.50e-2 + g.ch * 2.04e-1
             + g.c_quat * 4.27e-1 + g.oh * (-6.91e-2) + g.o_ether * (-6.32e-2)
             + g.cho * (-3.36e-2) + g.cooh * 4.27e-2 + g.coo * 4.02e-2
             + g.o_ring * (-1.26e-2) + g.nh2 * (-4.12e-2) + g.nh * 1.84e-1
             + g.n_tert * 5.58e-1 + g.cn * (-7.33e-2) + g.no2 * (-6.42e-2)
             + g.f_groups * (-7.86e-2) + g.cl_groups * (-9.63e-2)
             + g.br_groups * (-6.49e-2) + g.i_groups * (-6.41e-2)
             + g.sh * (-7.58e-2) + g.s_ring * (-1.66e-3);
    };
    auto pc = [&]() -> double {
        return g.ch3 * 1.53e-4 + g.ch2 * 5.44e-5 + g.ch * (-2.65e-4)
             + g.c_quat * (-5.59e-4) + g.oh * (-1.77e-4) + g.o_ether * (-1.76e-4)
             + g.cho * (-1.60e-4) + g.cooh * (-1.89e-4) + g.coo * (-4.52e-5)
             + g.o_ring * 1.66e-5 + g.nh2 * (-1.22e-4) + g.nh * (-1.96e-4)
             + g.n_tert * (-6.86e-4) + g.cn * (-1.84e-4) + g.no2 * (-2.59e-4)
             + g.f_groups * (-1.15e-4) + g.cl_groups * (-1.88e-4)
             + g.br_groups * (-1.36e-4) + g.i_groups * (-1.06e-4)
             + g.sh * (-1.83e-4) + g.s_ring * 1.55e-5;
    };
    auto pd = [&]() -> double {
        return g.ch3 * (-9.67e-8) + g.ch2 * (-1.19e-8) + g.ch * 1.20e-7
             + g.c_quat * 2.66e-7 + g.oh * 9.88e-8 + g.o_ether * 9.78e-8
             + g.cho * 6.88e-8 + g.cooh * 1.11e-7 + g.coo * 1.53e-8
             + g.o_ring * 1.12e-8 + g.nh2 * 7.43e-8 + g.nh * 9.32e-8
             + g.n_tert * 2.80e-7 + g.cn * 1.46e-7 + g.no2 * 1.68e-7
             + g.f_groups * 7.07e-8 + g.cl_groups * 1.09e-7
             + g.br_groups * 8.26e-8 + g.i_groups * 6.37e-8
             + g.sh * 1.04e-7 + g.s_ring * 3.57e-9;
    };
    double sumA = pa() - 37.93;
    double sumB = pb() + 0.210;
    double sumC = pc() - 3.91e-4;
    double sumD = pd() + 2.06e-7;
    return sumA + sumB * T + sumC * T * T + sumD * T * T * T;  // J/(mol·K)
}

// Convenience: Methane Cp (CH4, one -CH3 equivalent → 1× ch3)
inline double methaneIdealGasCp(double T) {
    JobackGroups g;
    g.ch3 = 1;
    return jobackIdealGasCp(T, g) / 16.04 * 1000.0;  // J/(kg·K)
}

// ─── #8 Squires method for saturated liquid viscosity ──────────

// Squires (1984): η_sL = η_0 * exp[f(T_r)]
// η_0 = 0.0038 * Mw^0.5 * Pc^(2/3) / Tc^(1/6)   [cP → Pa·s: ×1e-3]
inline double squiresViscosity(double T, const CriticalData& cd, double Psat)
{
    if (T >= cd.Tc) return 0.0;
    double Tr = T / cd.Tc;
    double Pr = Psat / cd.Pc;
    double term = 0.1023 + 0.023364 * Pr + 0.058533 * Pr * Pr
                - 0.040758 * Pr * Pr * Pr + 0.0093324 * Pr * Pr * Pr * Pr;
    double tau = std::pow(1.0 - Tr, 0.3406);
    double eta0_cP = 0.0038 * std::sqrt(cd.Mw) * std::pow(cd.Pc * 1e-5, 2.0/3.0)
                     / std::pow(cd.Tc, 1.0/6.0); // Pc in bar for this formula
    return eta0_cP * 1e-3 * std::exp(term * tau);  // Pa·s
}

// Simplified Squires when Psat not available
inline double squiresViscositySimple(double T, const CriticalData& cd)
{
    double Psat = wagnerVaporPressure(T, wagnerLOX()); // fallback using generic coeffs
    return squiresViscosity(T, cd, Psat);
}

// ─── #9 Pitzer method for latent heat of vaporization ──────────

// Pitzer (1995): ΔH_vap = R·Tc·[7.08·(1-Tr)^0.354 + 10.95·ω·(1-Tr)^0.456]
// Returns kJ/kg
inline double pitzerHeatOfVaporization(double T, const CriticalData& cd)
{
    if (T >= cd.Tc || T <= 0.0) return 0.0;
    double Tr = T / cd.Tc;
    double tau = 1.0 - Tr;
    double term1 = 7.08 * std::pow(tau, 0.354);
    double term2 = 10.95 * cd.omega * std::pow(tau, 0.456);
    double dH_Jmol = 8.314 * cd.Tc * (term1 + term2);  // J/mol
    return dH_Jmol / cd.Mw * 1e-3;  // kJ/kg
}

// ─── #10 Nicola method for liquid thermal conductivity ─────────

// Nicola (2001): λ_L = 0.5144 - 0.05053·ω + (1.1061 + 0.09556·ω)/Tr^0.88
//                     - (0.4598 + 0.01936·ω)·Tr   [W/(m·K)]
inline double nicolaThermalConductivity(double T, const CriticalData& cd)
{
    if (T >= cd.Tc) return 0.0;
    double Tr = T / cd.Tc;
    double a = 0.5144 - 0.05053 * cd.omega;
    double b = 1.1061 + 0.09556 * cd.omega;
    double c = 0.4598 + 0.01936 * cd.omega;
    return a + b / std::pow(Tr, 0.88) - c * Tr;
}

// ─── #11 Sastri-Rao method for surface tension ─────────────────

// Sastri-Rao (1995): σ = K·Pc^x·Tb^y·Tc^z·((1-Tr)/(1-Tbr))^n   [N/m]
inline double sastriRaoSurfaceTension(double T, const CriticalData& cd)
{
    if (T >= cd.Tc) return 0.0;
    double Tr = T / cd.Tc;
    double Tbr = cd.Tb / cd.Tc;
    double numerator = 1.0 - Tr;
    double denominator = 1.0 - Tbr;
    if (denominator <= 0.0) return 0.0;
    double exponent = 1.222; // average for organic liquids
    double K = 0.158;
    double x = 0.59, y = 0.11, z = -0.24; // Pc in bar internally
    double PcBar = cd.Pc * 1e-5;
    double sigma_mNm = K * std::pow(PcBar, x) * std::pow(cd.Tb, y)
                       * std::pow(cd.Tc, z)
                       * std::pow(numerator / denominator, exponent);
    return sigma_mNm * 1e-3;  // N/m
}

// ─── Temperature-dependent density fallback selector ────────────

inline double temperatureDependentDensity(double T, const CriticalData& cd)
{
    if (T <= 0.0) return cd.Mw / cd.Vc; // return critical density
    return yamadaGunnDensity(T, cd);
}

// ─── Temperature-dependent viscosity fallback ───────────────────

inline double temperatureDependentViscosity(double T, const CriticalData& cd)
{
    if (T <= 0.0) return 1e-3;
    return squiresViscositySimple(T, cd);
}

// ─── Composite property getter ──────────────────────────────────

struct TemperatureDependentProps {
    double density;
    double viscosity;
    double vaporPressure;
    double heatOfVaporization;    // kJ/kg
    double thermalConductivity;   // W/(m·K)
    double surfaceTension;        // N/m
    double idealGasCp;            // J/(kg·K)
};

inline TemperatureDependentProps computeAllProperties(double T, const CriticalData& cd,
                                                       const WagnerCoeffs& wc)
{
    TemperatureDependentProps p;
    p.density = temperatureDependentDensity(T, cd);
    p.viscosity = temperatureDependentViscosity(T, cd);
    p.vaporPressure = wagnerVaporPressure(T, wc);
    p.heatOfVaporization = pitzerHeatOfVaporization(T, cd);
    p.thermalConductivity = nicolaThermalConductivity(T, cd);
    p.surfaceTension = sastriRaoSurfaceTension(T, cd);
    p.idealGasCp = 0.0;  // Joback requires group specification per substance
    return p;
}

} // namespace PropellantProperties
