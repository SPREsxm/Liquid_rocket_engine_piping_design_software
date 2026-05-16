#pragma once

#include "utils/NetworkSolver.h"
#include "utils/BomGenerator.h"
#include <QString>

// Generates a self-contained HTML engineering report from solver results.
// Pure static generation — no external dependencies, embedded CSS, UTF-8.

class ReportGenerator {
public:
    // Generate complete HTML report as a QString
    static QString generate(const NetworkSolution& solution,
                            const QString& title = "LRE Piping Analysis Report");

    // Generate HTML report with BOM section
    static QString generate(const NetworkSolution& solution,
                            const BomResult& bom,
                            const QString& title = "LRE Piping Analysis Report");

    // Write HTML report directly to a file. Returns true on success.
    static bool saveToFile(const QString& filePath,
                           const NetworkSolution& solution,
                           const QString& title = "LRE Piping Analysis Report");

    // Write HTML report with BOM section
    static bool saveToFile(const QString& filePath,
                           const NetworkSolution& solution,
                           const BomResult& bom,
                           const QString& title = "LRE Piping Analysis Report");

private:
    static QString buildStyles();
    static QString buildHeader(const QString& title);
    static QString buildSummary(const NetworkSolution& sol);
    static QString buildNodeTable(const NetworkSolution& sol);
    static QString buildEdgeTable(const NetworkSolution& sol);
    static QString buildThrustSection(const NetworkSolution& sol);
    static QString buildBomSection(const BomResult& bom);
    static QString buildFooter();

    // Helpers
    static QString esc(const QString& s);
    static QString fmtPa(double pa);
    static QString fmtKgPerS(double kgs);
};
