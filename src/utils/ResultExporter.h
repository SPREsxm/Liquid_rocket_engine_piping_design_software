#pragma once

#include <QList>
#include <QString>

struct NodeState;
struct EdgeState;

class ResultExporter {
public:
    static bool exportNodesToCSV(const QList<NodeState>& nodes,
                                 const QString& filePath);
    static bool exportEdgesToCSV(const QList<EdgeState>& edges,
                                 const QString& filePath);
};
