#include "SolverResultsPanel.h"
#include "ui/graphics/PaintChartWidget.h"
#include "utils/NetworkSolver.h"
#include "utils/TransientSolver.h"
#include "utils/ResultExporter.h"
#include "utils/ReportGenerator.h"
#include "utils/SensitivitySolver.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
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

    // ── Sensitivity Analysis tab ───────────────────────
    m_sensitivityTab = new QWidget;
    auto* sensLayout = new QVBoxLayout(m_sensitivityTab);

    // Controls row
    auto* sensControls = new QHBoxLayout;
    sensControls->addWidget(new QLabel(tr("Parameter:")));
    m_sweepParamCombo = new QComboBox;
    m_sweepParamCombo->addItems({
        tr("Inlet Pressure"), tr("Inlet Mass Flow"),
        tr("Fluid Density"), tr("Fluid Viscosity"),
        tr("Pipe Roughness"), tr("Pipe Wall Thickness")
    });
    m_sweepParamCombo->setCurrentIndex(0);
    sensControls->addWidget(m_sweepParamCombo);

    sensControls->addWidget(new QLabel(tr("Min:")));
    m_sweepMinSpin = new QDoubleSpinBox;
    m_sweepMinSpin->setRange(0.0, 1e12);
    m_sweepMinSpin->setDecimals(2);
    m_sweepMinSpin->setValue(0.5e6);
    sensControls->addWidget(m_sweepMinSpin);

    sensControls->addWidget(new QLabel(tr("Max:")));
    m_sweepMaxSpin = new QDoubleSpinBox;
    m_sweepMaxSpin->setRange(0.0, 1e12);
    m_sweepMaxSpin->setDecimals(2);
    m_sweepMaxSpin->setValue(5.0e6);
    sensControls->addWidget(m_sweepMaxSpin);

    sensControls->addWidget(new QLabel(tr("Steps:")));
    m_sweepStepsSpin = new QSpinBox;
    m_sweepStepsSpin->setRange(3, 50);
    m_sweepStepsSpin->setValue(10);
    sensControls->addWidget(m_sweepStepsSpin);

    m_runSweepBtn = new QPushButton(tr("Run Sweep"));
    m_runSweepBtn->setEnabled(false);
    connect(m_runSweepBtn, &QPushButton::clicked, this, &SolverResultsPanel::onRunSweep);
    sensControls->addWidget(m_runSweepBtn);
    sensControls->addStretch();
    sensLayout->addLayout(sensControls);

    // Tornado controls
    auto* tornadoControls = new QHBoxLayout;
    tornadoControls->addWidget(new QLabel(tr("Output Metric:")));
    m_tornadoOutputCombo = new QComboBox;
    m_tornadoOutputCombo->addItems({
        tr("Total Pressure Drop"), tr("Thrust"),
        tr("Specific Impulse"), tr("Max Pressure")
    });
    tornadoControls->addWidget(m_tornadoOutputCombo);

    m_computeTornadoBtn = new QPushButton(tr("Compute Tornado"));
    m_computeTornadoBtn->setEnabled(false);
    connect(m_computeTornadoBtn, &QPushButton::clicked, this, &SolverResultsPanel::onComputeTornado);
    tornadoControls->addWidget(m_computeTornadoBtn);
    tornadoControls->addStretch();
    sensLayout->addLayout(tornadoControls);

    // Chart area
    m_sensitivityChart = new PaintChartWidget;
    m_sensitivityChart->setChartType(PaintChartWidget::LineChart);
    m_sensitivityChart->setXLabel(tr("Parameter Value"));
    m_sensitivityChart->setYLabel(tr("Output"));
    m_sensitivityChart->setTitle(tr("Parameter Sweep"));
    sensLayout->addWidget(m_sensitivityChart, 1);

    m_tabWidget->addTab(m_sensitivityTab, tr("Sensitivity"));

    // ── Path Profile tab ─────────────────────────────────
    m_pathProfileTab = new QWidget;
    auto* ppLayout = new QVBoxLayout(m_pathProfileTab);

    m_pathProfileChart = new PaintChartWidget;
    m_pathProfileChart->setChartType(PaintChartWidget::LineChart);
    m_pathProfileChart->setXLabel(tr("Cumulative Distance (m)"));
    m_pathProfileChart->setYLabel(tr("Pressure (Pa)"));
    m_pathProfileChart->setTitle(tr("Path Pressure Profile"));
    ppLayout->addWidget(m_pathProfileChart, 1);

    m_pathProfileTable = new QTableWidget(0, 3);
    m_pathProfileTable->setHorizontalHeaderLabels({
        tr("Node"), tr("Distance (m)"), tr("Pressure (Pa)")
    });
    m_pathProfileTable->horizontalHeader()->setStretchLastSection(true);
    m_pathProfileTable->setAlternatingRowColors(true);
    m_pathProfileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ppLayout->addWidget(m_pathProfileTable);

    m_tabWidget->addTab(m_pathProfileTab, tr("Path Profile"));

    // ── Transient animation (embedded in Transient tab) ──
    {
        auto* animWidget = new QWidget;
        auto* animLayout = new QVBoxLayout(animWidget);

        m_transientAnimChart = new PaintChartWidget;
        m_transientAnimChart->setChartType(PaintChartWidget::LineChart);
        m_transientAnimChart->setXLabel(tr("Node Index"));
        m_transientAnimChart->setYLabel(tr("Pressure (Pa)"));
        m_transientAnimChart->setTitle(tr("Water Hammer — Frame Replay"));
        animLayout->addWidget(m_transientAnimChart, 1);

        auto* animCtrls = new QHBoxLayout;
        m_transientPlayBtn = new QPushButton(tr("▶ Play"));
        m_transientPlayBtn->setEnabled(false);
        connect(m_transientPlayBtn, &QPushButton::clicked,
                this, &SolverResultsPanel::onTransientPlayPause);
        animCtrls->addWidget(m_transientPlayBtn);

        m_transientProgress = new QSlider(Qt::Horizontal);
        m_transientProgress->setEnabled(false);
        m_transientProgress->setTickPosition(QSlider::TicksBelow);
        connect(m_transientProgress, &QSlider::valueChanged, this, [this](int frame) {
            m_transientFrame = frame;
            onTransientFrame();
        });
        animCtrls->addWidget(m_transientProgress, 1);

        animLayout->addLayout(animCtrls);
        m_tabWidget->addTab(animWidget, tr("Transient Animation"));
    }

    // Timer for animation
    m_transientTimer = new QTimer(this);

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
    m_runSweepBtn->setEnabled(m_sensitivityScene != nullptr);
    m_computeTornadoBtn->setEnabled(m_sensitivityScene != nullptr);
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
    // Original summary text
    m_transientSummary->append(result.message);

    // Populate transient pressure history chart (original code)
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

    // Store history for animation replay
    m_transientHistory = result.history;
    m_transientFrame = 0;
    m_transientTimer->stop();
    m_transientPlaying = false;

    if (!result.history.empty()) {
        m_transientProgress->setEnabled(true);
        m_transientProgress->setMaximum(static_cast<int>(result.history.size()) - 1);
        m_transientProgress->setValue(0);
        m_transientPlayBtn->setEnabled(true);
        m_transientPlayBtn->setText(tr("▶ Play"));

        // Show first frame
        onTransientFrame();
    }
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
    m_runSweepBtn->setEnabled(false);
    m_computeTornadoBtn->setEnabled(false);
    m_sensitivityScene = nullptr;
    m_transientHistory.clear();
    m_transientFrame = 0;
    m_transientTimer->stop();
    m_transientPlaying = false;
    m_transientPlayBtn->setEnabled(false);
    m_transientProgress->setEnabled(false);
    m_transientProgress->setMaximum(0);
}

