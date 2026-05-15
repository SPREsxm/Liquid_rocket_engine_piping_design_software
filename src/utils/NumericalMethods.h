#pragma once

#include <cmath>
#include <vector>
#include <functional>

// Second-order accurate numerical methods for fluid dynamics.
// Per architecture doc §2.4: "二阶精度数值格式（空间+时间）"

namespace NumericalMethods {

// ─── Second-order spatial derivatives ──────────────────────────

// Central difference (2nd order): df/dx ≈ (f(x+h) - f(x-h)) / (2h)
inline double centralDiff(double fm, double fp, double h) {
    return (fp - fm) / (2.0 * h);
}

// Second derivative (2nd order): d²f/dx² ≈ (f(x+h) - 2f(x) + f(x-h)) / h²
inline double centralDiff2(double fm, double f0, double fp, double h) {
    return (fp - 2.0 * f0 + fm) / (h * h);
}

// One-sided upwind (2nd order): df/dx ≈ (3f(x) - 4f(x-h) + f(x-2h)) / (2h)
inline double upwindDiff2(double f0, double fm1, double fm2, double h, int sign) {
    if (sign > 0)
        return (3.0 * f0 - 4.0 * fm1 + fm2) / (2.0 * h);
    else
        return (-3.0 * f0 + 4.0 * fm1 - fm2) / (2.0 * h);
}

// ─── Second-order time integration ─────────────────────────────

// Adams-Bashforth 2-step (2nd order explicit):
//   y_{n+1} = y_n + Δt/2 · (3·f_n − f_{n-1})
inline double adamsBashforth2(double yn, double fn, double fnm1, double dt) {
    return yn + dt * 0.5 * (3.0 * fn - fnm1);
}

// Crank-Nicolson step for dy/dt = f(y):
//   y_{n+1} = y_n + Δt/2 · (f(y_n) + f(y_{n+1}))
// For linear f(y) = λy, solved implicitly:
//   y_{n+1}(1 − λΔt/2) = y_n(1 + λΔt/2)
inline double crankNicolsonLinear(double yn, double lambda, double dt) {
    return yn * (1.0 + 0.5 * lambda * dt) / (1.0 - 0.5 * lambda * dt);
}

// ─── Lax-Wendroff scheme (2nd order space+time) ────────────────

// Lax-Wendroff for advection equation: ∂u/∂t + a·∂u/∂x = 0
// u_j^{n+1} = u_j^n − (c/2)(u_{j+1}^n − u_{j-1}^n) + (c²/2)(u_{j+1}^n − 2u_j^n + u_{j-1}^n)
// where c = a·Δt/Δx (Courant number)
inline std::vector<double> laxWendroffStep(const std::vector<double>& u,
                                            double courant, int n) {
    std::vector<double> uNew(n, 0.0);
    double c = courant;
    double c2 = c * c;

    // Interior points
    for (int j = 1; j < n - 1; ++j) {
        uNew[j] = u[j]
                - 0.5 * c * (u[j + 1] - u[j - 1])
                + 0.5 * c2 * (u[j + 1] - 2.0 * u[j] + u[j - 1]);
    }

    // Boundary: zero-gradient extrapolation
    uNew[0] = uNew[1];
    uNew[n - 1] = uNew[n - 2];

    return uNew;
}

// ─── TVD min-mod limiter (§5.3) ────────────────────────────────

// Min-mod limiter for slope reconstruction
inline double minmod(double a, double b) {
    if (a * b <= 0.0) return 0.0;
    if (std::abs(a) < std::abs(b)) return a;
    return b;
}

inline double minmod3(double a, double b, double c) {
    return minmod(a, minmod(b, c));
}

// MUSCL reconstruction with min-mod limiter (2nd-order TVD)
// Reconstructs left and right states at face j+1/2
inline void musclReconstruct(const std::vector<double>& u, int j,
                              double& uL, double& uR) {
    double slopeL = u[j] - u[j - 1];
    double slopeR = u[j + 1] - u[j];
    double slopeC = 0.5 * (u[j + 1] - u[j - 1]);

    double limited = minmod3(slopeL, slopeC, slopeR);
    uL = u[j] + 0.5 * limited;
    uR = u[j + 1] - 0.5 * limited;
}

// ─── Runge-Kutta 2 (midpoint method, 2nd order) ────────────────

using OdeRhs = std::function<double(double, double)>; // f(t, y)

inline double rk2Midpoint(double t, double y, double dt, OdeRhs f) {
    double k1 = f(t, y);
    double k2 = f(t + 0.5 * dt, y + 0.5 * dt * k1);
    return y + dt * k2;
}

// ─── Courant number helper ─────────────────────────────────────

inline double courantNumber(double velocity, double dt, double dx) {
    if (dx <= 0.0) return 1e10;
    return std::abs(velocity) * dt / dx;
}

} // namespace NumericalMethods
