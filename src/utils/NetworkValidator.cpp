#include "NetworkValidator.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "core/Types.h"

#include <QHash>
#include <QSet>

// ─── helpers ───────────────────────────────────────────────────

namespace {

enum class DfsColor { White, Gray, Black };

bool dfsCycleDetect(BlockItem* node,
                    const QHash<BlockItem*, QList<BlockItem*>>& adj,
                    QHash<BlockItem*, DfsColor>& color,
                    QList<BlockItem*>& cyclePath)
{
    color[node] = DfsColor::Gray;
    cyclePath.append(node);

    for (auto* neighbor : adj.value(node)) {
        if (color.value(neighbor) == DfsColor::Gray) {
            // Back edge found — extract the cycle portion
            cyclePath.append(neighbor);
            return true;
        }
        if (color.value(neighbor) == DfsColor::White) {
            if (dfsCycleDetect(neighbor, adj, color, cyclePath))
                return true;
        }
    }

    color[node] = DfsColor::Black;
    cyclePath.removeLast();
    return false;
}

QString blockLabel(BlockItem* b) {
    if (!b) return QStringLiteral("(null)");
    const QString& lbl = b->customLabel();
    return lbl.isEmpty() ? b->typeId() : lbl;
}

bool isMandatoryConnection(const PortItem* p) {
    // Input ports that accept Fluid are mandatory;
    // Signal/mechanical inputs are optional (e.g. control lines)
    if (p->direction() != PortDirection::Input) return false;
    return p->dataType() == PortDataType::Fluid;
}

} // anonymous namespace

// ─── topology validation ───────────────────────────────────────

ValidationResult validateTopology(BlockScene* scene)
{
    ValidationResult result;

    if (!scene) {
        result.issues.append({ValidationIssue::Error,
                              QStringLiteral("Scene pointer is null."), {}});
        return result;
    }

    const QList<BlockItem*> blocks = scene->allBlocks();
    const QList<ConnectionItem*> connections = scene->allConnections();

    // 1 ─ orphan blocks (no connections at all)
    QSet<BlockItem*> connectedBlocks;
    for (auto* c : connections) {
        if (c->sourcePort())
            connectedBlocks.insert(c->sourcePort()->parentBlock());
        if (c->destPort())
            connectedBlocks.insert(c->destPort()->parentBlock());
    }

    for (auto* b : blocks) {
        if (!connectedBlocks.contains(b)) {
            result.issues.append({ValidationIssue::Warning,
                QStringLiteral("Orphan block: %1 has no connections.")
                    .arg(blockLabel(b)),
                b->uuid()});
        }
    }

    // 2 ─ unconnected mandatory ports
    for (auto* b : blocks) {
        for (auto* p : b->inputPorts()) {
            if (!p->isConnected() && isMandatoryConnection(p)) {
                result.issues.append({ValidationIssue::Warning,
                    QStringLiteral("Block %1: mandatory input port '%2' is unconnected.")
                        .arg(blockLabel(b), p->portId()),
                    b->uuid()});
            }
        }
    }

    // 3 ─ type mismatch on connected ports
    for (auto* c : connections) {
        auto* src = c->sourcePort();
        auto* dst = c->destPort();
        if (!src || !dst) continue;

        if (src->dataType() != dst->dataType()) {
            result.issues.append({ValidationIssue::Error,
                QStringLiteral("Type mismatch: %1:%2 (%3) → %4:%5 (%6)")
                    .arg(blockLabel(src->parentBlock()), src->portId(),
                         src->dataType() == PortDataType::Fluid ? "Fluid"
                         : src->dataType() == PortDataType::Mechanical ? "Mechanical" : "Signal",
                         blockLabel(dst->parentBlock()), dst->portId(),
                         dst->dataType() == PortDataType::Fluid ? "Fluid"
                         : dst->dataType() == PortDataType::Mechanical ? "Mechanical" : "Signal"),
                src->parentBlock()->uuid()});
        }
    }

    // 4 ─ directed cycle detection
    // Build adjacency: block A → block B when A.output connects to B.input
    QHash<BlockItem*, QList<BlockItem*>> adj;
    for (auto* b : blocks)
        adj[b] = {};

    for (auto* c : connections) {
        auto* src = c->sourcePort();
        auto* dst = c->destPort();
        if (!src || !dst) continue;

        auto* srcBlock = src->parentBlock();
        auto* dstBlock = dst->parentBlock();
        if (srcBlock && dstBlock && srcBlock != dstBlock)
            adj[srcBlock].append(dstBlock);
    }

    QHash<BlockItem*, DfsColor> color;
    for (auto* b : blocks)
        color[b] = DfsColor::White;

    for (auto* b : blocks) {
        if (color[b] == DfsColor::White) {
            QList<BlockItem*> path;
            if (dfsCycleDetect(b, adj, color, path)) {
                QStringList cycleNames;
                for (auto* n : path)
                    cycleNames << blockLabel(n);
                result.issues.append({ValidationIssue::Error,
                    QStringLiteral("Directed cycle detected: %1")
                        .arg(cycleNames.join(" → ")),
                    b->uuid()});
                break; // report first cycle only
            }
        }
    }

    return result;
}

// ─── flow continuity validation ────────────────────────────────

ValidationResult validateFlowContinuity(BlockScene* scene)
{
    ValidationResult result;

    if (!scene) {
        result.issues.append({ValidationIssue::Error,
                              QStringLiteral("Scene pointer is null."), {}});
        return result;
    }

    // For each block with Fluid ports, check mass conservation:
    // sum(massFlow_in) ≈ sum(massFlow_out)
    for (auto* b : scene->allBlocks()) {
        double totalIn = 0.0, totalOut = 0.0;

        for (auto* p : b->inputPorts()) {
            if (p->dataType() == PortDataType::Fluid && p->isConnected()) {
                QVariant v = b->propertyValue("massFlowRate");
                if (v.isValid())
                    totalIn += v.toDouble();
            }
        }
        for (auto* p : b->outputPorts()) {
            if (p->dataType() == PortDataType::Fluid && p->isConnected()) {
                QVariant v = b->propertyValue("massFlowRate");
                if (v.isValid())
                    totalOut += v.toDouble();
            }
        }

        if (totalIn > 0.0 || totalOut > 0.0) {
            double imbalance = totalIn - totalOut;
            if (std::abs(imbalance) > 1e-6 * std::max(totalIn, totalOut) + 1e-9) {
                result.issues.append({ValidationIssue::Warning,
                    QStringLiteral("Block %1: mass flow imbalance (in=%2, out=%3, delta=%4 kg/s)")
                        .arg(blockLabel(b))
                        .arg(totalIn, 0, 'g', 4)
                        .arg(totalOut, 0, 'g', 4)
                        .arg(imbalance, 0, 'g', 4),
                    b->uuid()});
            }
        }
    }

    return result;
}
