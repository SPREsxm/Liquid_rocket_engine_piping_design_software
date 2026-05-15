#pragma once

#include <QDockWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>

#include "utils/NetworkSolver.h"

class SolverResultsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SolverResultsPanel(QWidget* parent = nullptr);

    void setResults(const NetworkSolution& solution);
    void clearResults();

    const NetworkSolution& lastSolution() const { return m_lastSolution; }

private slots:
    void onExportCsv();

signals:
    void exportCsvRequested();

private:
    void setupUi();

    QTabWidget* m_tabWidget = nullptr;
    QTableWidget* m_nodeTable = nullptr;
    QTableWidget* m_edgeTable = nullptr;
    QTextEdit* m_transientSummary = nullptr;
    QPushButton* m_exportBtn = nullptr;

    NetworkSolution m_lastSolution;
};
