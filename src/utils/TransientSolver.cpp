#include "TransientSolver.h"
#include "NetworkSolver.h"
#include "FluidDynamics.h"
#include "MathStubs.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"

#include <QtMath>
#include <cmath>
#include <algorithm>
#include <QHash>
#include <QMap>

namespace {

// Material Young's modulus lookup (Pa)
QHash<QString, double> materialModulus()
{
    return {
        {"316L", 2.0e11},
        {"SS316L", 2.0e11},
        {"SS304", 1.93e11},
        {"Al2219", 7.1e10},
        {"Al6061", 6.9e10},
        {"Ti6Al4V", 1.1e11},
        {"Inconel718", 2.08e11},
        {"Copper", 1.17e11},
    };
}

// Default propellant for transient — LOX
constexpr double kDefaultRho = 1141.0;
constexpr double kDefaultMu  = 1.96e-4;
constexpr double kDefaultK   = 1.0e9;

// Default pipe geometry
constexpr double kDefaultDiameter = 0.0254;
constexpr double kDefaultRoughness = 0.000045;

double getBlockProp(BlockItem *b, const QString &id, double fallback) {
    QVariant v = b->propertyValue(id);
    if (v.isValid()) {
        bool ok = false;
        double val = v.toDouble(&ok);
        if (ok && val > 0.0) return val;
    }
    return fallback;
}

// Build the flow path in order from inlet to outlet
bool buildFlowPath(BlockScene *scene,
                   const NetworkSolution &steady,
                   std::vector<BlockItem *> &pathBlocks,
                   double &totalLength)
{
    if (!scene || steady.edges.isEmpty() || steady.nodes.isEmpty())
        return false;

    // Map block uuid → BlockItem*
    QHash<QUuid, BlockItem *> blockMap;
    for (auto *b : scene->allBlocks())
        blockMap[b->uuid()] = b;

    // Map source uuid → (dest uuid, block)
    struct EdgeInfo { QUuid src; QUuid dst; };
    QList<EdgeInfo> edgeList;
    for (const auto &e : steady.edges)
        edgeList.append({e.sourceUuid, e.destUuid});

    // Find inlet: a node with no incoming edge as source
    QSet<QUuid> destUuids;
    for (const auto &e : edgeList)
        destUuids.insert(e.dst);

    QUuid currentUuid;
    for (const auto &e : edgeList) {
        if (!destUuids.contains(e.src)) {
            currentUuid = e.src;
            break;
        }
    }
    if (currentUuid.isNull() && !edgeList.isEmpty())
        currentUuid = edgeList.first().src;

    // Walk the chain
    QSet<QUuid> visited;
    pathBlocks.clear();
    totalLength = 0.0;

    while (!currentUuid.isNull() && !visited.contains(currentUuid)) {
        visited.insert(currentUuid);
        auto *blk = blockMap.value(currentUuid);
        if (blk) {
            pathBlocks.push_back(blk);
            totalLength += getBlockProp(blk, "length", 0.3);
        }

        // Find next edge in chain
        QUuid nextUuid;
        for (const auto &e : edgeList) {
            if (e.src == currentUuid && !visited.contains(e.dst)) {
                nextUuid = e.dst;
                break;
            }
        }
        currentUuid = nextUuid;
    }

    // Add final destination node (valve/outlet)
    if (!currentUuid.isNull()) {
        auto *blk = blockMap.value(currentUuid);
        if (blk) pathBlocks.push_back(blk);
    }

    return pathBlocks.size() >= 2;
}

} // anonymous namespace

double TransientSolver::computeWaveSpeed(const PipeSegment &seg) const
{
    return MathStubs::calculateWaveSpeed(
        seg.bulkModulus, seg.density,
        seg.youngsModulus, seg.diameter, seg.wallThickness);
}

double TransientSolver::frictionSlope(double velocity, double diameter,
                                       double roughness, double density,
                                       double viscosity) const
{
    if (diameter <= 0.0 || density <= 0.0) return 0.0;

    const double reynolds = MathStubs::calculateReynoldsNumber(
        std::abs(velocity), diameter, density, viscosity);
    const double lambda = FluidDynamics::calculateColebrookWhiteFrictionFactor(
        reynolds, roughness, diameter);
    // Darcy-Weisbach per-unit-length slope: λ/(2d) * v|v|
    return lambda / (2.0 * diameter) * velocity * std::abs(velocity);
}

