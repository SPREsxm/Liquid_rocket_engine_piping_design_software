#include "ReportGenerator.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>

QString ReportGenerator::generate(const NetworkSolution& solution, const QString& title)
{
    QString html;
    QTextStream out(&html);

    out << "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
        << "<meta charset=\"UTF-8\">\n"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        << "<title>" << esc(title) << "</title>\n"
        << buildStyles()
        << "</head>\n<body>\n";

    out << buildHeader(title);
    out << buildSummary(solution);
    out << buildNodeTable(solution);
    out << buildEdgeTable(solution);
    if (solution.hasThrustResults)
        out << buildThrustSection(solution);
    out << buildFooter();

    out << "</body>\n</html>\n";
    return html;
}

QString ReportGenerator::generate(const NetworkSolution& solution,
                                  const BomResult& bom, const QString& title)
{
    QString html;
    QTextStream out(&html);

    out << "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
        << "<meta charset=\"UTF-8\">\n"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        << "<title>" << esc(title) << "</title>\n"
        << buildStyles()
        << "</head>\n<body>\n";

    out << buildHeader(title);
    out << buildSummary(solution);
    if (!bom.lines.isEmpty())
        out << buildBomSection(bom);
    out << buildNodeTable(solution);
    out << buildEdgeTable(solution);
    if (solution.hasThrustResults)
        out << buildThrustSection(solution);
    out << buildFooter();

    out << "</body>\n</html>\n";
    return html;
}

bool ReportGenerator::saveToFile(const QString& filePath,
                                 const NetworkSolution& solution,
                                 const QString& title)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << generate(solution, title);
    return true;
}

bool ReportGenerator::saveToFile(const QString& filePath,
                                 const NetworkSolution& solution,
                                 const BomResult& bom, const QString& title)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << generate(solution, bom, title);
    return true;
}

// ─── CSS Styles ──────────────────────────────────────────────────

