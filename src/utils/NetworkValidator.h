#pragma once

#include <QList>
#include <QString>
#include <QUuid>

class BlockScene;
class BlockItem;

struct ValidationIssue {
    enum Severity { Info, Warning, Error };
    Severity severity;
    QString message;
    QUuid relatedBlockUuid;
};

struct ValidationResult {
    QList<ValidationIssue> issues;

    bool hasErrors() const {
        for (const auto& i : issues)
            if (i.severity == ValidationIssue::Error) return true;
        return false;
    }
    bool hasWarnings() const {
        for (const auto& i : issues)
            if (i.severity == ValidationIssue::Warning) return true;
        return false;
    }
};

// Validates the topology of a piping diagram.
// Checks: orphan blocks, unconnected mandatory ports, directed cycles, type mismatches.

ValidationResult validateTopology(BlockScene* scene);

// Validates flow continuity: mass conservation at each node.
// Requires a solved network (flow rates per connection).

ValidationResult validateFlowContinuity(BlockScene* scene);
