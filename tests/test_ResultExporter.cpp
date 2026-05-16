#include <catch2/catch_all.hpp>
#include "utils/ResultExporter.h"
#include "utils/NetworkSolver.h"

#include <QFile>
#include <QUuid>
#include <QTemporaryDir>

namespace {
    QList<NodeState> makeTestNodes() {
        QList<NodeState> nodes;
        NodeState n1;
        n1.blockLabel = "Tank1";
        n1.blockTypeId = "tank.storage";
        n1.pressure = 1.0e6;
        n1.inletFlow = 0.0;
        n1.outletFlow = 10.0;
        nodes.append(n1);

        NodeState n2;
        n2.blockLabel = "Pipe1";
        n2.blockTypeId = "pipe.straight";
        n2.pressure = 9.5e5;
        n2.inletFlow = 10.0;
        n2.outletFlow = 10.0;
        nodes.append(n2);

        NodeState n3;
        n3.blockLabel = "Valve1";
        n3.blockTypeId = "valve.ball";
        n3.pressure = 9.0e5;
        n3.inletFlow = 10.0;
        n3.outletFlow = 10.0;
        nodes.append(n3);

        return nodes;
    }

    QList<EdgeState> makeTestEdges() {
        QList<EdgeState> edges;
        EdgeState e1;
        e1.sourceUuid = QUuid::createUuid();
        e1.destUuid = QUuid::createUuid();
        e1.massFlowRate = 10.0;
        e1.pressureDrop = 50000.0;
        e1.resistance = 500.0;
        edges.append(e1);

        EdgeState e2;
        e2.sourceUuid = QUuid::createUuid();
        e2.destUuid = QUuid::createUuid();
        e2.massFlowRate = 10.0;
        e2.pressureDrop = 50000.0;
        e2.resistance = 500.0;
        edges.append(e2);

        return edges;
    }
}

TEST_CASE("Export nodes to CSV creates file", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/nodes.csv";
    bool ok = ResultExporter::exportNodesToCSV(makeTestNodes(), path);
    REQUIRE(ok == true);
    REQUIRE(QFile::exists(path));
}

TEST_CASE("Export nodes to CSV has correct header", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/nodes.csv";
    ResultExporter::exportNodesToCSV(makeTestNodes(), path);

    QFile f(path);
    REQUIRE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray content = f.readAll();
    REQUIRE(content.contains("Label,Type,Pressure (Pa),Pressure (MPa),Inlet Flow (kg/s),Outlet Flow (kg/s)"));
}

TEST_CASE("Export nodes to CSV data rows match input", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/nodes.csv";
    auto nodes = makeTestNodes();
    ResultExporter::exportNodesToCSV(nodes, path);

    QFile f(path);
    REQUIRE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray content = f.readAll();
    REQUIRE(content.contains("Tank1"));
    REQUIRE(content.contains("Pipe1"));
    REQUIRE(content.contains("Valve1"));
}

TEST_CASE("Export nodes to CSV empty list creates file", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/nodes.csv";
    QList<NodeState> empty;
    bool ok = ResultExporter::exportNodesToCSV(empty, path);
    REQUIRE(ok == true);
    REQUIRE(QFile::exists(path));
}

TEST_CASE("Export edges to CSV creates file", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/edges.csv";
    bool ok = ResultExporter::exportEdgesToCSV(makeTestEdges(), path);
    REQUIRE(ok == true);
    REQUIRE(QFile::exists(path));
}

TEST_CASE("Export edges to CSV data rows match input", "[ResultExporter]") {
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.path() + "/edges.csv";
    auto edges = makeTestEdges();
    ResultExporter::exportEdgesToCSV(edges, path);

    QFile f(path);
    REQUIRE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray content = f.readAll();
    REQUIRE(content.contains("Source,Target,Flow (kg/s),Pressure Drop (Pa),Resistance"));
    REQUIRE((content.contains("10,") || content.contains("10\r")));
}

TEST_CASE("Export nodes to CSV invalid path returns false", "[ResultExporter]") {
    bool ok = ResultExporter::exportNodesToCSV(makeTestNodes(), "/invalid/path/:/nodes.csv");
    REQUIRE(ok == false);
}

TEST_CASE("Export edges to CSV invalid path returns false", "[ResultExporter]") {
    bool ok = ResultExporter::exportEdgesToCSV(makeTestEdges(), "/invalid/path/:/edges.csv");
    REQUIRE(ok == false);
}
