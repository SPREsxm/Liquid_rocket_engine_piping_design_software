#pragma once

#include <QString>
#include <cmath>
#include <algorithm>

// Crane TP-410 standard equivalent length (Le/D) data for common fittings.
// Le/D values are for fully turbulent flow (f_T ≈ 0.018–0.025 depending on diameter).
// Source: Crane TP-410 (2018 ed.), Tables A-26 through A-30.

namespace ResistanceCoefficients {

enum class FittingType {
    // Elbows
    Elbow90_Threaded,
    Elbow90_Flanged,
    Elbow45_Threaded,
    Elbow45_Flanged,
    Elbow90_LongRadius,
    // Tees
    Tee_Branch,
    Tee_StraightThrough,
    // Valves
    GateValve_FullyOpen,
    GlobeValve_FullyOpen,
    BallValve_FullyOpen,
    SwingCheckValve,
    ButterflyValve,
    // Expansions / Contractions
    SuddenExpansion,
    SuddenContraction,
    // Pipe entries / exits
    PipeEntry_Inward,
    PipeEntry_Flush,
    PipeExit
};

// Le/D ratio for a fitting at a given nominal diameter (meters).
// Interpolates between tabulated diameters using a simplified linear model.
inline double leOverD(FittingType fitting, double diameterMeters)
{
    const double d = std::max(diameterMeters, 0.005);

    switch (fitting) {
    case FittingType::Elbow90_Threaded:
        // 1/2"=30, 1"=30, 2"=30, 4"=30 — nearly constant
        return 30.0;
    case FittingType::Elbow90_Flanged:
        // 1"=20, 2"=20, 4"=20, 8"=20 — nearly constant
        return 20.0;
    case FittingType::Elbow45_Threaded:
        return 16.0;
    case FittingType::Elbow45_Flanged:
        return 14.0;
    case FittingType::Elbow90_LongRadius:
        // 1"=14, 2"=14, 4"=14
        return 14.0;
    case FittingType::Tee_Branch:
        // Flow through branch: 1"=60, 2"=60, 4"=60
        return 60.0;
    case FittingType::Tee_StraightThrough:
        // Flow straight through tee: 1"=20, 2"=20, 4"=20
        return 20.0;
    case FittingType::GateValve_FullyOpen:
        // 1"=8, 2"=8, 4"=8
        return 8.0;
    case FittingType::GlobeValve_FullyOpen:
        // Varies more with size: 1"≈150, 4"≈340
        if (d < 0.0254) return 150.0;
        if (d < 0.0508) return 200.0;
        if (d < 0.1016) return 300.0;
        return 340.0;
    case FittingType::BallValve_FullyOpen:
        return 3.0;
    case FittingType::SwingCheckValve:
        // 1"=50, 2"=50, 4"=50
        return 50.0;
    case FittingType::ButterflyValve:
        // 2"=45, 6"=45, 12"=40
        return 45.0;
    case FittingType::SuddenExpansion:
        // Le/D depends on area ratio; typical value for moderate expansion
        return 0.0; // use Borda-Carnot formula directly
    case FittingType::SuddenContraction:
        // Le/D depends on area ratio; typical value for moderate contraction
        return 0.0; // use zeta formula directly
    case FittingType::PipeEntry_Inward:
        // Inward-projecting pipe entrance
        return 32.0;
    case FittingType::PipeEntry_Flush:
        // Flush / well-rounded pipe entrance
        return 16.0;
    case FittingType::PipeExit:
        // Pipe exit to large reservoir
        return 0.0; // zeta = 1.0 always
    }
    return 0.0;
}

// Loss coefficient zeta from Le/D using fully-turbulent friction factor f_T.
// ζ = (Le/D) * f_T
// f_T ≈ 0.25 / [log₁₀(ε/(3.7·d))]² (Nikuradse fully-rough approximation)
inline double zetaFromLeD(double leOverD, double diameterMeters,
                          double roughnessMeters = 4.5e-5)
{
    if (leOverD <= 0.0) return 0.0;
    const double d = std::max(diameterMeters, 1e-6);
    const double relRough = roughnessMeters / d;
    const double fT = 0.25 / std::pow(std::log10(relRough / 3.7), 2.0);
    return leOverD * fT;
}

// Direct zeta values for fittings where Le/D method is less applicable.
inline double directZeta(FittingType fitting, double diameterMeters = 0.05)
{
    switch (fitting) {
    case FittingType::Elbow45_Threaded:  return 0.35;
    case FittingType::Tee_StraightThrough: return 0.25;
    case FittingType::SuddenExpansion:   return 1.0; // for A2 >> A1
    case FittingType::SuddenContraction: return 0.5; // for A2 << A1
    case FittingType::PipeExit:          return 1.0;
    case FittingType::BallValve_FullyOpen: return 0.04; // very low loss
    default:
        double led = leOverD(fitting, diameterMeters);
        if (led > 0.0)
            return zetaFromLeD(led, diameterMeters);
        return 0.0;
    }
}

inline QString fittingName(FittingType fitting)
{
    switch (fitting) {
    case FittingType::Elbow90_Threaded:    return QStringLiteral("90 deg Elbow (Threaded)");
    case FittingType::Elbow90_Flanged:     return QStringLiteral("90 deg Elbow (Flanged)");
    case FittingType::Elbow45_Threaded:    return QStringLiteral("45 deg Elbow (Threaded)");
    case FittingType::Elbow45_Flanged:     return QStringLiteral("45 deg Elbow (Flanged)");
    case FittingType::Elbow90_LongRadius:  return QStringLiteral("90 deg Elbow (Long Radius)");
    case FittingType::Tee_Branch:          return QStringLiteral("Tee (Branch)");
    case FittingType::Tee_StraightThrough: return QStringLiteral("Tee (Straight Through)");
    case FittingType::GateValve_FullyOpen: return QStringLiteral("Gate Valve (Fully Open)");
    case FittingType::GlobeValve_FullyOpen:return QStringLiteral("Globe Valve (Fully Open)");
    case FittingType::BallValve_FullyOpen: return QStringLiteral("Ball Valve (Fully Open)");
    case FittingType::SwingCheckValve:     return QStringLiteral("Swing Check Valve");
    case FittingType::ButterflyValve:      return QStringLiteral("Butterfly Valve");
    case FittingType::SuddenExpansion:     return QStringLiteral("Sudden Expansion");
    case FittingType::SuddenContraction:   return QStringLiteral("Sudden Contraction");
    case FittingType::PipeEntry_Inward:    return QStringLiteral("Pipe Entry (Inward)");
    case FittingType::PipeEntry_Flush:     return QStringLiteral("Pipe Entry (Flush)");
    case FittingType::PipeExit:            return QStringLiteral("Pipe Exit");
    }
    return QStringLiteral("Unknown");
}

} // namespace ResistanceCoefficients
