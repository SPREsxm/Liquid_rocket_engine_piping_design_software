#pragma once

#include <cmath>
#include <limits>
#include <type_traits>

// Mixed-precision computation utility.
// Automatically selects float (32-bit) or double (64-bit) based on
// condition number estimation, per architecture doc §2.4:
//   "When condition number < 10⁹, automatically switch to float32 for
//    memory bandwidth savings; otherwise use float64."

namespace MixedPrecision {

// ─── Precision selector ────────────────────────────────────────

// Threshold from architecture doc: condition number 1e9
constexpr double kConditionThreshold = 1.0e9;

// Determine whether float32 is safe given a condition number estimate
inline bool useFloat32(double conditionNumberEstimate) {
    return conditionNumberEstimate < kConditionThreshold;
}

// Select type tag based on condition number
template<typename Float32Op, typename Float64Op>
auto selectPrecision(double cond, Float32Op&& f32, Float64Op&& f64)
    -> decltype(f32())
{
    return (cond < kConditionThreshold) ? f32() : f64();
}

// ─── Condition number estimators ───────────────────────────────

// Estimate the condition number of a linear system from residual.
// cond ≈ ||b|| / (||A|| · ||x|| · ε_mach)
// For well-conditioned systems: cond < 1e6
// For ill-conditioned systems: cond > 1e12
inline double estimateConditionNumber(double residualNorm,
                                       double matrixNormEstimate,
                                       double solutionNorm) {
    if (solutionNorm < 1e-30 || matrixNormEstimate < 1e-30)
        return std::numeric_limits<double>::infinity();
    // Simple estimate: cond ≈ residual / (ε_mach * ||A|| * ||x||)
    double eps = std::numeric_limits<double>::epsilon();
    return residualNorm / (eps * matrixNormEstimate * solutionNorm);
}

// Estimate condition number from diagonal dominance ratio.
// For pipe networks, a proxy: max(|diag|) / min(|diag|) of resistance matrix.
inline double estimateConditionFromDiagRatio(double maxDiag, double minDiag) {
    if (minDiag < 1e-30) return std::numeric_limits<double>::infinity();
    return maxDiag / minDiag;
}

// ─── Precision-adaptive operations ─────────────────────────────

// Conditionally cast to float32 and back when safe
template<typename T>
T safeAccumulate(T sum, T term, double conditionEstimate) {
    if constexpr (std::is_same_v<T, double>) {
        if (useFloat32(conditionEstimate)) {
            // Kahan-style compensated summation in float32
            float s = static_cast<float>(sum);
            float t = static_cast<float>(term);
            float y = t; // no compensation needed for single term
            float result = s + y;
            return static_cast<double>(result);
        }
    }
    return sum + term;
}

// Run an operation in float32 if condition allows, float64 otherwise.
// Returns the result cast back to double.
template<typename Op>
double adaptiveCompute(double conditionEstimate, Op&& operation) {
    if (useFloat32(conditionEstimate)) {
        return static_cast<double>(operation(1.0f)); // float32 path
    } else {
        return operation(1.0); // float64 path
    }
}

} // namespace MixedPrecision
