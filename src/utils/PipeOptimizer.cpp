#include "PipeOptimizer.h"
#include "NetworkSolver.h"
#include "DesignRules.h"
#include "BomGenerator.h"
#include "PipeScheduleDatabase.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"

#include <QSet>
#include <algorithm>

namespace {

double pipeWeight_kg(double nps, const QString& schedule, double length_m)
{
    const auto* entry = PipeScheduleDatabase::instance().lookup(nps, schedule);
    if (!entry) return 0.0;
    return entry->weightPerMeter * length_m;
}

// Collect all straight pipe blocks in the scene
QList<BlockItem*> findPipeBlocks(BlockScene* scene)
{
    QList<BlockItem*> pipes;
    for (auto* b : scene->allBlocks()) {
        if (b->typeId().startsWith("pipe."))
            pipes.append(b);
    }
    return pipes;
}

} // anonymous namespace

OptimizationResult optimizePipeSchedules(
    BlockScene* scene,
    const SolverSettings& baseSettings,
    double inletPressurePa,
    double inletMassFlowKgPerS,
    double maxPressureDropPa)
{
    OptimizationResult result;

    if (!scene) {
        result.violatedConstraints.append("Null scene");
        return result;
    }

    const auto& db = PipeScheduleDatabase::instance();
    QList<BlockItem*> pipes = findPipeBlocks(scene);
    if (pipes.isEmpty()) {
        result.allConstraintsSatisfied = true;
        return result;
    }

    // Store original NPS/schedule assignments and compute initial weight
    QHash<QUuid, double> origNPS;
    QHash<QUuid, QString> origSchedule;
    double origWeight = 0.0;
    for (auto* pipe : pipes) {
        double nps = pipe->propertyValue("nps").toDouble();
        QString sch = pipe->propertyValue("schedule").toString();
        if (sch.isEmpty()) sch = db.defaultSchedule(nps);
        origNPS[pipe->uuid()] = nps;
        origSchedule[pipe->uuid()] = sch;
        origWeight += pipeWeight_kg(nps, sch,
                                    pipe->propertyValue("length").toDouble());
    }
    result.originalTotalWeight_kg = origWeight;

    // ── Greedy optimization: 3 passes ─────────────────────────
    constexpr int kMaxPasses = 3;
    QSet<QUuid> improved;  // pipes improved this pass

    for (int pass = 0; pass < kMaxPasses; ++pass) {
        improved.clear();
        ++result.iterationsRun;

        for (auto* pipe : pipes) {
            double length = pipe->propertyValue("length").toDouble();
            if (length <= 0.0) length = 1.0;

            double bestNPS = origNPS[pipe->uuid()];
            QString bestSchedule = origSchedule[pipe->uuid()];
            double bestWeight = pipeWeight_kg(bestNPS, bestSchedule, length);
            bool foundValid = false;

            // Try every NPS × schedule combination
            const QStringList sizes = db.availableSizes();
            for (const auto& sizeStr : sizes) {
                double nps = sizeStr.toDouble();
                if (nps <= 0.0) continue;

                const QStringList schedules = db.schedulesForSize(nps);
                for (const auto& sch : schedules) {
                    double w = pipeWeight_kg(nps, sch, length);
                    // Skip if heavier than current best (or heavier than original,
                    // unless we've already found a lighter valid one)
                    if (foundValid && w >= bestWeight) continue;
                    if (!foundValid && w > bestWeight * 1.1) continue;

                    // Apply candidate
                    pipe->setPropertyValue("nps", nps);
                    pipe->setPropertyValue("schedule", sch);

                    // Update diameter from lookup table
                    auto innerD = db.innerDiameter(nps, sch);
                    if (innerD.has_value())
                        pipe->setPropertyValue("diameter", innerD.value() / 1000.0); // mm→m

                    // Re-solve network
                    NetworkSolution sol = solveNetworkAuto(
                        scene, baseSettings, inletPressurePa, inletMassFlowKgPerS);

                    // Run design checks
                    DesignCheckResult checks = runDesignChecks(
                        scene, sol, baseSettings, maxPressureDropPa);

                    // Check if all constraints pass (no errors)
                    bool allPass = (checks.errorCount() == 0);

                    if (allPass) {
                        foundValid = true;
                        if (w < bestWeight) {
                            bestNPS = nps;
                            bestSchedule = sch;
                            bestWeight = w;
                        }
                    }
                }
            }

            // Restore best (or original) settings
            if (foundValid && qAbs(bestWeight - pipeWeight_kg(origNPS[pipe->uuid()],
                                                               origSchedule[pipe->uuid()], length)) > 0.001) {
                improved.insert(pipe->uuid());
            }
            pipe->setPropertyValue("nps", bestNPS);
            pipe->setPropertyValue("schedule", bestSchedule);
            auto innerD = db.innerDiameter(bestNPS, bestSchedule);
            if (innerD.has_value())
                pipe->setPropertyValue("diameter", innerD.value() / 1000.0);

            origNPS[pipe->uuid()] = bestNPS;
            origSchedule[pipe->uuid()] = bestSchedule;
        }

        if (improved.isEmpty()) break; // converged
    }

    // ── Build result ─────────────────────────────────────────
    double optWeight = 0.0;
    for (auto* pipe : pipes) {
        double oldNps = pipe->propertyValue("nps").toDouble();
        QString oldSch = pipe->propertyValue("schedule").toString();
        double length = pipe->propertyValue("length").toDouble();
        if (length <= 0.0) length = 1.0;

        OptimizationResult::PipeSelection sel;
        sel.blockUuid = pipe->uuid();
        sel.blockLabel = pipe->customLabel().isEmpty() ? pipe->typeId() : pipe->customLabel();
        sel.newNPS = oldNps;
        sel.newSchedule = oldSch;
        sel.newWeight_kg = pipeWeight_kg(oldNps, oldSch, length);

        // Original weight
        QVariant savedNps = pipe->propertyValue("nps");  // already updated above
        sel.oldNPS = sel.newNPS;
        sel.oldSchedule = sel.newSchedule;
        sel.oldWeight_kg = sel.newWeight_kg;
        sel.changed = false;

        optWeight += sel.newWeight_kg;

        result.selections.append(sel);
    }

    result.optimizedTotalWeight_kg = optWeight;
    result.weightSaved_kg = result.originalTotalWeight_kg - optWeight;

    // Final verification: re-solve and check constraints
    NetworkSolution finalSol = solveNetworkAuto(
        scene, baseSettings, inletPressurePa, inletMassFlowKgPerS);
    DesignCheckResult finalChecks = runDesignChecks(
        scene, finalSol, baseSettings, maxPressureDropPa);

    result.allConstraintsSatisfied = (finalChecks.errorCount() == 0);
    for (const auto& item : finalChecks.items) {
        if (item.severity == DesignCheckResult::Error)
            result.violatedConstraints.append(item.message);
    }

    return result;
}
