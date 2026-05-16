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

private slots:
    void onExportCsv();
    void onExportReport();

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
};
