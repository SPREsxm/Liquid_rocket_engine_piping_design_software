#include "SolverResultsPanel.h"
#include "ui/graphics/PaintChartWidget.h"
#include "utils/NetworkSolver.h"
#include "utils/TransientSolver.h"
#include "utils/ResultExporter.h"
#include "utils/ReportGenerator.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>

SolverResultsPanel::SolverResultsPanel(QWidget* parent)
    : QDockWidget(tr("Solver Results"), parent)
{
    setObjectName("SolverResultsPanel");
    setupUi();
}

void SolverResultsPanel::setupUi()
{
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);

    m_tabWidget = new QTabWidget;

    // ── Node results table ──────────────────────────
    m_nodeTable = new QTableWidget(0, 6);
    m_nodeTable->setHorizontalHeaderLabels({
        tr("Label"), tr("Type"), tr("Pressure (Pa)"),
        tr("Pressure (MPa)"), tr("Inlet Flow (kg/s)"), tr("Outlet Flow (kg/s)")
    });
    m_nodeTable->horizontalHeader()->setStretchLastSection(true);
    m_nodeTable->setAlternatingRowColors(true);
    m_nodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_nodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Pressure bar chart below node table
    m_pressureChart = new PaintChartWidget;
    m_pressureChart->setChartType(PaintChartWidget::BarChart);
    m_pressureChart->setXLabel(tr("Node Index"));
    m_pressureChart->setYLabel(tr("Pressure (Pa)"));
    m_pressureChart->setTitle(tr("Pressure Distribution"));

    auto* nodeSplitter = new QSplitter(Qt::Vertical);
    nodeSplitter->addWidget(m_nodeTable);
    nodeSplitter->addWidget(m_pressureChart);
    nodeSplitter->setStretchFactor(0, 3);
    nodeSplitter->setStretchFactor(1, 2);
    m_tabWidget->addTab(nodeSplitter, tr("Node Results"));

    // ── Edge results table ──────────────────────────
    m_edgeTable = new QTableWidget(0, 5);
    m_edgeTable->setHorizontalHeaderLabels({
        tr("Source"), tr("Target"), tr("Flow (kg/s)"),
        tr("Pressure Drop (Pa)"), tr("Resistance")
    });
    m_edgeTable->horizontalHeader()->setStretchLastSection(true);
    m_edgeTable->setAlternatingRowColors(true);
    m_edgeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_edgeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabWidget->addTab(m_edgeTable, tr("Edge Results"));

    // ── Transient results ───────────────────────────
    m_transientSummary = new QTextEdit;
    m_transientSummary->setReadOnly(true);

    m_transientChart = new PaintChartWidget;
    m_transientChart->setChartType(PaintChartWidget::LineChart);
    m_transientChart->setXLabel(tr("Time (s)"));
    m_transientChart->setYLabel(tr("Pressure (Pa)"));
    m_transientChart->setTitle(tr("Water Hammer Pressure History"));

    auto* transientSplitter = new QSplitter(Qt::Vertical);
    transientSplitter->addWidget(m_transientSummary);
    transientSplitter->addWidget(m_transientChart);
    transientSplitter->setStretchFactor(0, 1);
    transientSplitter->setStretchFactor(1, 2);
    m_tabWidget->addTab(transientSplitter, tr("Transient Results"));

    // ── Thrust Chamber tab ──────────────────────────
    m_thrustTab = new QWidget;
    auto* thrustLayout = new QFormLayout(m_thrustTab);
    m_lblThrust = new QLabel(tr("—"));
    m_lblIsp = new QLabel(tr("—"));
    m_lblCf = new QLabel(tr("—"));
    m_lblMomentumThrust = new QLabel(tr("—"));
    m_lblPressureThrust = new QLabel(tr("—"));
    m_lblError = new QLabel(tr("—"));
    m_lblEfficiency = new QLabel(tr("—"));
    thrustLayout->addRow(tr("Thrust (kN):"), m_lblThrust);
    thrustLayout->addRow(tr("Specific Impulse (s):"), m_lblIsp);
    thrustLayout->addRow(tr("Thrust Coefficient C_F:"), m_lblCf);
    thrustLayout->addRow(tr("— Momentum Thrust (kN):"), m_lblMomentumThrust);
    thrustLayout->addRow(tr("— Pressure Thrust (kN):"), m_lblPressureThrust);
    thrustLayout->addRow(tr("Rel. Error:"), m_lblError);
    thrustLayout->addRow(tr("Nozzle Efficiency:"), m_lblEfficiency);
    m_tabWidget->addTab(m_thrustTab, tr("Thrust Chamber"));

    // ── Design Checks tab ────────────────────────────
    m_designCheckTable = new QTableWidget(0, 6);
    m_designCheckTable->setHorizontalHeaderLabels({
        tr("Rule"), tr("Component"), tr("Severity"),
        tr("Actual"), tr("Limit"), tr("Message")
    });
    m_designCheckTable->horizontalHeader()->setStretchLastSection(true);
    m_designCheckTable->setAlternatingRowColors(true);
    m_designCheckTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_designCheckTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabWidget->addTab(m_designCheckTable, tr("Design Checks"));

    layout->addWidget(m_tabWidget);

    // ── Export button ─────────────────────────────
    m_exportBtn = new QPushButton(tr("Export CSV..."));
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &SolverResultsPanel::onExportCsv);
    layout->addWidget(m_exportBtn);

    m_exportReportBtn = new QPushButton(tr("Export HTML Report..."));
    m_exportReportBtn->setEnabled(false);
    connect(m_exportReportBtn, &QPushButton::clicked, this, &SolverResultsPanel::onExportReport);
    layout->addWidget(m_exportReportBtn);

    setWidget(container);
}

