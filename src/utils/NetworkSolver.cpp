#include "NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"
#include "FluidDynamics.h"
#include "ResistanceCoefficients.h"
#include "SSTTurbulence.h"
#include "PropellantProperties.h"
#include "PipeScheduleDatabase.h"

#include <QQueue>
#include <QSet>
#include <QStack>
#include <cmath>
#include <algorithm>
#include <limits>

namespace {

// ─── resistance helpers ────────────────────────────────────────

constexpr double kDefaultDiameter = 0.0254;
constexpr double kDefaultRoughness = 0.000045;
constexpr double kDefaultDensity = 1141.0;
constexpr double kDefaultViscosity = 1.96e-4;

double getProp(BlockItem* b, const QString& id, double fallback) {
    QVariant v = b->propertyValue(id);
    if (v.isValid() && v.toDouble() != 0.0) return v.toDouble();
    // Fallback: resolve diameter from NPS/schedule via PipeScheduleDatabase
    if (id == "diameter") {
        double nps = b->propertyValue("nps").toDouble();
        QString sch = b->propertyValue("schedule").toString();
        if (nps > 0.0 && !sch.isEmpty()) {
            auto id_mm = PipeScheduleDatabase::instance().innerDiameter(nps, sch);
            if (id_mm.has_value()) return id_mm.value() / 1000.0; // mm→m
        }
    }
    return fallback;
}

double pipeResistance(double length, double diameter, double roughness,
                      double density, double viscosity, double massFlow,
                      bool useSST = true) {
    if (diameter <= 0.0 || density <= 0.0) return 1e12;
    const double A = M_PI * diameter * diameter / 4.0;
    const double velocity = massFlow / (density * A);
    const double reynolds = density * velocity * diameter / viscosity;

    double lambda;
    if (useSST && reynolds > 4000.0) {
        // SST k-ω turbulence model for turbulent pipe flow
        lambda = SSTTurbulence::effectiveFrictionFactorSST(
            reynolds, roughness, diameter, density, viscosity, velocity);
    } else {
        // Colebrook-White for laminar, transition, and SST fallback
        lambda = FluidDynamics::calculateColebrookWhiteFrictionFactor(
            reynolds, roughness, diameter);
    }
    const double d5 = std::pow(diameter, 5);
    return 8.0 * lambda * length / (M_PI * M_PI * d5 * density);
}

double localResistance(double zeta, double diameter, double density) {
    if (diameter <= 0.0 || density <= 0.0) return 1e12;
    const double A = M_PI * diameter * diameter / 4.0;
    return zeta / (2.0 * density * A * A);
}

// Map component type ID to Crane TP-410 FittingType
ResistanceCoefficients::FittingType fittingTypeForComponent(const QString& typeId) {
    using RT = ResistanceCoefficients::FittingType;
    if (typeId == "valve.gate")        return RT::GateValve_FullyOpen;
    if (typeId == "valve.globe")       return RT::GlobeValve_FullyOpen;
    if (typeId == "valve.ball")        return RT::BallValve_FullyOpen;
    if (typeId == "valve.solenoid")    return RT::GateValve_FullyOpen; // solenoid ≈ gate
    if (typeId == "valve.check")       return RT::SwingCheckValve;
    if (typeId == "valve.butterfly")   return RT::ButterflyValve;
    if (typeId == "valve.main")        return RT::GateValve_FullyOpen;
    if (typeId == "valve.secondary")   return RT::BallValve_FullyOpen;
    if (typeId == "valve.fill")        return RT::BallValve_FullyOpen;
    if (typeId == "valve.vent")        return RT::GlobeValve_FullyOpen;
    if (typeId == "valve.regulator")   return RT::GlobeValve_FullyOpen;
    if (typeId == "valve.selector")    return RT::BallValve_FullyOpen;
    if (typeId == "valve.flowRegulator") return RT::GlobeValve_FullyOpen;
    if (typeId == "valve.throttle")    return RT::ButterflyValve;
    if (typeId == "valve.purge")       return RT::GateValve_FullyOpen;
    if (typeId == "valve.relief")      return RT::GlobeValve_FullyOpen;
    if (typeId == "pipe.elbow")        return RT::Elbow90_Flanged;
    if (typeId == "pipe.elbow45")      return RT::Elbow45_Flanged;
    if (typeId == "pipe.tee")          return RT::Tee_Branch;
    if (typeId == "pipe.teestraight")  return RT::Tee_StraightThrough;
    // sentinel — caller checks
    return RT::PipeExit; // never a valid fitting match
}

// Returns true if this component type is a fitting with Crane Le/D data
bool isFittingType(const QString& typeId) {
    static const QSet<QString> fittings = {
        "valve.gate", "valve.globe", "valve.ball", "valve.solenoid",
        "valve.check", "valve.butterfly",
        "valve.main", "valve.secondary", "valve.fill", "valve.vent",
        "valve.regulator", "valve.selector", "valve.flowRegulator",
        "valve.throttle", "valve.purge", "valve.relief",
        "pipe.elbow", "pipe.elbow45", "pipe.tee", "pipe.teestraight",
        "pipe.cross", "pipe.filter", "pipe.cavitatingVenturi",
        "pipe.throttleOrifice", "pipe.compensator"
    };
    return fittings.contains(typeId);
}

double computeEdgeResistance(BlockItem* srcBlock, PortItem*,
                             double massFlow, double rho, double mu,
                             bool useSST = true,
                             double defaultRoughness = kDefaultRoughness) {
    const QString& typeId = srcBlock->typeId();
    const double d   = getProp(srcBlock, "diameter", kDefaultDiameter);
    const double eps = getProp(srcBlock, "roughness", defaultRoughness);

    // Explicit loss coefficient overrides everything
    QVariant zetaV = srcBlock->propertyValue("lossCoefficient");
    if (zetaV.isValid() && zetaV.toDouble() > 0.0)
        return localResistance(zetaV.toDouble(), d, rho);

    // Chamber components — use GasDynamics
    if (typeId == "chamber.nozzle") {
        double areaRatio = getProp(srcBlock, "areaRatio", 9.0);
        double gamma     = getProp(srcBlock, "gamma", 1.2);
        double chamberP  = getProp(srcBlock, "chamberPressure", 7.0e6);
        // Compute exit Mach from area ratio (supersonic branch)
        double exitMach = FluidDynamics::GasDynamics::machFromAreaRatio(areaRatio, gamma, false);
        // Pressure ratio p_exit / p_chamber
        double pr = FluidDynamics::GasDynamics::pressureRatio(
            FluidDynamics::GasDynamics::speedCoefficient(exitMach, gamma), gamma);
        double exitP = chamberP * pr;
        // Effective resistance: Δp / ṁ → use choked mass flow as reference
        double throatD = getProp(srcBlock, "throatDiameter", 0.05);
        double throatA = M_PI * throatD * throatD / 4.0;
        double molarMass = getProp(srcBlock, "molarMass", 22.3e-3);
        double chamberT  = getProp(srcBlock, "chamberTemp", 3500.0);
        double chokedMdot = FluidDynamics::GasDynamics::chokedMassFlow(
            chamberP, chamberT, throatA, gamma, molarMass);
        if (chokedMdot > 0.0) {
            double dp = chamberP - exitP;
            return dp / (chokedMdot * chokedMdot); // linearized resistance
        }
    }

    if (typeId == "chamber.injector") {
        double injArea = getProp(srcBlock, "injectionArea", 0.01);
        double injVel  = getProp(srcBlock, "injectionVelocity", 30.0);
        // Simple orifice model: Δp = 0.5 * ρ * v² / Cd², Cd ≈ 0.7
        const double Cd = 0.7;
        double dp = 0.5 * rho * injVel * injVel / (Cd * Cd);
        double mdot = rho * injArea * injVel;
        if (mdot > 0.0)
            return dp / (mdot * mdot);
    }

    // Crane Le/D method for fittings with known geometry
    if (isFittingType(typeId)) {
        auto fitting = fittingTypeForComponent(typeId);
        double led = ResistanceCoefficients::leOverD(fitting, d);
        if (led > 0.0) {
            double zeta = ResistanceCoefficients::zetaFromLeD(led, d, eps);
            if (zeta > 0.0)
                return localResistance(zeta, d, rho);
        }
    }

    // Pipes and straight runs
    const double L = getProp(srcBlock, "length", 0.3);
    if (typeId.startsWith("pipe.") || L > 0.001)
        return pipeResistance(L, d, eps, rho, mu, massFlow, useSST);

    // Fallback for components without explicit resistance
    return localResistance(0.5, d, rho); // reasonable default minor loss
}

// Forward declaration
struct Graph;
double computeDownstreamResistance(const Graph& g, BlockItem* startBlock,
                                   double massFlow, double rho, double mu,
                                   const QSet<BlockItem*>& /*visited*/,
                                   double defaultRoughness = kDefaultRoughness);

// ─── graph building ────────────────────────────────────────────

struct GraphEdge {
    ConnectionItem* connection;
    PortItem* srcPort;
    PortItem* dstPort;
    BlockItem* srcBlock;
    BlockItem* dstBlock;
};

struct Graph {
    QList<BlockItem*> nodes;
    QList<GraphEdge> edges;
    QHash<BlockItem*, QList<int>> outEdges;
    QHash<BlockItem*, QList<int>> inEdges;
    double fluidDensity = kDefaultDensity;
    double fluidViscosity = kDefaultViscosity;
    double pipeRoughness = kDefaultRoughness;
    bool useSST = true;
};

double computeDownstreamResistance(const Graph& g, BlockItem* startBlock,
                                   double massFlow, double rho, double mu,
                                   const QSet<BlockItem*>& /*visited*/,
                                   double defaultRoughness) {
    double totalK = 0.0;
    BlockItem* current = startBlock;
    QSet<BlockItem*> localVisited;
    const int maxDepth = 20;
    for (int depth = 0; depth < maxDepth; ++depth) {
        const QList<int>& outIndices = g.outEdges.value(current);
        if (outIndices.isEmpty()) break;
        int ei = -1;
        for (int idx : outIndices) {
            auto* dst = g.edges[idx].dstBlock;
            if (!localVisited.contains(dst)) {
                ei = idx;
                break;
            }
        }
        if (ei < 0) break;
        const GraphEdge& ge = g.edges[ei];
        double K = computeEdgeResistance(ge.srcBlock, ge.srcPort, massFlow, rho, mu,
                                          g.useSST, defaultRoughness);
        if (K <= 0.0) K = 1e-12;
        totalK += K;
        localVisited.insert(current);
        current = ge.dstBlock;
        if (current == startBlock) break;
    }
    return (totalK > 0.0) ? totalK : 1e-12;
}

Graph buildGraph(BlockScene* scene, double density = -1.0, double viscosity = -1.0,
                 bool useSST = true, double roughness = -1.0) {
    Graph g;
    g.nodes = scene->allBlocks();
    g.useSST = useSST;

    // Global overrides from SolverSettings (if provided)
    if (density > 0.0) g.fluidDensity = density;
    if (viscosity > 0.0) g.fluidViscosity = viscosity;
    if (roughness > 0.0) g.pipeRoughness = roughness;

    // Extract fluid properties from blocks that define them (per-block overrides)
    for (auto* b : g.nodes) {
        double rho = getProp(b, "density", -1.0);
        double mu  = getProp(b, "viscosity", -1.0);
        if (rho > 0.0) g.fluidDensity = rho;
        if (mu > 0.0)  g.fluidViscosity = mu;
    }

    for (int i = 0; i < g.nodes.size(); ++i) {
        g.outEdges[g.nodes[i]] = {};
        g.inEdges[g.nodes[i]] = {};
    }

    const auto& conns = scene->allConnections();
    for (int ei = 0; ei < conns.size(); ++ei) {
        auto* c = conns[ei];
        auto* sp = c->sourcePort();
        auto* dp = c->destPort();
        if (!sp || !dp) continue;

        auto* sb = sp->parentBlock();
        auto* db = dp->parentBlock();
        if (!sb || !db || sb == db) continue;

        GraphEdge ge{c, sp, dp, sb, db};
        const int idx = g.edges.size();
        g.edges.append(ge);
        g.outEdges[sb].append(idx);
        g.inEdges[db].append(idx);
    }

    return g;
}

QList<BlockItem*> findInlets(const Graph& g) {
    QList<BlockItem*> inlets;
    for (auto* b : g.nodes) {
        if (g.inEdges.value(b).isEmpty()) {
            if (!g.outEdges.value(b).isEmpty())
                inlets.append(b);
        }
    }
    if (inlets.isEmpty()) {
        for (auto* b : g.nodes) {
            if (!g.outEdges.value(b).isEmpty()) {
                inlets.append(b);
                break;
            }
        }
    }
    return inlets;
}

bool isOutletBlock(BlockItem* b) {
    if (!b) return false;
    QVariant envP = b->propertyValue("outletEnvironmentPressure");
    return envP.isValid() && envP.toDouble() > 0.0;
}

double getInletPressure(BlockItem* b, double fallback) {
    QVariant v = b->propertyValue("currentInletPressure");
    if (v.isValid() && v.toDouble() > 0.0) return v.toDouble();
    return fallback;
}
QList<BlockItem*> findOutlets(const Graph& g) {
    QList<BlockItem*> outlets;
    for (auto* b : g.nodes) {
        if (isOutletBlock(b) && g.outEdges.value(b).isEmpty())
            outlets.append(b);
    }
    return outlets;
}

// Build NodeState list and compute total pressure drop
// Compute thrust results from a nozzle block's properties and mass flow
ThrustAnalysis::ThrustResult computeNozzleThrust(BlockItem* nozzleBlock, double massFlow)
{
    ThrustAnalysis::ThrustResult r{};
    if (!nozzleBlock || massFlow <= 0.0) return r;

    double chamberP  = getProp(nozzleBlock, "chamberPressure", 7.0e6);
    double throatD   = getProp(nozzleBlock, "throatDiameter", 0.05);
    double exitD     = getProp(nozzleBlock, "exitDiameter", 0.15);
    double areaRatio = getProp(nozzleBlock, "areaRatio", 9.0);
    double gamma     = getProp(nozzleBlock, "gamma", 1.2);

    double throatA = M_PI * throatD * throatD / 4.0;
    double exitA   = M_PI * exitD * exitD / 4.0;
    if (throatA <= 0.0 || exitA <= 0.0) return r;

    // Compute exit pressure from area ratio and chamber pressure (supersonic)
    double exitMach = FluidDynamics::GasDynamics::machFromAreaRatio(areaRatio, true, gamma);
    double pr = FluidDynamics::GasDynamics::pressureRatio(
        FluidDynamics::GasDynamics::speedCoefficient(exitMach, gamma), gamma);
    double exitP = chamberP * pr;

    ThrustAnalysis::ThrustInputs in;
    in.chamberPressure_Pa = chamberP;
    in.exitPressure_Pa    = exitP;
    in.ambientPressure_Pa = 101325.0;  // sea level
    in.exitArea_m2        = exitA;
    in.throatArea_m2      = throatA;
    in.massFlow_kgPerS    = massFlow;
    in.gamma              = gamma;

    r = ThrustAnalysis::calculateThrust(in);

    // Compute nozzle efficiency using ideal Cf from GasDynamics
    double Cf_ideal = FluidDynamics::GasDynamics::thrustCoefficient(
        chamberP, exitP, in.ambientPressure_Pa, exitA, throatA, gamma);
    r.thrustCoefficient = Cf_ideal; // use GasDynamics value (more precise)
    double Cf_actual = r.thrustCoefficient;
    r.thrust_N = Cf_actual * chamberP * throatA;
    r.momentumThrust_N = r.thrust_N - (exitP - in.ambientPressure_Pa) * exitA;
    r.pressureThrust_N = (exitP - in.ambientPressure_Pa) * exitA;
    double g0 = 9.80665;
    r.specificImpulse_s = r.thrust_N / (massFlow * g0);

    return r;
}

void finalizeSolution(NetworkSolution& sol,
                      const Graph& g,
                      const QHash<BlockItem*, double>& pressure,
                      const QHash<BlockItem*, double>& inflow,
                      const QHash<BlockItem*, double>& outflow,
                      double inletPressurePa)
{
    for (auto* b : g.nodes) {
        NodeState ns;
        ns.blockUuid = b->uuid();
        ns.blockLabel = b->customLabel().isEmpty() ? b->typeId() : b->customLabel();
        ns.blockTypeId = b->typeId();
        ns.pressure = pressure.value(b);
        ns.inletFlow = inflow.value(b);
        ns.outletFlow = outflow.value(b);
        sol.nodes.append(ns);
    }

    double pMin = inletPressurePa;
    for (auto* b : g.nodes) {
        if (pressure[b] < pMin && !g.outEdges.value(b).isEmpty())
            pMin = pressure[b];
    }
    sol.totalPressureDrop = inletPressurePa - pMin;
    sol.converged = true;

    // Compute thrust if network contains a nozzle
    for (auto* b : g.nodes) {
        if (b->typeId() == "chamber.nozzle" && inflow.value(b) > 0.0) {
            sol.thrustResult = computeNozzleThrust(b, inflow.value(b));
            sol.hasThrustResults = true;
            break;
        }
    }
}

// Cavitation / NPSH risk check.
// Computes approximate vapor pressure for the working fluid at room temperature
// and flags any node where local pressure drops below NPSH safety margin.
QString checkCavitationRisk(const NetworkSolution& sol, FluidType fType)
{
    // Representative temperature — room temp unless propellant is cryogenic
    double TK = 293.15; // 20°C default
    double vaporPressurePa = 0.0;
    QString fluidName;

    // Use Wagner equation coefficients for approximate vapor pressure
    namespace PP = PropellantProperties;
    switch (fType) {
    case FluidType::LOX:
        TK = 90.0; // LOX boiling point ~90 K
        vaporPressurePa = PP::wagnerVaporPressure(TK, PP::wagnerLOX());
        fluidName = QStringLiteral("LOX");
        break;
    case FluidType::LH2:
        TK = 20.3;
        vaporPressurePa = PP::wagnerVaporPressure(TK, PP::wagnerLH2());
        fluidName = QStringLiteral("LH2");
        break;
    case FluidType::CH4:
        TK = 111.6;
        vaporPressurePa = PP::wagnerVaporPressure(TK, PP::wagnerMethane());
        fluidName = QStringLiteral("Methane");
        break;
    case FluidType::RP1:
        vaporPressurePa = 1333.0; // ~10 Torr at room temp
        fluidName = QStringLiteral("RP-1");
        break;
    case FluidType::Water:
    default:
        vaporPressurePa = 2338.0; // water at 20°C
        fluidName = QStringLiteral("Water");
        break;
    }

    // NPSH margin: require local pressure > vapor pressure × safety factor
    constexpr double npshMargin = 1.5;
    const double minSafePressure = vaporPressurePa * npshMargin;

    QStringList warnings;
    for (const auto& node : sol.nodes) {
        if (node.pressure > 0.0 && node.pressure < minSafePressure) {
            warnings.append(QStringLiteral("  ⚠ %1: p=%2 kPa < NPSH margin %3 kPa (%4)")
                .arg(node.blockLabel)
                .arg(node.pressure / 1e3, 0, 'f', 1)
                .arg(minSafePressure / 1e3, 0, 'f', 1)
                .arg(fluidName));
        }
    }

    if (!warnings.isEmpty()) {
        return QStringLiteral("[Cavitation Risk] %1 nodes below %2 vapor pressure margin:\n%3")
            .arg(warnings.size()).arg(fluidName).arg(warnings.join('\n'));
    }
    return QString();
}

// ─── BFS forward-propagation solver ────────────────────────────

NetworkSolution solveGraph(const Graph& g,
                           double inletPressurePa,
                           double inletMassFlowKgPerS,
                           int /*maxSplits*/ = 3,
                           double relaxation = 1.0) {
    NetworkSolution sol;
    const QList<BlockItem*> inlets = findInlets(g);

    if (inlets.isEmpty() || g.edges.isEmpty()) {
        sol.message = QStringLiteral("No inlets found or no edges in network.");
        return sol;
    }

    QHash<BlockItem*, double> pressure;
    QHash<BlockItem*, double> inflow;
    QHash<BlockItem*, double> outflow;

    for (auto* b : g.nodes) {
        pressure[b] = 0.0;
        inflow[b] = 0.0;
        outflow[b] = 0.0;
    }

    QQueue<BlockItem*> queue;
    QSet<BlockItem*> visited;

    double remainingFlow = inletMassFlowKgPerS;
    for (auto* inlet : inlets) {
        pressure[inlet] = getInletPressure(inlet, inletPressurePa);
        outflow[inlet] = remainingFlow / inlets.size();
        visited.insert(inlet);
        queue.enqueue(inlet);
    }

    while (!queue.isEmpty()) {
        auto* block = queue.dequeue();
        double flowIntoBlock = inflow[block];
        double totalOutFlow = outflow[block];
        double drivingFlow = (flowIntoBlock > 0.0) ? flowIntoBlock : totalOutFlow;
        if (drivingFlow <= 0.0) drivingFlow = inletMassFlowKgPerS;

        const QList<int>& outEdgeIndices = g.outEdges.value(block);
        if (outEdgeIndices.isEmpty()) continue;

        const int nOut = outEdgeIndices.size();

        // Single-pass flow distribution: Q_i ∝ 1/√K_i  (correct for turbulent parallel branches)
        QList<double> flows(nOut, 0.0);
        {
            double sumInvSqrtK = 0.0;
            QList<double> branchK(nOut, 0.0);
            double initFlow = drivingFlow / std::max(nOut, 1);
            for (int i = 0; i < nOut; ++i) {
                const GraphEdge& ge = g.edges[outEdgeIndices[i]];
                double K0 = computeEdgeResistance(ge.srcBlock, ge.srcPort, initFlow,
                                                  g.fluidDensity, g.fluidViscosity,
                                                  g.useSST, g.pipeRoughness);
                double Kdown = computeDownstreamResistance(g, ge.dstBlock, initFlow,
                                                           g.fluidDensity, g.fluidViscosity,
                                                           visited, g.pipeRoughness);
                branchK[i] = K0 + Kdown;
                if (branchK[i] <= 0.0) branchK[i] = 1e-12;
                sumInvSqrtK += 1.0 / std::sqrt(branchK[i]);
            }
            if (sumInvSqrtK > 0.0) {
                for (int i = 0; i < nOut; ++i)
                    flows[i] = drivingFlow * (1.0 / std::sqrt(branchK[i])) / sumInvSqrtK;
            } else {
                for (int i = 0; i < nOut; ++i)
                    flows[i] = drivingFlow / nOut;
            }
        }

        for (int i = 0; i < nOut; ++i) {
            const GraphEdge& ge = g.edges[outEdgeIndices[i]];
            auto* dst = ge.dstBlock;
            double flowPerEdge = flows[i];
            double K = computeEdgeResistance(ge.srcBlock, ge.srcPort, flowPerEdge,
                                             g.fluidDensity, g.fluidViscosity,
                                             g.useSST, g.pipeRoughness);
            double dp = K * flowPerEdge * flowPerEdge;
            double pDst = pressure[block] - dp;

            if (!visited.contains(dst)) {
                pressure[dst] = pDst;
            } else {
                double oldP = pressure[dst];
                pressure[dst] = relaxation * std::max(oldP, pDst) + (1.0 - relaxation) * oldP;
            }
            inflow[dst] += flowPerEdge;

            EdgeState es;
            es.sourceUuid = ge.srcBlock->uuid();
            es.destUuid = ge.dstBlock->uuid();
            es.massFlowRate = flowPerEdge;
            es.pressureDrop = dp;
            es.resistance = K;
            sol.edges.append(es);

            if (!visited.contains(dst)) {
                visited.insert(dst);
                outflow[dst] = flowPerEdge;
                queue.enqueue(dst);
            }
        }
    }

    // Outlet boundary condition: clamp outlet node pressures
    {
        QList<BlockItem*> outlets = findOutlets(g);
        for (auto* outlet : outlets) {
            double envP = getProp(outlet, "outletEnvironmentPressure", 0.0);
            if (envP > 0.0)
                pressure[outlet] = envP;
        }
    }

    finalizeSolution(sol, g, pressure, inflow, outflow, inletPressurePa);
    sol.message = QStringLiteral("[BFS] Solved: %1 nodes, %2 edges, Δp_total = %3 Pa")
        .arg(sol.nodes.size()).arg(sol.edges.size())
        .arg(sol.totalPressureDrop, 0, 'f', 1);

    return sol;
}

// ─── Cycle detection (DFS) ─────────────────────────────────────

// Returns a list of loops; each loop is a list of edge indices forming a cycle.
// Uses DFS with backtracking to find elementary cycles.

using EdgeIdxList = QList<int>;

void dfsFindCycles(int startNode, int currentNode,
                   const QVector<QList<int>>& adjOut, // [nodeIdx] → outgoing edge indices
                   const QVector<int>& edgeDest,       // [edgeIdx] → destination node index
                   QSet<int>& visited,
                   QVector<int>& pathEdges,
                   QSet<int>& pathNodes,
                   QList<EdgeIdxList>& loops,
                   int maxLoops = 20) {
    if (loops.size() >= maxLoops) return;

    for (int ei : adjOut[currentNode]) {
        int dst = edgeDest[ei];

        if (dst == startNode && !pathEdges.isEmpty()) {
            // Found a cycle back to start
            EdgeIdxList loop = pathEdges;
            loop.append(ei);
            loops.append(loop);
            if (loops.size() >= maxLoops) return;
            continue;
        }

        if (pathNodes.contains(dst)) continue; // would create a chord, not elementary

        // Recurse
        pathNodes.insert(dst);
        pathEdges.append(ei);
        dfsFindCycles(startNode, dst, adjOut, edgeDest, visited, pathEdges, pathNodes, loops, maxLoops);
        pathEdges.removeLast();
        pathNodes.remove(dst);
    }
}

QList<EdgeIdxList> detectLoops(const Graph& g) {
    QList<EdgeIdxList> loops;
    if (g.nodes.size() < 2 || g.edges.size() < 2) return loops;

    // Build adjacency: node → outgoing edge indices
    // Build node index map
    QHash<BlockItem*, int> nodeIdx;
    for (int i = 0; i < g.nodes.size(); ++i)
        nodeIdx[g.nodes[i]] = i;

    QVector<QList<int>> adjOut(g.nodes.size());
    QVector<int> edgeDest(g.edges.size());

    for (int ei = 0; ei < g.edges.size(); ++ei) {
        const GraphEdge& ge = g.edges[ei];
        int srcIdx = nodeIdx.value(ge.srcBlock, -1);
        int dstIdx = nodeIdx.value(ge.dstBlock, -1);
        if (srcIdx < 0 || dstIdx < 0) continue;
        adjOut[srcIdx].append(ei);
        edgeDest[ei] = dstIdx;
    }

    // DFS from each node
    QSet<int> visited;
    for (int i = 0; i < g.nodes.size(); ++i) {
        if (loops.size() >= 20) break;
        QVector<int> pathEdges;
        QSet<int> pathNodes;
        pathNodes.insert(i);
        dfsFindCycles(i, i, adjOut, edgeDest, visited, pathEdges, pathNodes, loops, 20);
    }

    return loops;
}

// ─── Hardy-Cross solver ────────────────────────────────────────

NetworkSolution solveHardyCross(const Graph& g,
                                double inletPressurePa,
                                double inletMassFlowKgPerS,
                                int maxIterations,
                                double tolerance) {
    NetworkSolution sol;
    const QList<BlockItem*> inlets = findInlets(g);

    if (inlets.isEmpty() || g.edges.isEmpty()) {
        sol.message = QStringLiteral("[Hardy-Cross] No inlets or edges.");
        return sol;
    }

    // Detect loops
    QList<EdgeIdxList> loops = detectLoops(g);

    if (loops.isEmpty()) {
        // No loops — fall back to BFS solver
        sol = solveGraph(g, inletPressurePa, inletMassFlowKgPerS);
        sol.message = QStringLiteral("[Hardy-Cross] No loops detected, used BFS: %1 nodes, %2 edges")
            .arg(sol.nodes.size()).arg(sol.edges.size());
        return sol;
    }

    // Initial flow distribution: BFS forward propagation
    QVector<double> Q(g.edges.size(), 0.0);
    QVector<double> K(g.edges.size(), 1.0);
    QHash<BlockItem*, double> nodeInflow;

    // Use BFS to get initial estimate
    NetworkSolution bfsSol = solveGraph(g, inletPressurePa, inletMassFlowKgPerS);
    for (int ei = 0; ei < std::min(Q.size(), bfsSol.edges.size()); ++ei) {
        Q[ei] = bfsSol.edges[ei].massFlowRate;
        K[ei] = bfsSol.edges[ei].resistance;
    }

    // Hardy-Cross iteration
    double maxCorrection = 0.0;
    int iter = 0;
    for (iter = 0; iter < maxIterations; ++iter) {
        maxCorrection = 0.0;

        for (const auto& loop : loops) {
            if (loop.isEmpty()) continue;

            // Sum head losses around loop: Σ(K·Q|Q|)
            // Sum derivatives: Σ(2K|Q|)
            double sumHL = 0.0;
            double sumDeriv = 0.0;

            for (int ei : loop) {
                if (ei < 0 || ei >= g.edges.size()) continue;
                double flow = Q[ei];
                double k = K[ei];
                if (k > 1e10) continue; // skip blocked edges

                // Head loss: K·Q|Q| (sign convention: positive = pressure drop in edge direction)
                sumHL += k * flow * std::abs(flow);
                sumDeriv += 2.0 * k * std::abs(flow);
            }

            if (sumDeriv < 1e-15) continue;

            // Hardy-Cross correction: ΔQ = -Σ(K·Q|Q|) / Σ(2K|Q|)
            double dQ = -sumHL / sumDeriv;
            if (std::abs(dQ) > maxCorrection)
                maxCorrection = std::abs(dQ);

            // Apply correction to all edges in the loop
            for (int ei : loop) {
                if (ei < 0 || ei >= g.edges.size()) continue;
                Q[ei] += dQ;
            }
        }

        if (maxCorrection < tolerance)
            break;
    }

    // Build solution from converged flows
    // Track pressure from inlets
    QHash<BlockItem*, double> pressure;
    QHash<BlockItem*, double> totalInflow;
    QHash<BlockItem*, double> totalOutflow;

    for (auto* b : g.nodes) {
        pressure[b] = 0.0;
        totalInflow[b] = 0.0;
        totalOutflow[b] = 0.0;
    }
    for (auto* inlet : inlets)
        pressure[inlet] = getInletPressure(inlet, inletPressurePa);

    // Forward pressure computation (BFS order from inlets)
    QQueue<BlockItem*> queue;
    QSet<BlockItem*> visited;
    for (auto* inlet : inlets) {
        visited.insert(inlet);
        queue.enqueue(inlet);
    }

    while (!queue.isEmpty()) {
        auto* block = queue.dequeue();
        const auto& outEdges = g.outEdges.value(block);

        for (int ei : outEdges) {
            const GraphEdge& ge = g.edges[ei];
            auto* dst = ge.dstBlock;
            double flow = Q[ei];
            double dp = K[ei] * flow * std::abs(flow);
            double pDst = pressure[block] - dp;

            if (!visited.contains(dst) || pressure[dst] < pDst)
                pressure[dst] = pDst;

            totalOutflow[block] += flow;
            totalInflow[dst] += flow;

            EdgeState es;
            es.sourceUuid = ge.srcBlock->uuid();
            es.destUuid = ge.dstBlock->uuid();
            es.massFlowRate = flow;
            es.pressureDrop = dp;
            es.resistance = K[ei];
            sol.edges.append(es);

            if (!visited.contains(dst)) {
                visited.insert(dst);
                queue.enqueue(dst);
            }
        }
    }

    // Outlet boundary condition: clamp outlet node pressures
    {
        QList<BlockItem*> outlets = findOutlets(g);
        for (auto* outlet : outlets) {
            double envP = getProp(outlet, "outletEnvironmentPressure", 0.0);
            if (envP > 0.0)
                pressure[outlet] = envP;
        }
    }

    finalizeSolution(sol, g, pressure, totalInflow, totalOutflow, inletPressurePa);
    sol.converged = (maxCorrection < tolerance);

    sol.message = QStringLiteral("[Hardy-Cross] %1 loops, %2 iterations, %3 nodes, %4 edges, Δp_total = %5 Pa")
        .arg(loops.size()).arg(iter).arg(sol.nodes.size()).arg(sol.edges.size())
        .arg(sol.totalPressureDrop, 0, 'f', 1);

    return sol;
}

// ─── Full matrix solver ────────────────────────────────────────

// Builds full incidence matrix and solves with simple Gauss-Seidel iteration
// for nonlinear flow resistance.
NetworkSolution solveMatrix(const Graph& g,
                            double inletPressurePa,
                            double inletMassFlowKgPerS,
                            int maxIter = 500,
                            double tol = 1e-8) {
    NetworkSolution sol;
    const QList<BlockItem*> inlets = findInlets(g);

    if (inlets.isEmpty() || g.edges.isEmpty()) {
        sol.message = QStringLiteral("[Matrix] No inlets or edges.");
        return sol;
    }

    const int nNodes = g.nodes.size();
    const int nEdges = g.edges.size();
    if (nEdges == 0) {
        sol.message = QStringLiteral("[Matrix] No edges.");
        return sol;
    }

    // Node index map
    QHash<BlockItem*, int> nodeIdx;
    for (int i = 0; i < nNodes; ++i)
        nodeIdx[g.nodes[i]] = i;

    // Incidence matrix A: [nNodes × nEdges]
    // A[i][j] = +1 if edge j leaves node i
    // A[i][j] = -1 if edge j enters node i
    // A[i][j] =  0 otherwise
    QVector<QVector<int>> A(nNodes, QVector<int>(nEdges, 0));
    QVector<int> inletNodeIdx;
    for (auto* inlet : inlets)
        inletNodeIdx.append(nodeIdx.value(inlet, -1));

    for (int j = 0; j < nEdges; ++j) {
        int src = nodeIdx.value(g.edges[j].srcBlock, -1);
        int dst = nodeIdx.value(g.edges[j].dstBlock, -1);
        if (src >= 0 && src < nNodes) A[src][j] = +1;
        if (dst >= 0 && dst < nNodes) A[dst][j] = -1;
    }

    // Resistance and flow vectors
    QVector<double> Kvals(nEdges, 0.0);
    QVector<double> Qvals(nEdges, 0.0);

    // Initial flow estimate: equally distribute among inlet edges
    for (int i : inletNodeIdx) {
        int nOut = 0;
        for (int j = 0; j < nEdges; ++j)
            if (A[i][j] > 0) ++nOut;
        if (nOut == 0) continue;
        double flowPerEdge = inletMassFlowKgPerS / nOut;
        for (int j = 0; j < nEdges; ++j)
            if (A[i][j] > 0) Qvals[j] = flowPerEdge;
    }

    // Iterative Gauss-Seidel: nodal mass balance + pressure continuity
    QVector<double> nodePressure(nNodes, 0.0);
    for (int i : inletNodeIdx)
        nodePressure[i] = getInletPressure(g.nodes[i], inletPressurePa);

    for (int iter = 0; iter < maxIter; ++iter) {
        double maxError = 0.0;

        // Update resistances based on current flows
        for (int j = 0; j < nEdges; ++j) {
            Kvals[j] = computeEdgeResistance(g.edges[j].srcBlock,
                                              g.edges[j].srcPort,
                                              std::abs(Qvals[j]) + 0.001,
                                              g.fluidDensity, g.fluidViscosity,
                                              g.useSST, g.pipeRoughness);
        }

        // Propagate pressures forward from inlets
        QQueue<int> q;
        QSet<int> vis;
        for (int i : inletNodeIdx) {
            q.enqueue(i);
            vis.insert(i);
        }

        while (!q.isEmpty()) {
            int ni = q.dequeue();
            double pHere = nodePressure[ni];

            for (int j = 0; j < nEdges; ++j) {
                if (A[ni][j] > 0) { // edge leaves this node
                    int dst = -1;
                    for (int k = 0; k < nNodes; ++k) {
                        if (A[k][j] < 0) { dst = k; break; }
                    }
                    if (dst < 0) continue;

                    double flow = Qvals[j];
                    double dp = Kvals[j] * flow * std::abs(flow);
                    double pDst = pHere - dp;

                    double oldP = nodePressure[dst];
                    if (!vis.contains(dst)) {
                        nodePressure[dst] = pDst;
                    } else {
                        nodePressure[dst] = 0.5 * (oldP + pDst);
                    }
                    double err = std::abs(nodePressure[dst] - oldP);
                    if (err > maxError) maxError = err;

                    if (!vis.contains(dst)) {
                        vis.insert(dst);
                        q.enqueue(dst);
                    }
                }
            }
        }

        // Enforce mass balance at each node (adjust flows)
        for (int i = 0; i < nNodes; ++i) {
            if (inletNodeIdx.contains(i)) continue; // inlet flow is fixed

            double sumIn = 0.0, sumOut = 0.0;
            for (int j = 0; j < nEdges; ++j) {
                if (A[i][j] < 0) sumIn += std::abs(Qvals[j]);
                if (A[i][j] > 0) sumOut += std::abs(Qvals[j]);
            }

            if (sumIn > 1e-12 || sumOut > 1e-12) {
                double scale = (sumIn > 1e-12) ? sumIn / (sumOut + 1e-12) : 1.0;
                for (int j = 0; j < nEdges; ++j) {
                    if (A[i][j] > 0) {
                        Qvals[j] *= scale;
                    }
                }
            }
        }

        if (maxError < tol) break;
    }

    // Outlet boundary condition: clamp outlet node pressures
    for (int i = 0; i < nNodes; ++i) {
        BlockItem* b = g.nodes[i];
        double envP = getProp(b, "outletEnvironmentPressure", 0.0);
        if (envP > 0.0 && g.outEdges.value(b).isEmpty())
            nodePressure[i] = envP;
    }

    // Build solution
    for (int j = 0; j < nEdges; ++j) {
        const GraphEdge& ge = g.edges[j];
        EdgeState es;
        es.sourceUuid = ge.srcBlock->uuid();
        es.destUuid = ge.dstBlock->uuid();
        es.massFlowRate = Qvals[j];
        es.pressureDrop = Kvals[j] * Qvals[j] * std::abs(Qvals[j]);
        es.resistance = Kvals[j];
        sol.edges.append(es);
    }

    // Build node states — adapted for matrix solver's array-based storage
    for (int i = 0; i < nNodes; ++i) {
        NodeState ns;
        ns.blockUuid = g.nodes[i]->uuid();
        ns.blockLabel = g.nodes[i]->customLabel().isEmpty()
            ? g.nodes[i]->typeId() : g.nodes[i]->customLabel();
        ns.blockTypeId = g.nodes[i]->typeId();
        ns.pressure = nodePressure[i];
        ns.inletFlow = 0.0;  // matrix solver doesn't track per-node flow
        ns.outletFlow = 0.0;
        sol.nodes.append(ns);
    }

    double pMin = inletPressurePa;
    for (double p : nodePressure) {
        if (p < pMin && p > 0.0) pMin = p;
    }
    sol.totalPressureDrop = inletPressurePa - pMin;
    sol.converged = true;
    sol.message = QStringLiteral("[Matrix] Solved: %1 nodes × %2 edges, Δp_total = %3 Pa")
        .arg(nNodes).arg(nEdges).arg(sol.totalPressureDrop, 0, 'f', 1);

    return sol;
}

} // anonymous namespace