double TransientSolver::computeAdaptiveDt(
    double waveSpeed, double dx,
    const std::vector<double>& velocities) const
{
    double maxCharacteristic = waveSpeed;
    for (double v : velocities) {
        double characteristic = std::abs(v) + waveSpeed;
        if (characteristic > maxCharacteristic)
            maxCharacteristic = characteristic;
    }
    double dt = m_targetCFL * dx / maxCharacteristic;
    return std::max(dt, 1.0e-7);
}

TransientSolver::PipeParams TransientSolver::preparePipeParams(
    const NetworkSolution& steady, BlockScene* scene, int /*spatialNodes*/) const
{
    PipeParams pp;
    // Apply configurable defaults
    pp.roughness = m_defaultRoughness;
    pp.youngsModulus = m_defaultYoungsModulus;

    // Extract from path blocks for fluid/material properties
    QHash<QUuid, BlockItem*> blockMap;
    for (auto* b : scene->allBlocks())
        blockMap[b->uuid()] = b;

    for (const auto& edge : steady.edges) {
        auto* b = blockMap.value(edge.sourceUuid);
        if (!b) continue;

        double d = getBlockProp(b, "diameter", -1.0);
        if (d > 0.0) {
            pp.diameter = d;
            pp.wallThickness = (m_defaultWallThickness > 0.0)
                ? m_defaultWallThickness : d / 20.0;
        }
        double r = getBlockProp(b, "roughness", -1.0);
        if (r >= 0.0) pp.roughness = r; // per-block override

        QVariant mat = b->propertyValue("material");
        if (mat.isValid()) {
            const auto modMap = materialModulus();
            auto it = modMap.find(mat.toString());
            if (it != modMap.end()) pp.youngsModulus = it.value(); // per-block override
        }
        break; // use first pipe block
    }

    // Find inlet pressure from steady solution
    for (const auto& node : steady.nodes) {
        if (node.pressure > pp.inletPressure * 0.1)
            pp.inletPressure = node.pressure + steady.totalPressureDrop;
    }

    // Compute velocity from mass flow
    if (!steady.edges.isEmpty()) {
        double massFlow = std::abs(steady.edges.first().massFlowRate);
        double area = M_PI * pp.diameter * pp.diameter / 4.0;
        pp.initialVelocity = massFlow / (pp.density * area);
        if (pp.initialVelocity < 0.01) pp.initialVelocity = 5.0;
    }

    return pp;
}

QString TransientSolver::formatWaterHammerMessage(
    const PipeParams& pp, int pathBlocks, double totalLength,
    double closureTime, int spatialNodes, int totalSteps,
    double dt, double maxPressure, double maxPressureTime) const
{
    double maxPressureMPa = maxPressure / 1.0e6;
    double joukowsky = pp.density * pp.waveSpeed * pp.initialVelocity;

    return QStringLiteral(
        "Water hammer simulation complete:\n"
        "  Path: %1 blocks, L=%2 m, d=%3 mm, c=%4 m/s\n"
        "  Initial velocity: %5 m/s, Closure time: %6 s\n"
        "  Grid: N=%7, dt=%8 ms, steps=%9\n"
        "  Max pressure: %10 MPa at t=%11 ms\n"
        "  Joukowsky estimate: %12 MPa (Δp=ρcΔv)")
        .arg(pathBlocks)
        .arg(totalLength, 0, 'f', 2)
        .arg(pp.diameter * 1000.0, 0, 'f', 1)
        .arg(pp.waveSpeed, 0, 'f', 1)
        .arg(pp.initialVelocity, 0, 'f', 2)
        .arg(closureTime, 0, 'f', 3)
        .arg(spatialNodes)
        .arg(dt * 1000.0, 0, 'f', 3)
        .arg(totalSteps)
        .arg(maxPressureMPa, 0, 'f', 3)
        .arg(maxPressureTime * 1000.0, 0, 'f', 1)
        .arg(joukowsky / 1.0e6, 0, 'f', 3);
}

