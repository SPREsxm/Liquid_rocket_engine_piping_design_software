#include <catch2/catch_all.hpp>
#include "utils/ReportGenerator.h"
#include "utils/NetworkSolver.h"
#include <QFile>
#include <QDir>

TEST_CASE("ReportGenerator generates valid HTML for empty solution") {
    NetworkSolution empty;
    empty.converged = false;
    empty.message = "No network data";

    QString html = ReportGenerator::generate(empty, "Test Report");
    REQUIRE(!html.isEmpty());
    REQUIRE(html.contains("<!DOCTYPE html>"));
    REQUIRE(html.contains("<html"));
    REQUIRE(html.contains("Test Report"));
    REQUIRE(html.contains("Not Converged"));
    REQUIRE(html.contains("</html>"));
}

TEST_CASE("ReportGenerator generates valid HTML for populated solution") {
    NetworkSolution sol;
    sol.converged = true;
    sol.totalPressureDrop = 500000.0;
    sol.message = "Solution converged in 42 iterations";

    NodeState n1;
    n1.blockUuid = QUuid::createUuid();
    n1.blockLabel = "Oxidizer Tank";
    n1.blockTypeId = "tank.storage";
    n1.pressure = 1.0e6;
    n1.inletFlow = 0.0;
    n1.outletFlow = 10.0;

    NodeState n2;
    n2.blockUuid = QUuid::createUuid();
    n2.blockLabel = "Straight Pipe";
    n2.blockTypeId = "pipe.straight";
    n2.pressure = 0.95e6;
    n2.inletFlow = 10.0;
    n2.outletFlow = 10.0;

    NodeState n3;
    n3.blockUuid = QUuid::createUuid();
    n3.blockLabel = "Nozzle";
    n3.blockTypeId = "chamber.nozzle";
    n3.pressure = 0.5e6;
    n3.inletFlow = 10.0;
    n3.outletFlow = 0.0;

    sol.nodes = {n1, n2, n3};

    EdgeState e1;
    e1.sourceUuid = n1.blockUuid;
    e1.destUuid = n2.blockUuid;
    e1.massFlowRate = 10.0;
    e1.pressureDrop = 50000.0;
    e1.resistance = 5000.0;
    sol.edges = {e1};

    QString html = ReportGenerator::generate(sol, "LRE Analysis");

    REQUIRE(html.contains("Converged"));
    REQUIRE(html.contains("Oxidizer Tank"));
    REQUIRE(html.contains("Straight Pipe"));
    REQUIRE(html.contains("Nozzle"));
    REQUIRE(html.contains("tank.storage"));
}

TEST_CASE("ReportGenerator includes thrust section when hasThrustResults") {
    NetworkSolution sol;
    sol.converged = true;
    sol.hasThrustResults = true;
    sol.thrustResult.thrust_N = 250000.0;
    sol.thrustResult.specificImpulse_s = 320.5;
    sol.thrustResult.thrustCoefficient = 1.75;
    sol.thrustResult.momentumThrust_N = 180000.0;
    sol.thrustResult.pressureThrust_N = 70000.0;
    sol.thrustResult.relativeError = 0.001;
    sol.thrustResult.thrustUncertainty_N = 250.0;
    sol.thrustResult.withinSpec = true;

    QString html = ReportGenerator::generate(sol, "Thrust Test");

    REQUIRE(html.contains("Thrust Chamber Analysis"));
    REQUIRE(html.contains("250"));
    REQUIRE(html.contains("320.5"));
    REQUIRE(html.contains("1.75"));
}

TEST_CASE("ReportGenerator omits thrust section when hasThrustResults=false") {
    NetworkSolution sol;
    sol.converged = true;
    sol.hasThrustResults = false;

    QString html = ReportGenerator::generate(sol, "No Thrust");
    REQUIRE_FALSE(html.contains("Thrust Chamber Analysis"));
}

TEST_CASE("ReportGenerator saveToFile writes file") {
    NetworkSolution sol;
    sol.converged = true;

    QString path = QDir::tempPath() + "/lrep_test_report.html";
    bool ok = ReportGenerator::saveToFile(path, sol, "File Test");
    REQUIRE(ok);

    QFile f(path);
    REQUIRE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = f.readAll();
    f.close();
    QFile::remove(path);

    REQUIRE(content.contains("<!DOCTYPE html>"));
    REQUIRE(content.contains("File Test"));
}