// ── Sensitivity Analysis ──────────────────────────────────────────

void SolverResultsPanel::setAnalysisContext(BlockScene* scene,
                                            const SolverSettings& settings,
                                            double inletPressurePa,
                                            double inletMassFlowKgPerS)
{
    m_sensitivityScene = scene;
    m_sensitivitySettings = settings;
    m_sensitivityInletPressure = inletPressurePa;
    m_sensitivityInletFlow = inletMassFlowKgPerS;
    m_runSweepBtn->setEnabled(scene != nullptr);
    m_computeTornadoBtn->setEnabled(scene != nullptr);
}

void SolverResultsPanel::onRunSweep()
{
    if (!m_sensitivityScene) return;

    // Map combo index to param key
    static const QStringList keys = {
        "inletPressurePa", "inletMassFlow",
        "fluidDensity", "fluidViscosity",
        "pipeRoughness", "pipeWallThickness"
    };
    QString key = keys.value(m_sweepParamCombo->currentIndex(), "inletPressurePa");

    SensitivityResult result = runParameterSweep(
        m_sensitivityScene, m_sensitivitySettings, key,
        m_sweepMinSpin->value(), m_sweepMaxSpin->value(),
        m_sweepStepsSpin->value(),
        m_sensitivityInletPressure, m_sensitivityInletFlow);

    setSensitivityResults(result);
}

