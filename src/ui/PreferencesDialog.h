#pragma once

#include <QDialog>

class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreferencesDialog(QWidget* parent = nullptr);

private:
    void loadSettings();
    void saveSettings();

    // Display
    QSpinBox* m_gridSize;
    QCheckBox* m_snapToGrid;
    QDoubleSpinBox* m_defaultZoom;
    QCheckBox* m_antialiasing;

    // Language
    QComboBox* m_languageCombo;

    // Solver
    QDoubleSpinBox* m_solverTolerance;
    QSpinBox* m_solverMaxIter;
    QDoubleSpinBox* m_solverRelaxation;
    QDoubleSpinBox* m_solverCourant;
    QDoubleSpinBox* m_solverTimeStep;
    QSpinBox* m_solverGridNodes;
};