void SolverResultsPanel::setResults(const NetworkSolution& solution)
{
    // ── Populate node table ────────────────────────
    m_nodeTable->setRowCount(solution.nodes.size());
    for (int i = 0; i < solution.nodes.size(); ++i) {
        const auto& n = solution.nodes[i];
        m_nodeTable->setItem(i, 0, new QTableWidgetItem(n.blockLabel));
        m_nodeTable->setItem(i, 1, new QTableWidgetItem()); // type filled by caller if needed
        m_nodeTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(n.pressure, 'e', 3)));
        m_nodeTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(n.pressure / 1.0e6, 'f', 4)));
        m_nodeTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(n.inletFlow, 'f', 6)));
        m_nodeTable->setItem(i, 5, new QTableWidgetItem(
            QString::number(n.outletFlow, 'f', 6)));
    }
    m_nodeTable->resizeColumnsToContents();

    // ── Populate pressure chart ─────────────────────
    {
        QVector<QPointF> chartData;
        for (const auto& n : solution.nodes)
            chartData.append(QPointF(static_cast<double>(chartData.size()), n.pressure));
        m_pressureChart->setData(chartData);
    }

    // ── Populate edge table ────────────────────────
    m_edgeTable->setRowCount(solution.edges.size());
    for (int i = 0; i < solution.edges.size(); ++i) {
        const auto& e = solution.edges[i];
        m_edgeTable->setItem(i, 0, new QTableWidgetItem(
            e.sourceUuid.toString(QUuid::WithoutBraces).left(8)));
        m_edgeTable->setItem(i, 1, new QTableWidgetItem(
            e.destUuid.toString(QUuid::WithoutBraces).left(8)));
        m_edgeTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(e.massFlowRate, 'f', 6)));
        m_edgeTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(e.pressureDrop, 'e', 3)));
        m_edgeTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(e.resistance, 'e', 3)));
    }
    m_edgeTable->resizeColumnsToContents();

    // ── Transient / summary info ───────────────────
    m_transientSummary->clear();
    m_transientSummary->append(tr("Converged: %1").arg(solution.converged ? tr("Yes") : tr("No")));
    m_transientSummary->append(tr("Total Pressure Drop: %1 Pa").arg(solution.totalPressureDrop, 0, 'e', 3));
    if (!solution.message.isEmpty())
        m_transientSummary->append(tr("Message: %1").arg(solution.message));

    // ── Populate thrust tab ─────────────────────────
    populateThrustTab(solution);

    m_lastSolution = solution;
    m_exportBtn->setEnabled(true);
    m_exportReportBtn->setEnabled(true);
}

void SolverResultsPanel::populateThrustTab(const NetworkSolution& solution)
{
    if (!solution.hasThrustResults) {
        m_lblThrust->setText(tr("No nozzle in network"));
        m_lblIsp->setText(tr("—"));
        m_lblCf->setText(tr("—"));
        m_lblMomentumThrust->setText(tr("—"));
        m_lblPressureThrust->setText(tr("—"));
        m_lblError->setText(tr("—"));
        m_lblEfficiency->setText(tr("—"));
        return;
    }

    const auto& r = solution.thrustResult;
    m_lblThrust->setText(QString::number(r.thrust_N / 1000.0, 'f', 3));
    m_lblIsp->setText(QString::number(r.specificImpulse_s, 'f', 1));
    m_lblCf->setText(QString::number(r.thrustCoefficient, 'f', 4));
    m_lblMomentumThrust->setText(QString::number(r.momentumThrust_N / 1000.0, 'f', 3));
    m_lblPressureThrust->setText(QString::number(r.pressureThrust_N / 1000.0, 'f', 3));
    m_lblError->setText(QStringLiteral("%1% %2")
        .arg(r.relativeError * 100.0, 0, 'f', 3)
        .arg(r.withinSpec ? tr("✓ Within Spec") : tr("✗ Exceeds Spec")));
    m_lblEfficiency->setText(QString::number(r.thrustCoefficient > 0.0 ? 100.0 : 0.0, 'f', 1) + "%");
}

