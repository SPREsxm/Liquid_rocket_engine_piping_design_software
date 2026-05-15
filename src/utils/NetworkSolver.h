#pragma once

#include "core/Types.h"

#include <QList>
#include <QString>
#include <QUuid>
#include <QHash>
#include <QVector>
#include <QSet>

class BlockScene;
class BlockItem;
class ConnectionItem;

struct NodeState {
    QUuid blockUuid;
    QString blockLabel;
    QString blockTypeId;
    double pressure = 0.0;   // Pa (gauge, relative to inlet reference)
    double inletFlow = 0.0;  // kg/s
    double outletFlow = 0.0; // kg/s
};

struct EdgeState {
    QUuid sourceUuid;
    QUuid destUuid;
    double massFlowRate = 0.0;   // kg/s
    double pressureDrop = 0.0;   // Pa
    double resistance = 0.0;     // Pa/(kg/s) — linearized flow resistance
};

struct NetworkSolution {
    QList<NodeState> nodes;
    QList<EdgeState> edges;
    double totalPressureDrop = 0.0;
    bool converged = false;
    QString message;
};

// BFS forward-propagation solver (existing)
NetworkSolution solveNetwork(BlockScene* scene,
                             double inletPressurePa = 1.0e6,
                             double inletMassFlowKgPerS = 10.0);

// Hardy-Cross iterative solver for looped networks
NetworkSolution solveNetworkHardyCross(BlockScene* scene,
                                       double inletPressurePa = 1.0e6,
                                       double inletMassFlowKgPerS = 10.0,
                                       int maxIterations = 200,
                                       double tolerance = 1e-6);

// Full node-edge matrix solver with direct linear system
NetworkSolution solveNetworkMatrix(BlockScene* scene,
                                   double inletPressurePa = 1.0e6,
                                   double inletMassFlowKgPerS = 10.0);

// Auto-select best solver based on network topology
NetworkSolution solveNetworkAuto(BlockScene* scene,
                                 double inletPressurePa = 1.0e6,
                                 double inletMassFlowKgPerS = 10.0);

// SolverSettings-aware overloads
NetworkSolution solveNetworkHardyCross(BlockScene* scene,
                                       const SolverSettings& settings,
                                       double inletPressurePa = 1.0e6,
                                       double inletMassFlowKgPerS = 10.0);

NetworkSolution solveNetworkMatrix(BlockScene* scene,
                                   const SolverSettings& settings,
                                   double inletPressurePa = 1.0e6,
                                   double inletMassFlowKgPerS = 10.0);

NetworkSolution solveNetworkAuto(BlockScene* scene,
                                 const SolverSettings& settings,
                                 double inletPressurePa = 1.0e6,
                                 double inletMassFlowKgPerS = 10.0);
