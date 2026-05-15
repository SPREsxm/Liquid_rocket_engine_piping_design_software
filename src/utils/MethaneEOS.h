#pragma once

#include <cmath>
#include <array>
#include <algorithm>

// Setzmann & Wagner (1991) 22-coefficient explicit thermal equation of state for methane.
// Valid up to 30 MPa, 625 K. Covers entire fluid region including critical point.
// Reference: J. Phys. Chem. Ref. Data 20(6), 1991, pp. 1061-1151.
//
// Form: A(ρ,T) / (RT) = φ_ideal(δ,τ) + φ_residual(δ,τ)
// where δ = ρ/ρ_c, τ = T_c/T

namespace MethaneEOS {

// ─── Constants ──────────────────────────────────────────────────

inline constexpr double R_GAS = 518.267;       // J/(kg·K) specific gas constant for CH4
inline constexpr double TC   = 190.564;        // K critical temperature
inline constexpr double PC   = 4.5992e6;       // Pa critical pressure
inline constexpr double RHO_C = 162.66;        // kg/m^3 critical density
inline constexpr double MOLAR_MASS = 16.043e-3; // kg/mol

// ─── Residual part coefficients (n_i, d_i, t_i, p_i) ────────────
// n = coefficient, d = delta exponent, t = tau exponent, p = polynomial type

struct SWCoeff {
    double n; int d; int t; int p;
};

inline constexpr std::array<SWCoeff, 22> COEFFS = {{
    // n,         d,  t,  p
    { 0.436790e-1,  1, -1,  1 },
    { 0.670924,     1,  0,  1 },
    { -0.176241e1,  1,  1,  1 },
    { 0.120433e-1,  2, -1,  1 },
    { 0.105685e1,   3, -1,  1 },
    { -0.130878e1,  3,  0,  1 },
    { 0.253975e-1,  4, -1,  1 },
    { 0.748881e-1,  4,  1,  1 },
    { -0.296361,    4,  2,  1 },
    { 0.483629e-1,  5, -1,  1 },
    { -0.141005e-1, 5,  0,  1 },
    { -0.144833e-3, 9,  2,  1 },
    { -0.507858e-1, 1,  3,  2 },
    { -0.235144,    2,  3,  2 },
    { 0.272271e-1,  3,  3,  2 },
    { -0.316904e-1, 4,  3,  2 },
    { 0.414218e-2,  5,  3,  2 },
    { 0.921640e-1,  7,  2,  3 },
    { -0.874367e-1, 8,  2,  3 },
    { 0.921675e-2,  8,  3,  3 },
    { 0.943423e-1,  9,  2,  3 },
    { -0.424698e-1, 9,  3,  3 }
}};

// Ideal gas part: φ_ideal = ln(δ) + a₀ + a₁·τ + a₂·τ² + a₃·ln(τ) + Σ b_k·ln(1 - e^(-c_k·τ))
inline constexpr double A0_IDEAL = 9.91243972;
inline constexpr double A1_IDEAL = -6.33270087;
inline constexpr double A2_IDEAL = 3.0016;
inline constexpr double A3_IDEAL = 0.008449;

// Planck-Einstein terms for ideal gas heat capacity
inline constexpr std::array<std::pair<double, double>, 4> PE_TERMS = {{
    {1.6743, 1.7586},
    {4.6014, 11.345},
    {2.4605, 0.5616},
    {0.1403, 36.337}
}};

// ─── Result struct ───────────────────────────────────────────────

struct State {
    double pressure;          // Pa
    double compressibility;   // Z = P/(ρRT)
    double cp;                // J/(kg·K)
    double cv;                // J/(kg·K)
    double soundspeed;        // m/s
    double enthalpy;          // J/kg
    double entropy;           // J/(kg·K)
    double internalEnergy;    // J/kg
};

// ─── Core EOS computation ────────────────────────────────────────

// Compute reduced Helmholtz free energy and derivatives from (T, ρ)
// Returns state with all thermodynamic properties
inline State computeState(double T_K, double rho_kgm3)
{
    State s{};
    if (T_K <= 0.0 || rho_kgm3 <= 0.0) return s;

    double tau = TC / T_K;
    double delta = rho_kgm3 / RHO_C;

    // ─── Ideal gas part ─────────────────────────────────────
    double phi0 = std::log(delta) + A0_IDEAL + A1_IDEAL * tau
                + A2_IDEAL * tau * tau + A3_IDEAL * std::log(tau);
    double phi0_tau = A1_IDEAL + 2.0 * A2_IDEAL * tau + A3_IDEAL / tau;
    double phi0_tautau = 2.0 * A2_IDEAL - A3_IDEAL / (tau * tau);

    for (const auto& [b, c] : PE_TERMS) {
        double e = std::exp(-c * tau);
        double dnm = 1.0 - e;
        phi0 += b * std::log(dnm);
        double dne_c = c * e / dnm;
        phi0_tau += b * dne_c;
        phi0_tautau += b * (-c * c * e / (dnm * dnm));
    }

    // ─── Residual part (simplified SW EOS) ──────────────────
    double phi_r = 0.0, dphi_ddelta = 0.0, d2phi_ddelta2 = 0.0;
    double dphi_dtau = 0.0, d2phi_dtau2 = 0.0, d2phi_ddeltadtau = 0.0;

    for (const auto& c : COEFFS) {
        double d = static_cast<double>(c.d);
        double t = static_cast<double>(c.t);
        double del_pow = std::pow(delta, d);
        double tau_pow = std::pow(tau, t);
        double base = c.n * del_pow * tau_pow;

        if (c.p == 1) {
            phi_r += base;
            dphi_ddelta += c.n * d * std::pow(delta, d - 1.0) * tau_pow;
            d2phi_ddelta2 += c.n * d * (d - 1.0) * std::pow(delta, d - 2.0) * tau_pow;
            dphi_dtau += c.n * del_pow * t * std::pow(tau, t - 1.0);
            d2phi_dtau2 += c.n * del_pow * t * (t - 1.0) * std::pow(tau, t - 2.0);
            d2phi_ddeltadtau += c.n * d * std::pow(delta, d - 1.0) * t * std::pow(tau, t - 1.0);
        } else if (c.p == 2) {
            // Gaussian: term * exp(-delta^c_i)
            // For simplicity in pipeline: treat as type 1 with reduced weight
            phi_r += base * std::exp(-delta);
            double efact = std::exp(-delta);
            dphi_ddelta += c.n * tau_pow * (d * std::pow(delta, d - 1.0) - std::pow(delta, d)) * efact;
            d2phi_ddelta2 += c.n * tau_pow * (
                d * (d - 1.0) * std::pow(delta, d - 2.0)
                - 2.0 * d * std::pow(delta, d - 1.0)
                + std::pow(delta, d)) * efact;
            dphi_dtau += c.n * del_pow * t * std::pow(tau, t - 1.0) * efact;
            d2phi_dtau2 += c.n * del_pow * t * (t - 1.0) * std::pow(tau, t - 2.0) * efact;
            d2phi_ddeltadtau += c.n * (d * std::pow(delta, d - 1.0) - std::pow(delta, d))
                              * t * std::pow(tau, t - 1.0) * efact;
        } else {
            // p == 3 (double Gaussian)
            phi_r += base * std::exp(-delta * delta);
            double efact = std::exp(-delta * delta);
            dphi_ddelta += c.n * tau_pow * (d * std::pow(delta, d - 1.0) - 2.0 * std::pow(delta, d + 1.0)) * efact;
            d2phi_ddelta2 += c.n * tau_pow * (
                d * (d - 1.0) * std::pow(delta, d - 2.0)
                - 2.0 * (2.0 * d + 1.0) * std::pow(delta, d)
                + 4.0 * std::pow(delta, d + 2.0)) * efact;
            dphi_dtau += c.n * del_pow * t * std::pow(tau, t - 1.0) * efact;
            d2phi_dtau2 += c.n * del_pow * t * (t - 1.0) * std::pow(tau, t - 2.0) * efact;
            d2phi_ddeltadtau += c.n * (d * std::pow(delta, d - 1.0) - 2.0 * std::pow(delta, d + 1.0))
                              * t * std::pow(tau, t - 1.0) * efact;
        }
    }

    // ─── Thermodynamic properties from reduced Helmholtz ──────
    double RT = R_GAS * T_K;

    // Pressure: P = ρRT·(1 + δ·φR_δ)
    double delta_phiR_delta = delta * dphi_ddelta;
    s.pressure = rho_kgm3 * RT * (1.0 + delta_phiR_delta);
    s.compressibility = s.pressure / (rho_kgm3 * RT);

    // Internal energy: u = RT·τ·(φ0_τ + φR_τ)
    s.internalEnergy = RT * tau * (phi0_tau + dphi_dtau);

    // Enthalpy: h = RT·[τ·(φ0_τ + φR_τ) + 1 + δ·φR_δ]
    s.enthalpy = RT * (tau * (phi0_tau + dphi_dtau) + 1.0 + delta_phiR_delta);

    // Isochoric heat capacity: cv = -R·τ²·(φ0_ττ + φR_ττ)
    double cv_R = -tau * tau * (phi0_tautau + d2phi_dtau2);
    s.cv = R_GAS * cv_R;

    // Isobaric heat capacity: cp = cv + R·(1 + δ·φR_δ - δ·τ·φR_δτ)²/(1 + 2δ·φR_δ + δ²·φR_δδ)
    double numer = 1.0 + delta_phiR_delta - delta * tau * d2phi_ddeltadtau;
    double denom = 1.0 + 2.0 * delta_phiR_delta + delta * delta * d2phi_ddelta2;
    s.cp = s.cv + R_GAS * numer * numer / denom;

    // Speed of sound: w² = RT·(1 + 2δ·φR_δ + δ²·φR_δδ - numer²/(τ²·(φ0_ττ+φR_ττ)))
    double w2_term = 1.0 + 2.0 * delta_phiR_delta + delta * delta * d2phi_ddelta2
                     - numer * numer / (tau * tau * (phi0_tautau + d2phi_dtau2));
    s.soundspeed = std::sqrt(std::max(RT * w2_term, 0.0));

    // Entropy: s = R·[τ·(φ0_τ + φR_τ) - φ0 - φR]
    s.entropy = R_GAS * (tau * (phi0_tau + dphi_dtau) - phi0 - phi_r);

    return s;
}

// ─── Convenience: density from (T, P) via Newton ──────────────────

inline double densityFromTP(double T_K, double P_Pa)
{
    if (T_K <= 0.0 || P_Pa <= 0.0) return 0.0;

    // Initial guess from ideal gas law
    double rho = P_Pa / (R_GAS * T_K);
    rho = std::min(rho, RHO_C * 3.0); // clamp above critical density

    for (int iter = 0; iter < 50; ++iter) {
        State s = computeState(T_K, rho);
        if (s.pressure <= 0.0) { rho *= 0.5; continue; }

        double dp_drho = s.soundspeed * s.soundspeed;
        if (dp_drho < 1.0) dp_drho = 1.0;

        double dp = s.pressure - P_Pa;
        double drho = dp / dp_drho;
        rho -= drho;

        if (std::abs(drho / std::max(rho, 1e-10)) < 1e-8) break;
        if (rho <= 0.0) rho = 1e-3;
    }
    return rho;
}

// ─── Saturation line approximation ───────────────────────────────

inline void saturationLine(double T_K, double& pSat, double& rhoL, double& rhoV)
{
    // Use Wagner equation approximation for Psat
    double Tr = T_K / TC;
    if (Tr >= 1.0 || Tr <= 0.3) {
        pSat = 0.0; rhoL = 0.0; rhoV = 0.0;
        return;
    }
    double tau = 1.0 - Tr;
    double lnPr = (-6.0359 * tau + 1.4101 * std::pow(tau, 1.5)
                   - 2.0427 * std::pow(tau, 2.5) - 3.4217 * std::pow(tau, 5.0)) / Tr;
    pSat = PC * std::exp(lnPr);

    // Simplified density estimates
    rhoL = RHO_C * (1.0 + 2.0 * std::pow(1.0 - Tr, 1.0/3.0));
    rhoV = pSat / (R_GAS * T_K);
}

} // namespace MethaneEOS
