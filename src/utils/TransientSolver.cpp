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

    // 2. Aggregate pipe parameters (equivalent uniform pipe for simplicity)
    double diameter = kDefaultDiameter;
    double roughness = kDefaultRoughness;
    double density = kDefaultRho;
    double viscosity = kDefaultMu;
    double bulkModulus = kDefaultK;
    double youngsModulus = 2.0e11; // default 316L
    double wallThickness = diameter / 20.0;

    // Extract from first pipe block in path for fluid/material properties
    for (auto *b : pathBlocks) {
        double d = getBlockProp(b, "diameter", -1.0);
        if (d > 0.0) {
            diameter = d;
            wallThickness = d / 20.0;
        }
        double r = getBlockProp(b, "roughness", -1.0);
        if (r >= 0.0) roughness = r;

        // Material lookup
        QVariant mat = b->propertyValue("material");
        if (mat.isValid()) {
            const auto modMap = materialModulus();
            auto it = modMap.find(mat.toString());
            if (it != modMap.end()) youngsModulus = it.value();
        }
    }

    // Use LOX properties as default for liquid rocket context
    bulkModulus = kDefaultK;
    density = kDefaultRho;
    viscosity = kDefaultMu;

    // 3. Compute wave speed
    double c = computeWaveSpeed({totalLength, diameter, wallThickness,
                                  youngsModulus, roughness,
                                  density, viscosity, bulkModulus});
    if (c <= 0.0) {
        result.message = QStringLiteral("Transient: invalid wave speed.");
        return result;
    }

    // 4. Find inlet pressure and initial velocity from steady solution
    double inletPressure = 1.0e6; // default 1 MPa
    double initialVelocity = 0.0;

    for (const auto &node : steady.nodes) {
        if (node.pressure > inletPressure * 0.1)
            inletPressure = node.pressure + steady.totalPressureDrop;
    }

    // Compute velocity from mass flow
    if (!steady.edges.isEmpty()) {
        double massFlow = std::abs(steady.edges.first().massFlowRate);
        double area = M_PI * diameter * diameter / 4.0;
        initialVelocity = massFlow / (density * area);
        if (initialVelocity < 0.01) initialVelocity = 5.0; // reasonable default
    }

    // 5. Discretization: Courant condition dt = dx / c
    double dx = totalLength / spatialNodes;

    // Compute time step: user-specified, or auto CFL
    double dt;
    if (timeStepSeconds > 0.0) {
        dt = timeStepSeconds;
        // Ensure CFL constraint is met: dt must satisfy (|v|+c)*dt/dx ≤ 1
        double maxV = initialVelocity;
        double dtCFL = m_targetCFL * dx / (maxV + c);
        if (dt > dtCFL) dt = dtCFL;
    } else {
        dt = dx / c * m_targetCFL;
    }

    double simTime = closureTime * 2.5;
    int totalSteps = static_cast<int>(simTime / dt);
    if (totalSteps > 50000) totalSteps = 50000;
    if (totalSteps < 100) totalSteps = 100;
    dt = simTime / totalSteps;
    result.spatialNodes = spatialNodes;
    result.timeSteps = totalSteps;

    // 6. MOC solver initialization
    std::vector<double> p(spatialNodes + 1, inletPressure);  // pressure
    std::vector<double> v(spatialNodes + 1, initialVelocity); // velocity
    std::vector<double> pNew(spatialNodes + 1);
    std::vector<double> vNew(spatialNodes + 1);

    const double rho_c = density * c;
    double maxPressure = inletPressure;
    double maxPressureTime = 0.0;

    // 7. Time stepping
    for (int step = 0; step < totalSteps; ++step) {
        double t = step * dt;

        // Interior nodes (i = 1 .. N-1): C+ and C- characteristics
        for (int i = 1; i < spatialNodes; ++i) {
            // C+: from i-1
            double fA = frictionSlope(v[i - 1], diameter, roughness, density, viscosity);
            double cp = p[i - 1] + rho_c * v[i - 1] - rho_c * fA * dx;

            // C-: from i+1
            double fB = frictionSlope(v[i + 1], diameter, roughness, density, viscosity);
            double cm = p[i + 1] - rho_c * v[i + 1] + rho_c * fB * dx;

            pNew[i] = 0.5 * (cp + cm);
            vNew[i] = 0.5 * (cp - cm) / rho_c;
        }

        // Upstream boundary (i = 0): constant pressure reservoir
        pNew[0] = inletPressure;
        double f0 = frictionSlope(v[1], diameter, roughness, density, viscosity);
        double cm0 = p[1] - rho_c * v[1] + rho_c * f0 * dx;
        vNew[0] = (pNew[0] - cm0) / rho_c;

        // Downstream boundary (i = N): closing valve
        int N = spatialNodes;
        double fN = frictionSlope(v[N - 1], diameter, roughness, density, viscosity);
        double cpN = p[N - 1] + rho_c * v[N - 1] - rho_c * fN * dx;

        // Valve closure: linear ramp from initial velocity to 0
        double valveFraction = 1.0;
        if (t < closureTime)
            valveFraction = 1.0 - t / closureTime;
        else
            valveFraction = 0.0;

        // Valve characteristic: Δp = K_v * (τ * v)²
        // When fully open: v = initialVelocity, pressure ~ inlet
        // When closing: v approaches 0, causing pressure surge
        double valveVelocity = initialVelocity * valveFraction * valveFraction;
        vNew[N] = valveVelocity;

        // Pressure from C+ characteristic with valve velocity
        pNew[N] = cpN - rho_c * vNew[N];

        // Track maximum pressure
        for (int i = 0; i <= spatialNodes; ++i) {
            if (pNew[i] > maxPressure) {
                maxPressure = pNew[i];
                maxPressureTime = t;
            }
        }

        // Adaptive CFL: recompute dt every 10 steps based on current velocities
        if (step % 10 == 0 && timeStepSeconds <= 0.0) {
            double newDt = computeAdaptiveDt(c, dx, vNew);
            dt = std::max(newDt, 1.0e-7);
            dt = std::min(dt, dx / c);
        }

        // Record state every 20 steps to keep history manageable
        if (step % 20 == 0 || step == totalSteps - 1) {
            TransientState state;
            state.time = t;
            state.pressures = pNew;
            state.velocities = vNew;
            result.history.push_back(state);
        }

        // Swap buffers
        p.swap(pNew);
        v.swap(vNew);
    }

    result.maxPressure = maxPressure;
    result.maxPressureTime = maxPressureTime;
    result.spatialNodes = spatialNodes;
    result.timeSteps = totalSteps;

    double maxPressureMPa = maxPressure / 1.0e6;
    double joukowskyEstimate = density * c * initialVelocity;
    double joukowskyMPa = joukowskyEstimate / 1.0e6;

    result.message = QStringLiteral(
        "Water hammer simulation complete:\n"
        "  Path: %1 blocks, L=%2 m, d=%3 mm, c=%4 m/s\n"
        "  Initial velocity: %5 m/s, Closure time: %6 s\n"
        "  Grid: N=%7, dt=%8 ms, steps=%9\n"
        "  Max pressure: %10 MPa at t=%11 ms\n"
        "  Joukowsky estimate: %12 MPa (Δp=ρcΔv)")
        .arg(pathBlocks.size())
        .arg(totalLength, 0, 'f', 2)
        .arg(diameter * 1000.0, 0, 'f', 1)
        .arg(c, 0, 'f', 1)
        .arg(initialVelocity, 0, 'f', 2)
        .arg(closureTime, 0, 'f', 3)
        .arg(spatialNodes)
        .arg(dt * 1000.0, 0, 'f', 3)
        .arg(totalSteps)
        .arg(maxPressureMPa, 0, 'f', 3)
        .arg(maxPressureTime * 1000.0, 0, 'f', 1)
        .arg(joukowskyMPa, 0, 'f', 3);

    return result;
}