// ─── public API ─────────────────────────────────────────────────

NetworkSolution solveNetwork(BlockScene* scene,
                             double inletPressurePa,
                             double inletMassFlowKgPerS) {
    NetworkSolution sol;
    if (!scene) {
        sol.message = QStringLiteral("Scene is null.");
        return sol;
    }
    if (scene->allBlocks().isEmpty()) {
        sol.message = QStringLiteral("Network is empty — no blocks to solve.");
        return sol;
    }
    if (scene->allConnections().isEmpty()) {
        sol.message = QStringLiteral("Network has no connections.");
        return sol;
    }
    Graph g = buildGraph(scene);
    return solveGraph(g, inletPressurePa, inletMassFlowKgPerS);
}

NetworkSolution solveNetworkHardyCross(BlockScene* scene,
                                       double inletPressurePa,
                                       double inletMassFlowKgPerS,
                                       int maxIterations,
                                       double tolerance) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network for Hardy-Cross solver.");
        return sol;
    }
    Graph g = buildGraph(scene);
    return solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS,
                           maxIterations, tolerance);
}

NetworkSolution solveNetworkMatrix(BlockScene* scene,
                                   double inletPressurePa,
                                   double inletMassFlowKgPerS) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network for matrix solver.");
        return sol;
    }
    Graph g = buildGraph(scene);
    return solveMatrix(g, inletPressurePa, inletMassFlowKgPerS);
}

