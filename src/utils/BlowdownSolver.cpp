#include "BlowdownSolver.h"
#include "NetworkSolver.h"
#include "core/Types.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"

#include <QSet>
#include <cmath>

namespace {

double getProp(BlockItem* b, const QString& id, double fallback) {
    QVariant v = b->propertyValue(id);
    if (v.isValid()) {
        bool ok = false;
        double val = v.toDouble(&ok);
        if (ok) return val;
    }
    return fallback;
}

double fluidTemperatureFromType(FluidType ft) {
    switch (ft) {
    case FluidType::LOX:   return 90.0;   // LOX boiling point ~90 K
    case FluidType::RP1:   return 293.0;
    case FluidType::CH4:   return 112.0;  // CH4 boiling point ~112 K
    case FluidType::LH2:   return 20.0;   // LH2 boiling point ~20 K
    case FluidType::Water: return 293.0;
    }
    return 293.0;
}

struct TankState {
    BlockItem* block;
    double totalVolume;          // m³
    double initialGasVolume;     // m³
    double initialGasPressure;   // Pa
    double propellantMass;       // kg (current)
    double propellantDensity;    // kg/m³
    double currentPressure;      // Pa
};

} // anonymous namespace

BlowdownResult BlowdownSolver::simulate(
    BlockScene* scene,
    const SolverSettings& baseSettings,
    double initialInletPressurePa,
    double initialMassFlowKgPerS)
{
    BlowdownResult result;

    if (!scene || scene->allBlocks().isEmpty()) {
        result.message = QStringLiteral("Scene is empty — no blocks to simulate.");
        return result;
    }

    // ── 1. Identify tank blocks ──────────────────────────────
    QList<TankState> tanks;
    QSet<BlockItem*> sensorBlocks;

    for (auto* b : scene->allBlocks()) {
        const QString& tid = b->typeId();

        // Tanks that serve as propellant sources
        if (tid == "tank.storage") {
            double ullageP = getProp(b, "ullagePressure", 0.0);
            if (ullageP > 0.0) {
                TankState ts;
                ts.block = b;
                ts.totalVolume = getProp(b, "volume", 10.0);
                double ullageFrac = getProp(b, "ullageFraction", 0.2);
                ts.initialGasVolume = ullageFrac * ts.totalVolume;
                ts.initialGasPressure = ullageP;
                ts.propellantMass = getProp(b, "storedMass",
                    ts.totalVolume * (1.0 - ullageFrac) * fluidDefaults(baseSettings.fluidType).density);
                ts.propellantDensity = fluidDefaults(baseSettings.fluidType).density;
                ts.currentPressure = ullageP;
                tanks.append(ts);
            }
        }

        // Collect sensor blocks for data recording
        if (tid.startsWith("sensor.") || tid == "tank.storage" || tid == "tank.buffer") {
            sensorBlocks.insert(b);
        }
    }

    if (tanks.isEmpty()) {
        result.message = QStringLiteral("No propellant tanks found (tank.storage with ullagePressure > 0).");
        return result;
    }

    // ── 2. Determine chamber back-pressure for cutoff ────────
    double chamberPressure = 0.0;
    for (auto* b : scene->allBlocks()) {
        if (b->typeId().startsWith("chamber.")) {
            double envP = getProp(b, "outletEnvironmentPressure", 0.0);
            if (envP > chamberPressure)
                chamberPressure = envP;
        }
    }

    // ── 3. Initialize sensor traces ──────────────────────────
    QHash<BlockItem*, int> traceIndex;
    for (auto* sb : sensorBlocks) {
        BlowdownSensorTrace trace;
        trace.blockUuid = sb->uuid();
        trace.blockLabel = sb->customLabel().isEmpty() ? sb->typeId() : sb->customLabel();
        trace.blockTypeId = sb->typeId();
        traceIndex[sb] = result.sensorTraces.size();
        result.sensorTraces.append(trace);
    }

    // ── 4. Time-stepping loop ────────────────────────────────
    const double dt = m_timeStep;
    const double maxT = m_maxDuration;
    double fluidTemp = fluidTemperatureFromType(baseSettings.fluidType);

    int maxSteps = static_cast<int>(maxT / dt) + 1;
    result.timePoints.reserve(maxSteps);

    double t = 0.0;
    bool stopped = false;

    for (int step = 0; step < maxSteps && !stopped; ++step) {
        t = step * dt;
        result.timePoints.append(t);

        // ── 4a. Set current tank pressures as inlet BCs ──────
        double maxTankPressure = 0.0;
        for (auto& tank : tanks) {
            tank.block->setPropertyValue("currentInletPressure", tank.currentPressure);
            if (tank.currentPressure > maxTankPressure)
                maxTankPressure = tank.currentPressure;
        }

        // Use the highest tank pressure as fallback inlet pressure
        double solveInletP = (maxTankPressure > 0.0) ? maxTankPressure : initialInletPressurePa;
        double solveFlow = initialMassFlowKgPerS;

        // ── 4b. Run steady-state solver ──────────────────────
        SolverSettings settings = baseSettings;
        auto sol = solveNetworkAuto(scene, settings, solveInletP, solveFlow);

        if (!sol.converged && step > 0) {
            // If solver fails after first step, stop
            if (step == 0) {
                result.message = QStringLiteral("Steady-state solver did not converge at t=0.");
                // Clean up property
                for (auto& tank : tanks)
                    tank.block->setPropertyValue("currentInletPressure", QVariant());
                return result;
            }
            stopped = true;
            break;
        }

        // ── 4c. Record sensor data ──────────────────────────
        // Build a node lookup from solution
        QHash<QUuid, const NodeState*> nodeByUuid;
        for (const auto& ns : sol.nodes)
            nodeByUuid[ns.blockUuid] = &ns;

        // Build edge flow lookup by destination node
        QHash<QUuid, double> flowIntoNode;
        for (const auto& es : sol.edges) {
            flowIntoNode[es.destUuid] += es.massFlowRate;
        }

        for (auto* sb : sensorBlocks) {
            int idx = traceIndex.value(sb, -1);
            if (idx < 0) continue;

            auto& trace = result.sensorTraces[idx];
            const NodeState* ns = nodeByUuid.value(sb->uuid(), nullptr);

            trace.times.append(t);
            trace.pressures.append(ns ? ns->pressure : 0.0);
            trace.flowRates.append(flowIntoNode.value(sb->uuid(), 0.0));
            trace.temperatures.append(fluidTemp);
        }

        // ── 4d. Compute total outlet flow rate ───────────────
        double totalOutletFlow = 0.0;
        for (const auto& ns : sol.nodes) {
            if (ns.blockTypeId.startsWith("chamber.")) {
                totalOutletFlow += ns.inletFlow;
            }
        }
        // Fallback: sum all edge flows from tanks
        if (totalOutletFlow <= 0.0) {
            for (const auto& es : sol.edges) {
                for (const auto& tank : tanks) {
                    if (es.sourceUuid == tank.block->uuid())
                        totalOutletFlow += es.massFlowRate;
                }
            }
        }

        // ── 4e. Update tank states ───────────────────────────
        bool allDepleted = true;
        for (auto& tank : tanks) {
            if (tank.propellantMass <= 0.0) {
                tank.currentPressure = 0.0;
                continue;
            }
            allDepleted = false;

            // Deduct propellant mass: distribute flow proportionally
            double tankFlowShare = totalOutletFlow;
            if (tanks.size() > 1) {
                // Each tank contributes in proportion to its current pressure
                double pressureSum = 0.0;
                for (const auto& t2 : tanks)
                    pressureSum += std::max(t2.currentPressure, 0.0);
                if (pressureSum > 0.0)
                    tankFlowShare = totalOutletFlow * tank.currentPressure / pressureSum;
                else
                    tankFlowShare = totalOutletFlow / tanks.size();
            }

            double dm = tankFlowShare * dt;
            // Only consume what remains — don't go negative
            double actualDm = qMin(dm, tank.propellantMass);
            tank.propellantMass -= actualDm;
            result.totalFuelConsumed += actualDm;

            // Update gas pressure (isothermal expansion)
            if (tank.propellantMass <= 0.0) {
                tank.currentPressure = 0.0;
            } else {
                double propVol = tank.propellantMass / tank.propellantDensity;
                double gasVol = tank.totalVolume - propVol;
                if (gasVol <= tank.initialGasVolume)
                    tank.currentPressure = tank.initialGasPressure;
                else
                    tank.currentPressure = tank.initialGasPressure * tank.initialGasVolume / gasVol;

                if (tank.currentPressure < m_minPressure)
                    tank.currentPressure = 0.0;
            }
        }

        result.stepsCompleted = step + 1;

        // ── 4f. Check stop conditions ────────────────────────
        // All tanks empty
        if (allDepleted) {
            result.depleted = true;
            stopped = true;
        }

        // All tank pressures below chamber pressure
        bool allBelowChamber = true;
        for (const auto& tank : tanks) {
            if (tank.currentPressure > chamberPressure) {
                allBelowChamber = false;
                break;
            }
        }
        if (allBelowChamber && chamberPressure > 0.0) {
            result.depleted = true;
            stopped = true;
        }

        // All tank pressures at zero
        bool allZero = true;
        for (const auto& tank : tanks) {
            if (tank.currentPressure > 0.0) {
                allZero = false;
                break;
            }
        }
        if (allZero) {
            result.depleted = true;
            stopped = true;
        }
    }

    // ── 5. Clean up ──────────────────────────────────────────
    for (auto& tank : tanks)
        tank.block->setPropertyValue("currentInletPressure", QVariant());

    // ── 6. Build result message ──────────────────────────────
    result.totalDuration = t;
    if (result.depleted)
        result.message = QStringLiteral("[Blowdown] Propellant depleted at t=%1 s, total consumed=%2 kg, steps=%3")
            .arg(t, 0, 'f', 1).arg(result.totalFuelConsumed, 0, 'f', 1).arg(result.stepsCompleted);
    else if (stopped)
        result.message = QStringLiteral("[Blowdown] Stopped at t=%1 s, steps=%2 (solver convergence issue)")
            .arg(t, 0, 'f', 1).arg(result.stepsCompleted);
    else
        result.message = QStringLiteral("[Blowdown] Completed %1 steps over %2 s, total consumed=%3 kg")
            .arg(result.stepsCompleted).arg(t, 0, 'f', 1).arg(result.totalFuelConsumed, 0, 'f', 1);

    return result;
}
