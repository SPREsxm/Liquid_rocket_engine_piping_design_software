#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

class QSettings;

enum class PortDirection {
    Input,
    Output,
    Bidirectional
};

enum class PortDataType {
    Fluid,        // Liquid propellant flow
    Mechanical,   // Shaft, structural coupling
    Signal        // Sensor/control signal
};

enum class PropertyType {
    Double,
    Int,
    Bool,
    String,
    Enum,
    Expression  // runtime-evaluated ExprTk expression
};

struct PropertyDescriptor {
    QString id;
    QString displayName;
    PropertyType type = PropertyType::Double;
    QVariant defaultValue;
    QVariant minValue;
    QVariant maxValue;
    QString unit;               // e.g. "m", "Pa", "kg/s"
    QStringList enumOptions;    // only used when type == Enum
};

enum class FluidType {
    LOX,
    RP1,
    CH4,
    LH2,
    Water
};

// Approximate room-temperature liquid properties for default solver values.
// Temperature-dependent properties are computed by PropellantProperties when actual T is supplied.
struct FluidProperties {
    double density;   // kg/m^3
    double viscosity; // Pa*s
    double bulkModulus; // Pa — for water hammer / wave speed
};

inline FluidProperties fluidDefaults(FluidType type) {
    switch (type) {
    case FluidType::LOX:    return {1141.0, 1.96e-4, 9.6e8};
    case FluidType::RP1:    return {810.0,  7.5e-4, 1.3e9};
    case FluidType::CH4:    return {422.0,  1.03e-4, 7.5e8};
    case FluidType::LH2:    return {70.9,   1.32e-5, 2.3e8};
    case FluidType::Water:  return {998.0,  1.002e-3, 2.18e9};
    }
    return {1000.0, 1.0e-3, 2.0e9}; // fallback
}

// Solver configuration per architecture doc §2.4
struct SolverSettings {
    double tolerance = 1e-6;
    int maxIterations = 200;
    double relaxationFactor = 1.0;
    bool useAutoSolver = true;
    double hardyCrossTolerance = 1e-6;
    int hardyCrossMaxIter = 200;
    double matrixSolverTolerance = 1e-8;
    int matrixSolverMaxIter = 500;
    double targetCourant = 0.9;
    double timeStepSeconds = -1.0;  // -1 = auto
    int gridBaseNodes = 50;
    double fluidDensity = 1141.0;    // kg/m^3  (LOX default)
    double fluidViscosity = 1.96e-4; // Pa·s    (LOX default)
    bool useSSTTurbulence = true;    // SST k-ω vs Colebrook-White
    FluidType fluidType = FluidType::LOX; // working fluid for NPSH/cavitation checks

    // Pipe material properties (overridden by per-block properties when available)
    double pipeRoughness = 1.5e-6;     // m  (drawn SS tubing ~1.5 μm)
    double pipeYoungsModulus = 2.0e11; // Pa (stainless steel ~200 GPa)
    double pipeWallThickness = 0.001;  // m  (1 mm typical)
    double tankPressurePa = 101325.0;  // Pa (tank ullage pressure for NPSHa)

    // Read solver settings from QSettings, using struct defaults as fallbacks.
    static SolverSettings fromQSettings();
};
