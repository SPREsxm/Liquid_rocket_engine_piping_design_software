#pragma once

#include <QList>
#include <QString>
#include <QUuid>

class BlockScene;
struct NetworkSolution;
struct SolverSettings;
struct ThermalStressResult;

struct DesignCheckResult {
    enum Severity { Pass, Warning, Error };

    struct Item {
        Severity severity = Pass;
        QString ruleName;
        QString componentLabel;
        QUuid   componentUuid;
        QString message;
        double  actualValue = 0.0;
        double  limitValue = 0.0;
        QString unit;
    };

    QList<Item> items;

    int errorCount() const {
        int n = 0;
        for (const auto& it : items)
            if (it.severity == Error) ++n;
        return n;
    }
    int warningCount() const {
        int n = 0;
        for (const auto& it : items)
            if (it.severity == Warning) ++n;
        return n;
    }
};

DesignCheckResult runDesignChecks(
    BlockScene* scene,
    const NetworkSolution& solution,
    const SolverSettings& settings,
    double maxPressureDropPa = -1.0);

// Extended version that also checks pipe structural stress
DesignCheckResult runDesignChecks(
    BlockScene* scene,
    const NetworkSolution& solution,
    const SolverSettings& settings,
    double maxPressureDropPa,
    const ThermalStressResult& thermalStress);