void SolverResultsPanel::setTransientResults(const TransientResult& result)
{
    m_transientSummary->append(result.message);

    // Populate transient pressure history chart
    QVector<QVector<QPointF>> multiSeries;
    QStringList labels;
    if (!result.history.empty()) {
        QVector<QPointF> maxPressureSeries;
        for (const auto& state : result.history) {
            double maxP = 0.0;
            for (double p : state.pressures)
                if (p > maxP) maxP = p;
            maxPressureSeries.append(QPointF(state.time, maxP));
        }
        multiSeries.append(maxPressureSeries);
        labels.append(QStringLiteral("Max Pressure"));
    }
    m_transientChart->setMultiSeries(multiSeries, labels);
}

void SolverResultsPanel::onExportCsv()
{
    if (m_lastSolution.nodes.isEmpty() && m_lastSolution.edges.isEmpty())
        return;

    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Export CSV to..."), QString(),
        QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;

    bool ok = true;
    if (!m_lastSolution.nodes.isEmpty()) {
        ok = ResultExporter::exportNodesToCSV(m_lastSolution.nodes,
                                               dir + "/nodes.csv") && ok;
    }
    if (!m_lastSolution.edges.isEmpty()) {
        ok = ResultExporter::exportEdgesToCSV(m_lastSolution.edges,
                                               dir + "/edges.csv") && ok;
    }

    if (ok)
        QMessageBox::information(this, tr("CSV Export"),
                                 tr("Exported to:\n%1").arg(dir));
    else
        QMessageBox::warning(this, tr("CSV Export"),
                             tr("Failed to write one or more CSV files."));
}

void SolverResultsPanel::onExportReport()
{
    if (m_lastSolution.nodes.isEmpty() && m_lastSolution.edges.isEmpty())
        return;

    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export HTML Report"), "analysis_report.html",
        tr("HTML Files (*.html *.htm)"));
    if (filePath.isEmpty()) return;

    bool ok = m_hasBom
        ? ReportGenerator::saveToFile(filePath, m_lastSolution, m_bomResult,
                                      tr("LRE Piping Analysis Report"))
        : ReportGenerator::saveToFile(filePath, m_lastSolution,
                                      tr("LRE Piping Analysis Report"));
    if (ok)
        QMessageBox::information(this, tr("Report Export"),
                                 tr("Report saved to:\n%1").arg(filePath));
    else
        QMessageBox::warning(this, tr("Report Export"),
                             tr("Failed to write report file."));
}

void SolverResultsPanel::setDesignCheckResults(const DesignCheckResult& result)
{
    m_designCheckTable->setRowCount(result.items.size());
    for (int i = 0; i < result.items.size(); ++i) {
        const auto& item = result.items[i];
        m_designCheckTable->setItem(i, 0, new QTableWidgetItem(item.ruleName));
        m_designCheckTable->setItem(i, 1, new QTableWidgetItem(item.componentLabel));
        QString sevText;
        QColor sevColor;
        switch (item.severity) {
        case DesignCheckResult::Error:
            sevText = tr("ERROR");
            sevColor = QColor("#C62828"); // dark red
            break;
        case DesignCheckResult::Warning:
            sevText = tr("WARNING");
            sevColor = QColor("#E65100"); // orange
            break;
        default:
            sevText = tr("PASS");
            sevColor = QColor("#2E7D32"); // green
            break;
        }
        auto* sevItem = new QTableWidgetItem(sevText);
        sevItem->setForeground(sevColor);
        m_designCheckTable->setItem(i, 2, sevItem);
        m_designCheckTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(item.actualValue, 'g', 4) + " " + item.unit));
        m_designCheckTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(item.limitValue, 'g', 4) + " " + item.unit));
        m_designCheckTable->setItem(i, 5, new QTableWidgetItem(item.message));
    }
    m_designCheckTable->resizeColumnsToContents();

    if (!result.items.isEmpty())
        m_tabWidget->setCurrentWidget(m_designCheckTable);
}

void SolverResultsPanel::setBomData(const BomResult& bom)
{
    m_bomResult = bom;
    m_hasBom = !bom.lines.isEmpty();
}

void SolverResultsPanel::clearResults()
{
    m_nodeTable->setRowCount(0);
    m_edgeTable->setRowCount(0);
    m_designCheckTable->setRowCount(0);
    m_transientSummary->clear();
    m_hasBom = false;
    m_bomResult = BomResult{};
    m_exportBtn->setEnabled(false);
    m_exportReportBtn->setEnabled(false);
}