void SolverResultsPanel::setSensitivityResults(const SensitivityResult& result)
{
    if (result.points.isEmpty()) return;

    auto dpSeries = result.totalPressureDropSeries();

    m_sensitivityChart->setChartType(PaintChartWidget::LineChart);
    m_sensitivityChart->setXLabel(result.sweptParamName + " (" + result.sweptParamUnit + ")");
    m_sensitivityChart->setYLabel(tr("Total Pressure Drop (Pa)"));
    m_sensitivityChart->setTitle(tr("Parameter Sweep: %1 → ΔP").arg(result.sweptParamName));

    // Build x-y pairs for the swept param vs pressure drop
    QVector<QPointF> data;
    data.reserve(result.points.size());
    for (const auto& pt : result.points)
        data.append(QPointF(pt.paramValue, pt.solution.totalPressureDrop));
    m_sensitivityChart->setData(data);

    // Also add thrust and Isp as multi-series if available
    if (result.points.first().solution.hasThrustResults) {
        QVector<QVector<QPointF>> multi;
        QStringList labels;
        multi.append(data);
        labels.append(tr("Total ΔP"));

        QVector<QPointF> thrustData;
        for (const auto& pt : result.points)
            thrustData.append(QPointF(pt.paramValue,
                pt.solution.hasThrustResults ? pt.solution.thrustResult.thrust_N / 1000.0 : 0.0));
        multi.append(thrustData);
        labels.append(tr("Thrust (kN)"));

        m_sensitivityChart->setMultiSeries(multi, labels);
    }

    m_tabWidget->setCurrentWidget(m_sensitivityTab);
}

void SolverResultsPanel::onComputeTornado()
{
    if (!m_sensitivityScene) return;

    static const QStringList keys = {
        "inletPressurePa", "inletMassFlow",
        "fluidDensity", "fluidViscosity",
        "pipeRoughness", "pipeWallThickness"
    };

    static const QStringList metricKeys = {
        "totalPressureDrop", "thrust", "isp", "maxPressure"
    };
    QString metric = metricKeys.value(m_tornadoOutputCombo->currentIndex(),
                                      "totalPressureDrop");

    auto bars = computeTornado(m_sensitivityScene, m_sensitivitySettings, keys, metric,
                               m_sensitivityInletPressure, m_sensitivityInletFlow);

    setTornadoResults(bars, m_tornadoOutputCombo->currentText());
}

