#include "NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"
#include "FluidDynamics.h"
#include "ResistanceCoefficients.h"

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
    return fallback;
}

double pipeResistance(double length, double diameter, double roughness,
                      double density, double viscosity, double massFlow) {
    if (diameter <= 0.0 || density <= 0.0) return 1e12;
    const double A = M_PI * diameter * diameter / 4.0;
    const double velocity = massFlow / (density * A);
    const double reynolds = density * velocity * diameter / viscosity;
    const double lambda = FluidDynamics::calculateColebrookWhiteFrictionFactor(
        reynolds, roughness, diameter);
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
        "pipe.elbow", "pipe.elbow45", "pipe.tee", "pipe.teestraight"
    };
    return fittings.contains(typeId);
}

double computeEdgeResistance(BlockItem* srcBlock, PortItem*,
                             double massFlow, double rho, double mu) {
    const QString& typeId = srcBlock->typeId();
    const double d   = getProp(srcBlock, "diameter", kDefaultDiameter);
    const double eps = getProp(srcBlock, "roughness", kDefaultRoughness);

    // Explicit loss coefficient overrides everything
    QVariant zetaV = srcBlock->propertyValue("lossCoefficient");
    if (zetaV.isValid() && zetaV.toDouble() > 0.0)
        return localResistance(zetaV.toDouble(), d, rho);

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
        return pipeResistance(L, d, eps, rho, mu, massFlow);

    // Fallback for components without explicit resistance
    return localResistance(0.5, d, rho); // reasonable default minor loss
}

// Forward declaration
struct Graph;
double computeDownstreamResistance(const Graph& g, BlockItem* startBlock,
                                   double massFlow, double rho, double mu,
                                   const QSet<BlockItem*>& /*visited*/);

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
};

double computeDownstreamResistance(const Graph& g, BlockItem* startBlock,
                                   double massFlow, double rho, double mu,
                                   const QSet<BlockItem*>& /*visited*/) {
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
        double K = computeEdgeResistance(ge.srcBlock, ge.srcPort, massFlow, rho, mu);
        if (K <= 0.0) K = 1e-12;
        totalK += K;
        localVisited.insert(current);
        current = ge.dstBlock;
        if (current == startBlock) break;
    }
    return (totalK > 0.0) ? totalK : 1e-12;
}

Graph buildGraph(BlockScene* scene, double density = -1.0, double viscosity = -1.0) {
    Graph g;
    g.nodes = scene->allBlocks();

    // Global overrides from SolverSettings (if provided)
    if (density > 0.0) g.fluidDensity = density;
    if (viscosity > 0.0) g.fluidViscosity = viscosity;

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

// Build NodeState list and compute total pressure drop
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
}

// ─── BFS forward-propagation solver ────────────────────────────

NetworkSolution solveGraph(const Graph& g,
                           double inletPressurePa,
                           double inletMassFlowKgPerS) {
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
        pressure[inlet] = inletPressurePa;
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

        // Iterative impedance-based flow distribution (2-3 passes)
        // For each branch, compute total downstream resistance, then split by 1/sqrt(K_total)
        QList<double> flows(nOut, drivingFlow / nOut);
        QList<double> branchK(nOut, 0.0);
        for (int iter = 0; iter < 3; ++iter) {
            double sumInvSqrtK = 0.0;
            for (int i = 0; i < nOut; ++i) {
                const GraphEdge& ge = g.edges[outEdgeIndices[i]];
                double K0 = computeEdgeResistance(ge.srcBlock, ge.srcPort, flows[i],
                                                  g.fluidDensity, g.fluidViscosity);
                double Kdown = computeDownstreamResistance(g, ge.dstBlock, flows[i],
                                                           g.fluidDensity, g.fluidViscosity, visited);
                branchK[i] = K0 + Kdown;
                if (branchK[i] <= 0.0) branchK[i] = 1e-12;
                sumInvSqrtK += 1.0 / std::sqrt(branchK[i]);
            }
            if (sumInvSqrtK <= 0.0) break;
            for (int i = 0; i < nOut; ++i)
                flows[i] = drivingFlow * (1.0 / std::sqrt(branchK[i])) / sumInvSqrtK;
        }

        for (int i = 0; i < nOut; ++i) {
            const GraphEdge& ge = g.edges[outEdgeIndices[i]];
            auto* dst = ge.dstBlock;
            double flowPerEdge = flows[i];
            double K = computeEdgeResistance(ge.srcBlock, ge.srcPort, flowPerEdge,
                                             g.fluidDensity, g.fluidViscosity);
            double dp = K * flowPerEdge * flowPerEdge;
            double pDst = pressure[block] - dp;

            if (!visited.contains(dst) || pressure[dst] < pDst)
                pressure[dst] = pDst;
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
        pressure[inlet] = inletPressurePa;

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
        nodePressure[i] = inletPressurePa;

    for (int iter = 0; iter < maxIter; ++iter) {
        double maxError = 0.0;

        // Update resistances based on current flows
        for (int j = 0; j < nEdges; ++j) {
            Kvals[j] = computeEdgeResistance(g.edges[j].srcBlock,
                                              g.edges[j].srcPort,
                                              std::abs(Qvals[j]) + 0.001,
                                              g.fluidDensity, g.fluidViscosity);
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
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity);
    return solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS,
                           settings.hardyCrossMaxIter, settings.hardyCrossTolerance);
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
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity);
    return solveMatrix(g, inletPressurePa, inletMassFlowKgPerS,
                       settings.matrixSolverMaxIter, settings.matrixSolverTolerance);
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
    Graph g = buildGraph(scene, settings.fluidDensity, settings.fluidViscosity);
    QList<EdgeIdxList> loops = detectLoops(g);

    if (loops.isEmpty()) {
        return solveGraph(g, inletPressurePa, inletMassFlowKgPerS);
    } else {
        return solveHardyCross(g, inletPressurePa, inletMassFlowKgPerS,
                               settings.hardyCrossMaxIter, settings.hardyCrossTolerance);
    }
}
