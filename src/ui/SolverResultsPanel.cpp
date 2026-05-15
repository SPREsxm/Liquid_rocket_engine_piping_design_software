#include "SolverResultsPanel.h"
#include "utils/NetworkSolver.h"
#include "utils/ResultExporter.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

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
    m_tabWidget->addTab(m_nodeTable, tr("Node Results"));

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

    // ── Transient summary ───────────────────────────
    m_transientSummary = new QTextEdit;
    m_transientSummary->setReadOnly(true);
    m_tabWidget->addTab(m_transientSummary, tr("Transient Results"));

    layout->addWidget(m_tabWidget);

    // ── Export button ─────────────────────────────
    m_exportBtn = new QPushButton(tr("Export CSV..."));
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &SolverResultsPanel::onExportCsv);
    layout->addWidget(m_exportBtn);

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

    m_lastSolution = solution;
    m_exportBtn->setEnabled(true);
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

void SolverResultsPanel::clearResults()
{
    m_nodeTable->setRowCount(0);
    m_edgeTable->setRowCount(0);
    m_transientSummary->clear();
    m_exportBtn->setEnabled(false);
}