void SolverResultsPanel::setTornadoResults(
    const QVector<SensitivityResult::TornadoBar>& bars,
    const QString& outputMetric)
{
    if (bars.isEmpty()) return;

    QStringList labels;
    QVector<double> negImpacts, posImpacts;
    labels.reserve(bars.size());
    negImpacts.reserve(bars.size());
    posImpacts.reserve(bars.size());

    for (const auto& bar : bars) {
        labels.append(bar.paramName);
        negImpacts.append(bar.negativeImpact);
        posImpacts.append(bar.positiveImpact);
    }

    m_sensitivityChart->setChartType(PaintChartWidget::TornadoChart);
    m_sensitivityChart->setTitle(tr("Tornado: %1 Sensitivity").arg(outputMetric));
    m_sensitivityChart->setTornadoData(labels, negImpacts, posImpacts);

    m_tabWidget->setCurrentWidget(m_sensitivityTab);
}

// ── Path Profile ────────────────────────────────────────────────────

void SolverResultsPanel::setPathProfile(const QVector<PathProfilePoint>& profile)
{
    m_pathProfileTable->setRowCount(profile.size());
    QVector<QPointF> chartData;
    chartData.reserve(profile.size());

    for (int i = 0; i < profile.size(); ++i) {
        const auto& p = profile[i];
        m_pathProfileTable->setItem(i, 0, new QTableWidgetItem(p.nodeLabel));
        m_pathProfileTable->setItem(i, 1, new QTableWidgetItem(
            QString::number(p.cumulativeDistance, 'f', 3)));
        m_pathProfileTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(p.pressure, 'e', 3)));
        chartData.append(QPointF(p.cumulativeDistance, p.pressure));
    }
    m_pathProfileTable->resizeColumnsToContents();

    m_pathProfileChart->setChartType(PaintChartWidget::LineChart);
    m_pathProfileChart->setData(chartData);
    m_pathProfileChart->setTitle(tr("Path Pressure Profile"));

    m_tabWidget->setCurrentWidget(m_pathProfileTab);
}

// ── Transient Animation ─────────────────────────────────────────────

void SolverResultsPanel::onTransientPlayPause()
{
    if (m_transientHistory.empty()) return;

    if (m_transientPlaying) {
        m_transientTimer->stop();
        m_transientPlaying = false;
        m_transientPlayBtn->setText(tr("▶ Play"));
    } else {
        connect(m_transientTimer, &QTimer::timeout,
                this, &SolverResultsPanel::onTransientFrame, Qt::UniqueConnection);
        m_transientTimer->start(200); // ~5 fps
        m_transientPlaying = true;
        m_transientPlayBtn->setText(tr("⏸ Pause"));
    }
}

void SolverResultsPanel::onTransientFrame()
{
    if (m_transientHistory.empty()) return;
    if (m_transientFrame < 0)
        m_transientFrame = 0;
    if (m_transientFrame >= static_cast<int>(m_transientHistory.size()))
        m_transientFrame = static_cast<int>(m_transientHistory.size()) - 1;

    const auto& state = m_transientHistory[m_transientFrame];

    QVector<QPointF> data;
    data.reserve(state.pressures.size());
    for (size_t i = 0; i < state.pressures.size(); ++i)
        data.append(QPointF(static_cast<double>(i), state.pressures[i]));

    m_transientAnimChart->setData(data);
    m_transientAnimChart->setTitle(
        tr("Water Hammer — t = %1 s").arg(state.time, 0, 'f', 4));

    m_transientProgress->blockSignals(true);
    m_transientProgress->setValue(m_transientFrame);
    m_transientProgress->blockSignals(false);

    // Advance frame; loop back if at end
    if (m_transientPlaying) {
        ++m_transientFrame;
        if (m_transientFrame >= static_cast<int>(m_transientHistory.size())) {
            m_transientFrame = 0;
            m_transientTimer->stop();
            m_transientPlaying = false;
            m_transientPlayBtn->setText(tr("▶ Play"));
        }
    }
}