TransientResult TransientSolver::simulateWaterHammer(
    const NetworkSolution &steady,
    BlockScene *scene,
    double closureTime,
    int spatialNodes,
    double timeStepSeconds)
{
    TransientResult result;

    if (!scene) {
        result.message = QStringLiteral("Transient: no scene available.");
        return result;
    }

    if (steady.edges.isEmpty()) {
        result.message = QStringLiteral("Transient: steady solution has no edges.");
        return result;
    }

    // 1. Build flow path
    std::vector<BlockItem *> pathBlocks;
    double totalLength = 0.0;
    if (!buildFlowPath(scene, steady, pathBlocks, totalLength)) {
        result.message = QStringLiteral("Transient: failed to build flow path.");
        return result;
    }

    if (spatialNodes < 10) spatialNodes = 50;
    if (spatialNodes > 500) spatialNodes = 200;

    // 2. Extract pipe parameters and initial conditions
    PipeParams pp = preparePipeParams(steady, scene, spatialNodes);

    // 3. Compute wave speed
    pp.waveSpeed = computeWaveSpeed({totalLength, pp.diameter, pp.wallThickness,
                                      pp.youngsModulus, pp.roughness,
                                      pp.density, pp.viscosity, pp.bulkModulus});
    if (pp.waveSpeed <= 0.0) {
        result.message = QStringLiteral("Transient: invalid wave speed.");
        return result;
    }

    // 4. Discretization: Courant condition
    double dx = totalLength / spatialNodes;
    double dt;
    if (timeStepSeconds > 0.0) {
        dt = timeStepSeconds;
        double dtCFL = m_targetCFL * dx / (pp.initialVelocity + pp.waveSpeed);
        if (dt > dtCFL) dt = dtCFL;
    } else {
        dt = dx / pp.waveSpeed * m_targetCFL;
    }

    double simTime = closureTime * 2.5;
    int totalSteps = static_cast<int>(simTime / dt);
    if (totalSteps > 50000) totalSteps = 50000;
    if (totalSteps < 100) totalSteps = 100;
    dt = simTime / totalSteps;
    result.spatialNodes = spatialNodes;
    result.timeSteps = totalSteps;

    // 5. MOC state initialization
    std::vector<double> p(spatialNodes + 1, pp.inletPressure);
    std::vector<double> v(spatialNodes + 1, pp.initialVelocity);
    std::vector<double> pNew(spatialNodes + 1);
    std::vector<double> vNew(spatialNodes + 1);

    const double rho_c = pp.density * pp.waveSpeed;
    double maxPressure = pp.inletPressure;
    double maxPressureTime = 0.0;

    // 6. Time stepping (Method of Characteristics)
    for (int step = 0; step < totalSteps; ++step) {
        double t = step * dt;

        // Interior nodes: C+ from i-1, C- from i+1
        for (int i = 1; i < spatialNodes; ++i) {
            double fA = frictionSlope(v[i - 1], pp.diameter, pp.roughness, pp.density, pp.viscosity);
            double cp = p[i - 1] + rho_c * v[i - 1] - rho_c * fA * dx;

            double fB = frictionSlope(v[i + 1], pp.diameter, pp.roughness, pp.density, pp.viscosity);
            double cm = p[i + 1] - rho_c * v[i + 1] + rho_c * fB * dx;

            pNew[i] = 0.5 * (cp + cm);
            vNew[i] = 0.5 * (cp - cm) / rho_c;
        }

        // Upstream boundary: constant pressure reservoir
        pNew[0] = pp.inletPressure;
        double cm0 = p[1] - rho_c * v[1] + rho_c * frictionSlope(v[1], pp.diameter, pp.roughness, pp.density, pp.viscosity) * dx;
        vNew[0] = (pNew[0] - cm0) / rho_c;

        // Downstream boundary: closing valve
        int N = spatialNodes;
        double cpN = p[N - 1] + rho_c * v[N - 1] - rho_c * frictionSlope(v[N - 1], pp.diameter, pp.roughness, pp.density, pp.viscosity) * dx;

        double valveFraction = (t < closureTime) ? (1.0 - t / closureTime) : 0.0;
        vNew[N] = pp.initialVelocity * valveFraction * valveFraction;
        pNew[N] = cpN - rho_c * vNew[N];

        // Track maximum pressure
        for (int i = 0; i <= spatialNodes; ++i) {
            if (pNew[i] > maxPressure) {
                maxPressure = pNew[i];
                maxPressureTime = t;
            }
        }

        // Adaptive CFL every 10 steps
        if (step % 10 == 0 && timeStepSeconds <= 0.0) {
            double newDt = computeAdaptiveDt(pp.waveSpeed, dx, vNew);
            dt = std::max(newDt, 1.0e-7);
            dt = std::min(dt, dx / pp.waveSpeed);
        }

        // Record state
        if (step % 20 == 0 || step == totalSteps - 1) {
            result.history.push_back({t, pNew, vNew});
        }

        p.swap(pNew);
        v.swap(vNew);
    }

    result.maxPressure = maxPressure;
    result.maxPressureTime = maxPressureTime;
    result.spatialNodes = spatialNodes;
    result.timeSteps = totalSteps;
    result.message = formatWaterHammerMessage(pp, static_cast<int>(pathBlocks.size()), totalLength,
                                               closureTime, spatialNodes,
                                               totalSteps, dt,
                                               maxPressure, maxPressureTime);

    return result;
}
