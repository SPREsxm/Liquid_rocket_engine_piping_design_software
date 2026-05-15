#pragma once

#include <cmath>
#include <algorithm>

// Menter SST k-ω turbulence model (Menter, AIAA J. 32(8), 1994: 1598-1605).
// Blends k-ω (Wilcox) near-wall with k-ε (Jones-Launder) far-field for
// improved prediction of separated flows and adverse pressure gradients.
//
// For pipe flow applications, provides eddy viscosity and turbulent
// friction factor enhancement over standard Colebrook-White.

namespace SSTTurbulence {

// ─── Model constants ────────────────────────────────────────────

namespace C {
    inline constexpr double betaStar = 0.09;
    inline constexpr double kappa   = 0.41;
    inline constexpr double a1      = 0.31;

    // k-ω (inner / Wilcox) constants
    inline constexpr double sigma_k1 = 0.85;
    inline constexpr double sigma_w1 = 0.5;
    inline constexpr double beta1    = 0.075;

    // k-ε (outer / Jones-Launder) constants
    inline constexpr double sigma_k2 = 1.0;
    inline constexpr double sigma_w2 = 0.856;
    inline constexpr double beta2    = 0.0828;
}

// ─── Input structures ───────────────────────────────────────────

struct PipeFlowConditions {
    double density;              // kg/m^3
    double velocity;             // m/s (bulk mean velocity)
    double diameter;             // m
    double molecularViscosity;   // Pa·s (laminar dynamic viscosity)
    double roughness;            // m (sand-grain roughness)
};

struct TurbulenceResult {
    double turbKineticEnergy_k;    // m²/s²
    double specificDissipation_w;  // 1/s
    double eddyViscosity;          // Pa·s (μ_t)
    double blendingFunction_F1;    // 1=near-wall, 0=far-field
    double effectiveViscosity;     // μ + μ_t (Pa·s)
    double wallShearStress;        // τ_w (Pa)
    double frictionFactor;         // f = 8·τ_w/(ρ·U²) = 8·u_τ²/U²
};

// ─── Estimate initial k and ω from bulk flow parameters ─────────

// For fully-developed pipe flow, estimate turbulence at centerline.
// k ≈ 0.05·U² (typical for pipe flow at moderate Re)
// ω = k^0.5 / (C_μ^0.25 · L) where L ≈ 0.07·D (mixing length)
inline void estimateKandOmega(const PipeFlowConditions& fc,
                               double& k_init, double& omega_init)
{
    double absU = std::abs(fc.velocity);
    k_init = 0.05 * absU * absU;
    double L_turb = 0.07 * fc.diameter;
    double cmu025 = std::pow(C::betaStar, 0.25);
    omega_init = std::sqrt(k_init) / (cmu025 * L_turb);
    omega_init = std::max(omega_init, 1e-10);
}

// ─── F1 blending function ───────────────────────────────────────

// F1 = tanh(arg1^4)
// arg1 = min[max(√k/(β*·ω·y), 500ν/(y²·ω)), 4ρ·σ_w2·k/(CD_kw·y²)]
// CD_kw = max(2ρ·σ_w2·(1/ω)·(∂k/∂x_j)·(∂ω/∂x_j), 1e-20)
inline double blendingFunctionF1(double k, double omega, double y,
                                  double nu, double density)
{
    double sqrtK = std::sqrt(std::max(k, 0.0));
    double arg1_1 = sqrtK / (C::betaStar * omega * y);
    double arg1_2 = 500.0 * nu / (y * y * omega);

    // Cross-diffusion term — simplified for pipe flow (assume ∂k·∂ω ≈ 0 at centerline)
    double CD_kw = 1e-20; // minimal cross-diffusion for fully-developed flow
    double arg1_3 = 4.0 * density * C::sigma_w2 * k / (CD_kw * y * y);

    double arg1 = std::min(std::max(arg1_1, arg1_2), arg1_3);
    double arg1_4 = arg1 * arg1 * arg1 * arg1;
    return std::tanh(arg1_4);
}

// ─── F2 blending function ───────────────────────────────────────

// F2 = tanh(arg2^2)
// arg2 = max(2·√k/(β*·ω·y), 500ν/(y²·ω))
inline double blendingFunctionF2(double k, double omega, double y, double nu)
{
    double sqrtK = std::sqrt(std::max(k, 0.0));
    double arg2_1 = 2.0 * sqrtK / (C::betaStar * omega * y);
    double arg2_2 = 500.0 * nu / (y * y * omega);
    double arg2 = std::max(arg2_1, arg2_2);
    return std::tanh(arg2 * arg2);
}

// ─── Eddy viscosity (SST limiter) ───────────────────────────────

// μ_t = ρ·a1·k / max(a1·ω, Ω·F2)
// Ω = |du/dr| ≈ 8·U/D for fully-developed pipe flow (at wall)
inline double eddyViscositySST(double k, double omega,
                                double shearRate, double density)
{
    double a1w = C::a1 * omega;
    double omegaF2 = shearRate * 1.0; // F2 ≈ 1 near wall
    double denom = std::max(a1w, omegaF2);
    if (denom <= 0.0) return 0.0;
    return density * C::a1 * k / denom;
}

// ─── Effective pipe flow turbulence computation ─────────────────

// Computes turbulence properties for fully-developed pipe flow
// using the SST k-ω framework.
inline TurbulenceResult computePipeTurbulence(const PipeFlowConditions& fc)
{
    TurbulenceResult r{};

    double nu = fc.molecularViscosity / fc.density; // kinematic viscosity
    double U = std::abs(fc.velocity);

    if (U <= 0.0 || fc.diameter <= 0.0) return r;

    // Reynolds number
    double Re = U * fc.diameter / nu;
    if (Re < 2300.0) {
        // Laminar: no turbulence
        r.effectiveViscosity = fc.molecularViscosity;
        r.wallShearStress = 8.0 * fc.molecularViscosity * U / fc.diameter;
        r.frictionFactor = 64.0 / Re;
        return r;
    }

    // Estimate k and ω for pipe core
    double k, omega;
    estimateKandOmega(fc, k, omega);

    // Characteristic wall distance (pipe radius for near-wall scaling)
    double y = fc.diameter * 0.5;

    // Friction velocity estimate (u_τ = sqrt(τ_w/ρ))
    // Initial guess from Colebrook-White: λ ≈ 0.02 for turbulent
    double lambda_init = 0.02;
    double u_tau = U * std::sqrt(lambda_init / 8.0);

    // Simplified iteration to converge k, ω, and μ_t
    for (int iter = 0; iter < 20; ++iter) {
        double F1 = blendingFunctionF1(k, omega, y, nu, fc.density);
        double F2 = blendingFunctionF2(k, omega, y, nu);

        // Shear rate at wall: du/dr ≈ u_τ²/(ν + ν_t) * y⁺ scaling
        double shearRate = u_tau * u_tau / (nu + 1e-10);

        double mu_t = eddyViscositySST(k, omega, shearRate * F2, fc.density);

        // Update friction velocity
        double mu_eff = fc.molecularViscosity + mu_t;
        double new_u_tau = U * std::sqrt(mu_eff / (fc.density * fc.diameter * fc.diameter / 8.0));
        // Simplified: τ_w = μ_eff·du/dr ≈ μ_eff·8U/D
        double tau_w = mu_eff * 8.0 * U / fc.diameter;
        new_u_tau = std::sqrt(tau_w / fc.density);

        u_tau = 0.5 * (u_tau + new_u_tau); // relaxation

        // Store results
        r.turbKineticEnergy_k = k;
        r.specificDissipation_w = omega;
        r.eddyViscosity = mu_t;
        r.blendingFunction_F1 = F1;
        r.wallShearStress = tau_w;

        // Update k and ω for next iteration (simplified equilibrium)
        k = u_tau * u_tau / std::sqrt(C::betaStar);
        omega = u_tau / (std::sqrt(C::betaStar) * C::kappa * y);
        omega = std::max(omega, 1e-10);
    }

    r.effectiveViscosity = fc.molecularViscosity + r.eddyViscosity;
    r.frictionFactor = 8.0 * r.wallShearStress / (fc.density * U * U);

    return r;
}

// ─── Effective friction factor using SST ────────────────────────

// Convenience: compute friction factor using SST turbulence model.
// For Re < 2300, returns 64/Re (laminar).
// For Re >= 2300, uses SST k-ω to compute effective turbulent friction.
inline double effectiveFrictionFactorSST(double reynolds, double roughness,
                                          double diameter, double density,
                                          double molecularViscosity, double velocity)
{
    PipeFlowConditions fc;
    fc.density = density;
    fc.velocity = velocity;
    fc.diameter = diameter;
    fc.molecularViscosity = molecularViscosity;
    fc.roughness = roughness;

    auto result = computePipeTurbulence(fc);
    return result.frictionFactor;
}

// ─── Wall function helpers ──────────────────────────────────────

// Near-wall k estimate: k = u_τ² / √(β*)
inline double wallKEstimate(double frictionVelocity)
{
    return frictionVelocity * frictionVelocity / std::sqrt(C::betaStar);
}

// Near-wall ω estimate: ω = ρ·u_τ² / (μ·y⁺)
inline double wallOmegaEstimate(double frictionVelocity, double kinematicViscosity,
                                 double yPlus = 1.0)
{
    if (kinematicViscosity <= 0.0) return 1e10;
    return frictionVelocity * frictionVelocity
           / (kinematicViscosity * yPlus * std::sqrt(C::betaStar));
}

} // namespace SSTTurbulence
