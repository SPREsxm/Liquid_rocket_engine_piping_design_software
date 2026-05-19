#pragma once

#include <QVector>
#include <QString>
#include <QUuid>

struct SolverSettings;
class BlockScene;
class BlockItem;

struct BlowdownSensorTrace {
    QUuid blockUuid;
    QString blockLabel;
    QString blockTypeId;
    QVector<double> times;
    QVector<double> pressures;
    QVector<double> flowRates;
    QVector<double> temperatures;
};

struct BlowdownResult {
    QVector<BlowdownSensorTrace> sensorTraces;
    QVector<double> timePoints;
    QString message;
    double totalDuration = 0.0;
    double totalFuelConsumed = 0.0;
    bool depleted = false;
    int stepsCompleted = 0;
};

class BlowdownSolver {
public:
    void setTimeStep(double dt) { m_timeStep = dt; }
    void setMaxDuration(double maxT) { m_maxDuration = maxT; }
    void setMinTankPressure(double pMin) { m_minPressure = pMin; }

    BlowdownResult simulate(
        BlockScene* scene,
        const SolverSettings& baseSettings,
        double initialInletPressurePa,
        double initialMassFlowKgPerS
    );

private:
    double m_timeStep = 0.5;        // s
    double m_maxDuration = 200.0;   // s
    double m_minPressure = 1.0e5;   // Pa
};