NetworkSolution solveNetworkAuto(BlockScene* scene,
                                 double inletPressurePa,
                                 double inletMassFlowKgPerS) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network.");
        return sol;
    }
    Graph g = buildGraph(scene);
    QList<EdgeIdxList> loops = detectLoops(g);

    if (loops.isEmpty()) {
        return solveGraph(g, inletPressurePa, inletMassFlowKgPerS);
    } else {
        return solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS, 200, 1e-6);
    }
}

// ─── SolverSettings-aware overloads ──────────────────────────────

NetworkSolution solveNetworkHardyCross(BlockScene* scene,
                                       const SolverSettings& settings,
                                       double inletPressurePa,
                                       double inletMassFlowKgPerS) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network.");
        return sol;
    }
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity,
                         settings.useSSTTurbulence, settings.pipeRoughness);
    auto sol = solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS,
                               settings.hardyCrossMaxIter, settings.hardyCrossTolerance);
    // Cavitation/NPSH risk check
    if (sol.converged) {
        QString cavWarn = checkCavitationRisk(sol, settings.fluidType);
        if (!cavWarn.isEmpty())
            sol.message += QStringLiteral("\n") + cavWarn;
    }
    return sol;
}

NetworkSolution solveNetworkMatrix(BlockScene* scene,
                                   const SolverSettings& settings,
                                   double inletPressurePa,
                                   double inletMassFlowKgPerS) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network for matrix solver.");
        return sol;
    }
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity,
                         settings.useSSTTurbulence, settings.pipeRoughness);
    auto sol = solveMatrix(g, inletPressurePa, inletMassFlowKgPerS,
                           settings.matrixSolverMaxIter, settings.matrixSolverTolerance);
    // Cavitation/NPSH risk check
    if (sol.converged) {
        QString cavWarn = checkCavitationRisk(sol, settings.fluidType);
        if (!cavWarn.isEmpty())
            sol.message += QStringLiteral("\n") + cavWarn;
    }
    return sol;
}

