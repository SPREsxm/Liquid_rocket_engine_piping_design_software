#pragma once

#include "core/Types.h"

#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVector>

class BlockScene;

struct OptimizationResult {
    struct PipeSelection {
        QUuid   blockUuid;
        QString blockLabel;
        double  oldNPS = 0.0;
        QString oldSchedule;
        double  newNPS = 0.0;
        QString newSchedule;
        double  oldWeight_kg = 0.0;
        double  newWeight_kg = 0.0;
        bool    changed = false;
    };

    QVector<PipeSelection> selections;
    double  originalTotalWeight_kg = 0.0;
    double  optimizedTotalWeight_kg = 0.0;
    double  weightSaved_kg = 0.0;
    int     iterationsRun = 0;
    bool    allConstraintsSatisfied = false;
    QStringList violatedConstraints;
};

// Run greedy discrete optimization over the NPS × schedule matrix
// for all straight-pipe blocks in the scene.
// Returns the best (lightest) valid schedule assignment found.
OptimizationResult optimizePipeSchedules(
    BlockScene* scene,
    const SolverSettings& baseSettings,
    double inletPressurePa = 1.0e6,
    double inletMassFlowKgPerS = 10.0,
    double maxPressureDropPa = -1.0);
