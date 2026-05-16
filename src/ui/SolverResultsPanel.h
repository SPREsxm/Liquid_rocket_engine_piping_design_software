#pragma once

#include <QDockWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>
#include <QSplitter>

#include "utils/NetworkSolver.h"
#include "utils/DesignRules.h"
#include "utils/BomGenerator.h"
#include "utils/SensitivitySolver.h"
#include "utils/TransientSolver.h"
#include "utils/PipeOptimizer.h"
#include "utils/ThermalSolver.h"

#include <QTimer>

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QSlider;
class BlockScene;
class PaintChartWidget;

class SolverResultsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SolverResultsPanel(QWidget* parent = nullptr);

    void setResults(const NetworkSolution& solution);
    void setDesignCheckResults(const DesignCheckResult& result);
    void setBomData(const BomResult& bom);
    void clearResults();
    void setTransientResults(const struct TransientResult& result);

    const NetworkSolution& lastSolution() const { return m_lastSolution; }

    // Sensitivity
    void setAnalysisContext(class BlockScene* scene,
                            const SolverSettings& settings,
                            double inletPressurePa,
                            double inletMassFlowKgPerS);
    void setSensitivityResults(const SensitivityResult& result);
    void setTornadoResults(const QVector<SensitivityResult::TornadoBar>& bars,
                           const QString& outputMetric);

    // Path profile
    struct PathProfilePoint {
        double cumulativeDistance;
        double pressure;
        QString nodeLabel;
    };
    void setPathProfile(const QVector<PathProfilePoint>& profile);

    // Optimization
    void setOptimizationResults(const OptimizationResult& result);

    // Thermal / Stress
    void setThermalStressResults(const ThermalStressResult& result);

private slots:
    void onExportCsv();
    void onExportReport();
    void onRunSweep();
    void onComputeTornado();
    void onTransientPlayPause();
    void onTransientFrame();

signals:
    void exportCsvRequested();

private:
    void setupUi();
    void populateThrustTab(const NetworkSolution& solution);

    QTabWidget* m_tabWidget = nullptr;
    QTableWidget* m_nodeTable = nullptr;
    QTableWidget* m_edgeTable = nullptr;
    QTextEdit* m_transientSummary = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_exportReportBtn = nullptr;

    // Design Checks tab
    QTableWidget* m_designCheckTable = nullptr;

    // Charts
    PaintChartWidget* m_pressureChart = nullptr;
    PaintChartWidget* m_transientChart = nullptr;

    // Thrust Chamber tab widgets
    QWidget* m_thrustTab = nullptr;
    QLabel* m_lblThrust = nullptr;
    QLabel* m_lblIsp = nullptr;
    QLabel* m_lblCf = nullptr;
    QLabel* m_lblMomentumThrust = nullptr;
    QLabel* m_lblPressureThrust = nullptr;
    QLabel* m_lblError = nullptr;
    QLabel* m_lblEfficiency = nullptr;

    NetworkSolution m_lastSolution;
    BomResult m_bomResult;
    bool m_hasBom = false;

    // Sensitivity analysis tab
    QWidget* m_sensitivityTab = nullptr;
    QComboBox* m_sweepParamCombo = nullptr;
    QDoubleSpinBox* m_sweepMinSpin = nullptr;
    QDoubleSpinBox* m_sweepMaxSpin = nullptr;
    QSpinBox* m_sweepStepsSpin = nullptr;
    QPushButton* m_runSweepBtn = nullptr;
    QComboBox* m_tornadoOutputCombo = nullptr;
    QPushButton* m_computeTornadoBtn = nullptr;
    PaintChartWidget* m_sensitivityChart = nullptr;
    BlockScene* m_sensitivityScene = nullptr;
    SolverSettings m_sensitivitySettings;
    double m_sensitivityInletPressure = 1.0e6;
    double m_sensitivityInletFlow = 10.0;

    // Path profile tab
    QWidget* m_pathProfileTab = nullptr;
    PaintChartWidget* m_pathProfileChart = nullptr;
    QTableWidget* m_pathProfileTable = nullptr;

    // Optimization tab
    QWidget* m_optimizationTab = nullptr;
    QTableWidget* m_optimizationTable = nullptr;
    QLabel* m_optimizationSummary = nullptr;

    // Thermal / Stress tab
    QWidget* m_thermalTab = nullptr;
    QTableWidget* m_thermalTable = nullptr;
    QLabel* m_thermalSummary = nullptr;
    PaintChartWidget* m_thermalChart = nullptr;

    // Transient animation
    std::vector<TransientState> m_transientHistory;
    int m_transientFrame = 0;
    QTimer* m_transientTimer = nullptr;
    QSlider* m_transientProgress = nullptr;
    QPushButton* m_transientPlayBtn = nullptr;
    PaintChartWidget* m_transientAnimChart = nullptr;
    bool m_transientPlaying = false;
};
