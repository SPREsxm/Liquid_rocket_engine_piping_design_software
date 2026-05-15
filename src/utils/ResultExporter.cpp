#include "ResultExporter.h"
#include "NetworkSolver.h"

#include <QFile>
#include <QTextStream>

bool ResultExporter::exportNodesToCSV(const QList<NodeState>& nodes,
                                       const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    // UTF-8 BOM for Excel compatibility
    out << "\xEF\xBB\xBF";
    out << "Label,Type,Pressure (Pa),Pressure (MPa),Inlet Flow (kg/s),Outlet Flow (kg/s)\r\n";

    for (const auto& n : nodes) {
        out << '"' << n.blockLabel << '"' << ',';
        out << ','; // type placeholder
        out << n.pressure << ',';
        out << (n.pressure / 1.0e6) << ',';
        out << n.inletFlow << ',';
        out << n.outletFlow << "\r\n";
    }

    return true;
}

bool ResultExporter::exportEdgesToCSV(const QList<EdgeState>& edges,
                                       const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "\xEF\xBB\xBF";
    out << "Source,Target,Flow (kg/s),Pressure Drop (Pa),Resistance\r\n";

    for (const auto& e : edges) {
        out << '"' << e.sourceUuid.toString(QUuid::WithoutBraces) << '"' << ',';
        out << '"' << e.destUuid.toString(QUuid::WithoutBraces) << '"' << ',';
        out << e.massFlowRate << ',';
        out << e.pressureDrop << ',';
        out << e.resistance << "\r\n";
    }

    return true;
}