QString ReportGenerator::buildStyles()
{
    return QStringLiteral(
        "<style>\n"
        "  :root { --primary: #1a237e; --accent: #1565c0; --border: #cfd8dc; "
        "           --bg: #f5f7fa; --text: #263238; --ok: #2e7d32; --warn: #e65100; }\n"
        "  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "  body { font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; "
        "         background: var(--bg); color: var(--text); line-height: 1.6; padding: 20px 40px; }\n"
        "  .header { background: linear-gradient(135deg, var(--primary), var(--accent)); "
        "            color: #fff; padding: 28px 32px; border-radius: 8px; margin-bottom: 24px; }\n"
        "  .header h1 { font-size: 22px; font-weight: 600; }\n"
        "  .header .meta { font-size: 13px; opacity: 0.85; margin-top: 4px; }\n"
        "  .section { background: #fff; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,.1); "
        "             padding: 20px 24px; margin-bottom: 18px; }\n"
        "  .section h2 { font-size: 16px; color: var(--primary); border-bottom: 2px solid var(--accent); "
        "                padding-bottom: 6px; margin-bottom: 12px; }\n"
        "  table { width: 100%; border-collapse: collapse; font-size: 13px; }\n"
        "  th { background: var(--primary); color: #fff; padding: 8px 10px; text-align: left; "
        "       font-weight: 500; }\n"
        "  td { padding: 6px 10px; border-bottom: 1px solid var(--border); }\n"
        "  tr:nth-child(even) td { background: #fafbfc; }\n"
        "  .summary-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); "
        "                  gap: 12px; }\n"
        "  .summary-card { background: var(--bg); border-radius: 6px; padding: 12px 16px; }\n"
        "  .summary-card .label { font-size: 11px; color: #607d8b; text-transform: uppercase; }\n"
        "  .summary-card .value { font-size: 18px; font-weight: 600; color: var(--primary); }\n"
        "  .converged { color: var(--ok); } .not-converged { color: var(--warn); }\n"
        "  .footer { text-align: center; font-size: 11px; color: #90a4ae; margin-top: 24px; "
        "            padding: 12px 0; border-top: 1px solid var(--border); }\n"
        "  @media print { body { padding: 0; } .section { box-shadow: none; break-inside: avoid; } }\n"
        "</style>\n"
    );
}

// ─── Sections ────────────────────────────────────────────────────

QString ReportGenerator::buildHeader(const QString& title)
{
    QString h;
    QTextStream out(&h);
    out << "<div class=\"header\">\n"
        << "  <h1>" << esc(title) << "</h1>\n"
        << "  <div class=\"meta\">Liquid Rocket Engine Piping Design Software &mdash; "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "</div>\n"
        << "</div>\n";
    return h;
}

QString ReportGenerator::buildSummary(const NetworkSolution& sol)
{
    QString s;
    QTextStream out(&s);
    out << "<div class=\"section\">\n<h2>Solver Summary</h2>\n"
        << "<div class=\"summary-grid\">\n";

    auto card = [&](const QString& label, const QString& value, const QString& cls = {}) {
        out << "<div class=\"summary-card\"><div class=\"label\">" << esc(label)
            << "</div><div class=\"value " << cls << "\">" << value << "</div></div>\n";
    };

    card("Status", sol.converged ? "Converged" : "Not Converged",
         sol.converged ? "converged" : "not-converged");
    card("Total Pressure Drop", fmtPa(sol.totalPressureDrop));
    card("Min Pressure", fmtPa(sol.minPressure()));
    card("Max Pressure", fmtPa(sol.maxPressure()));
    card("Nodes", QString::number(sol.nodes.size()));
    card("Edges", QString::number(sol.edges.size()));

    if (!sol.message.isEmpty())
        card("Message", esc(sol.message));

    if (sol.hasThrustResults) {
        card("Thrust", QString::number(sol.thrustResult.thrust_N * 1e-3, 'f', 3) + " kN");
        card("Specific Impulse", QString::number(sol.thrustResult.specificImpulse_s, 'f', 1) + " s");
        card("Relative Error", QString::number(sol.thrustResult.relativeError * 100.0, 'f', 3) + " %");
    }

    // Boundary fluid type annotation
    card("Fluid Type", sol.nodes.isEmpty() ? "—" : esc(sol.nodes.first().blockLabel));

    out << "</div>\n</div>\n";
    return s;
}

QString ReportGenerator::buildNodeTable(const NetworkSolution& sol)
{
    QString s;
    QTextStream out(&s);
    out << "<div class=\"section\">\n<h2>Node Results</h2>\n"
        << "<table>\n<tr><th>#</th><th>Block</th><th>Type</th>"
        << "<th>Pressure (bar)</th><th>Inlet Flow (kg/s)</th><th>Outlet Flow (kg/s)</th></tr>\n";

    int idx = 1;
    for (const auto& n : sol.nodes) {
        out << "<tr>"
            << "<td>" << idx++ << "</td>"
            << "<td>" << esc(n.blockLabel) << "</td>"
            << "<td>" << esc(n.blockTypeId) << "</td>"
            << "<td>" << QString::number(n.pressure * 1e-5, 'f', 4) << "</td>"
            << "<td>" << QString::number(n.inletFlow, 'f', 4) << "</td>"
            << "<td>" << QString::number(n.outletFlow, 'f', 4) << "</td>"
            << "</tr>\n";
    }
    out << "</table>\n</div>\n";
    return s;
}

QString ReportGenerator::buildEdgeTable(const NetworkSolution& sol)
{
    QString s;
    QTextStream out(&s);
    out << "<div class=\"section\">\n<h2>Edge Results</h2>\n"
        << "<table>\n<tr><th>#</th><th>Source</th><th>Destination</th>"
        << "<th>Mass Flow (kg/s)</th><th>Pressure Drop (bar)</th><th>Resistance (Pa·s/kg)</th></tr>\n";

    int idx = 1;
    for (const auto& e : sol.edges) {
        // Resolve UUIDs to labels
        QString srcLabel = e.sourceUuid.toString().left(8);
        QString dstLabel = e.destUuid.toString().left(8);
        for (const auto& n : sol.nodes) {
            if (n.blockUuid == e.sourceUuid) srcLabel = n.blockLabel;
            if (n.blockUuid == e.destUuid)   dstLabel = n.blockLabel;
        }

        out << "<tr>"
            << "<td>" << idx++ << "</td>"
            << "<td>" << esc(srcLabel) << "</td>"
            << "<td>" << esc(dstLabel) << "</td>"
            << "<td>" << QString::number(e.massFlowRate, 'f', 4) << "</td>"
            << "<td>" << QString::number(e.pressureDrop * 1e-5, 'f', 4) << "</td>"
            << "<td>" << QString::number(e.resistance, 'e', 3) << "</td>"
            << "</tr>\n";
    }
    out << "</table>\n</div>\n";
    return s;
}

QString ReportGenerator::buildThrustSection(const NetworkSolution& sol)
{
    const auto& t = sol.thrustResult;
    QString s;
    QTextStream out(&s);
    out << "<div class=\"section\">\n<h2>Thrust Chamber Analysis</h2>\n"
        << "<div class=\"summary-grid\">\n";

    auto card = [&](const QString& label, const QString& value) {
        out << "<div class=\"summary-card\"><div class=\"label\">" << esc(label)
            << "</div><div class=\"value\">" << value << "</div></div>\n";
    };

    card("Thrust",       QString::number(t.thrust_N * 1e-3, 'f', 3) + " kN");
    card("Specific Impulse", QString::number(t.specificImpulse_s, 'f', 1) + " s");
    card("Thrust Coefficient", QString::number(t.thrustCoefficient, 'f', 4));
    card("Momentum Thrust",    QString::number(t.momentumThrust_N * 1e-3, 'f', 3) + " kN");
    card("Pressure Thrust",    QString::number(t.pressureThrust_N * 1e-3, 'f', 3) + " kN");
    card("Uncertainty (±)",    QString::number(t.thrustUncertainty_N, 'f', 2) + " N");
    card("Relative Error",     QString::number(t.relativeError * 100.0, 'f', 3) + " %");
    card("Within Spec (≤0.15%)", t.withinSpec ? "Yes" : "No");

    out << "</div>\n</div>\n";
    return s;
}

QString ReportGenerator::buildBomSection(const BomResult& bom)
{
    QString s;
    QTextStream out(&s);
    out << "<div class=\"section\">\n<h2>Bill of Materials</h2>\n"
        << "<table>\n<tr><th>Category</th><th>Component</th><th>Qty</th>"
        << "<th>Unit</th><th>Unit Wt (kg)</th><th>Total Wt (kg)</th>"
        << "<th>Specification</th><th>Remark</th></tr>\n";

    for (const auto& line : bom.lines) {
        out << "<tr>"
            << "<td>" << esc(line.category) << "</td>"
            << "<td>" << esc(line.displayName) << "</td>"
            << "<td>" << line.quantity << "</td>"
            << "<td>" << esc(line.unit) << "</td>"
            << "<td>" << QString::number(line.unitWeight, 'f', 2) << "</td>"
            << "<td>" << QString::number(line.totalWeight, 'f', 2) << "</td>"
            << "<td>" << esc(line.specification) << "</td>"
            << "<td>" << esc(line.remark) << "</td>"
            << "</tr>\n";
    }

    out << "</table>\n"
        << "<p style=\"margin-top:8px; font-weight:600;\">Total Weight: "
        << QString::number(bom.totalWeight, 'f', 1) << " kg</p>\n"
        << "</div>\n";
    return s;
}

QString ReportGenerator::buildFooter()
{
    QString f;
    QTextStream out(&f);
    out << "<div class=\"footer\">Generated by Liquid Rocket Engine Piping Design Software &mdash; "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        << "</div>\n";
    return f;
}

// ─── Helper functions ────────────────────────────────────────────

QString ReportGenerator::esc(const QString& s)
{
    return s.toHtmlEscaped();
}

QString ReportGenerator::fmtPa(double pa)
{
    if (std::abs(pa) >= 1e6)
        return QString::number(pa * 1e-6, 'f', 4) + " MPa";
    if (std::abs(pa) >= 1e3)
        return QString::number(pa * 1e-3, 'f', 2) + " kPa";
    return QString::number(pa, 'f', 1) + " Pa";
}

QString ReportGenerator::fmtKgPerS(double kgs)
{
    if (std::abs(kgs) >= 1.0)
        return QString::number(kgs, 'f', 3) + " kg/s";
    if (std::abs(kgs) >= 0.001)
        return QString::number(kgs * 1e3, 'f', 2) + " g/s";
    return QString::number(kgs, 'e', 2) + " kg/s";
}