NetworkSolution solveNetworkAuto(BlockScene* scene,
                                 const SolverSettings& settings,
                                 double inletPressurePa,
                                 double inletMassFlowKgPerS) {
    if (!scene || scene->allBlocks().isEmpty() || scene->allConnections().isEmpty()) {
        NetworkSolution sol;
        sol.message = QStringLiteral("Invalid or empty network.");
        return sol;
    }
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity,
                         settings.useSSTTurbulence, settings.pipeRoughness);
    QList<EdgeIdxList> loops = detectLoops(g);

    NetworkSolution sol;
    if (loops.isEmpty()) {
        sol = solveGraph(g, inletPressurePa, inletMassFlowKgPerS,
                        settings.maxIterations, settings.relaxationFactor);
    } else {
        sol = solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS,
                             settings.hardyCrossMaxIter, settings.hardyCrossTolerance);
    }
    // Cavitation/NPSH risk check
    if (sol.converged) {
        QString cavWarn = checkCavitationRisk(sol, settings.fluidType);
        if (!cavWarn.isEmpty())
            sol.message += QStringLiteral("\n") + cavWarn;
    }
    return sol;
}

// ─── Path Profile ──────────────────────────────────────────────────

PathProfile computePathProfile(BlockScene* scene,
                               const NetworkSolution& solution,
                               const QUuid& startBlockUuid,
                               const QUuid& endBlockUuid)
{
    PathProfile profile;
    profile.totalLength = 0.0;

    if (!scene) return profile;

    // Build node lookup from solution
    QHash<QUuid, const NodeState*> nodeByUuid;
    for (const auto& ns : solution.nodes)
        nodeByUuid[ns.blockUuid] = &ns;

    // Build edge adjacency
    struct EdgeInfo { QUuid dstUuid; };
    QHash<QUuid, QVector<EdgeInfo>> adj;
    for (const auto& es : solution.edges)
        adj[es.sourceUuid].append({es.destUuid});

    // BFS from start to end
    QQueue<QUuid> queue;
    QHash<QUuid, QUuid> parent;
    QSet<QUuid> visited;
    queue.enqueue(startBlockUuid);
    visited.insert(startBlockUuid);

    while (!queue.isEmpty()) {
        QUuid cur = queue.dequeue();
        if (cur == endBlockUuid) break;
        for (const auto& ei : adj.value(cur)) {
            if (!visited.contains(ei.dstUuid)) {
                visited.insert(ei.dstUuid);
                parent[ei.dstUuid] = cur;
                queue.enqueue(ei.dstUuid);
            }
        }
    }

    if (!parent.contains(endBlockUuid) && startBlockUuid != endBlockUuid)
        return profile;

    // Reconstruct path
    QVector<QUuid> path;
    QUuid cur = endBlockUuid;
    while (true) {
        path.prepend(cur);
        if (cur == startBlockUuid) break;
        if (!parent.contains(cur)) break;
        cur = parent[cur];
    }

    // Build profile points
    double dist = 0.0;
    for (int i = 0; i < path.size(); ++i) {
        const NodeState* ns = nodeByUuid.value(path[i], nullptr);

        PathProfilePoint pt;
        pt.cumulativeDistance = dist;
        pt.pressure = ns ? ns->pressure : 0.0;
        pt.nodeLabel = ns ? ns->blockLabel : path[i].toString(QUuid::WithoutBraces).left(8);
        profile.points.append(pt);

        if (i + 1 < path.size()) {
            BlockItem* block = scene->blockByUuid(path[i]);
            if (block) {
                QVariant lenVal = block->propertyValue("length");
                dist += lenVal.isValid() ? lenVal.toDouble() : 0.5;
            } else {
                dist += 0.5;
            }
        }
    }

    profile.totalLength = dist;
    return profile;
}
