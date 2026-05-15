#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

enum class PortDirection {
    Input,
    Output
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
};
