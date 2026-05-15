#include "Types.h"
#include <QSettings>

SolverSettings SolverSettings::fromQSettings()
{
    QSettings s;
    SolverSettings ss;
    ss.tolerance = s.value("Solver/Tolerance", ss.tolerance).toDouble();
    ss.maxIterations = s.value("Solver/MaxIter", ss.maxIterations).toInt();
    ss.relaxationFactor = s.value("Solver/Relaxation", ss.relaxationFactor).toDouble();
    ss.hardyCrossTolerance = s.value("Solver/HardyCrossTol", ss.hardyCrossTolerance).toDouble();
    ss.hardyCrossMaxIter = s.value("Solver/HardyCrossMaxIter", ss.hardyCrossMaxIter).toInt();
    ss.matrixSolverTolerance = s.value("Solver/MatrixTol", ss.matrixSolverTolerance).toDouble();
    ss.matrixSolverMaxIter = s.value("Solver/MatrixMaxIter", ss.matrixSolverMaxIter).toInt();
    ss.targetCourant = s.value("Solver/Courant", ss.targetCourant).toDouble();
    ss.timeStepSeconds = s.value("Solver/TimeStep", ss.timeStepSeconds).toDouble();
    ss.gridBaseNodes = s.value("Solver/GridNodes", ss.gridBaseNodes).toInt();
    ss.fluidDensity = s.value("Solver/FluidDensity", ss.fluidDensity).toDouble();
    ss.fluidViscosity = s.value("Solver/FluidViscosity", ss.fluidViscosity).toDouble();
    return ss;
}
