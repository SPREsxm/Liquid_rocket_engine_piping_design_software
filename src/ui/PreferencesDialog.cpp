#include "PreferencesDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(400, 550);

    auto* mainLayout = new QVBoxLayout(this);

    // Grid settings
    auto* gridGroup = new QGroupBox(tr("Grid"));
    auto* gridLayout = new QFormLayout(gridGroup);

    m_gridSize = new QSpinBox;
    m_gridSize->setRange(4, 100);
    m_gridSize->setSuffix(" px");
    gridLayout->addRow(tr("Grid size:"), m_gridSize);

    m_snapToGrid = new QCheckBox(tr("Snap to grid"));
    gridLayout->addRow(m_snapToGrid);

    mainLayout->addWidget(gridGroup);

    // Display settings
    auto* displayGroup = new QGroupBox(tr("Display"));
    auto* displayLayout = new QFormLayout(displayGroup);

    m_defaultZoom = new QDoubleSpinBox;
    m_defaultZoom->setRange(0.1, 5.0);
    m_defaultZoom->setSingleStep(0.1);
    m_defaultZoom->setDecimals(1);
    m_defaultZoom->setSuffix("x");
    displayLayout->addRow(tr("Default zoom:"), m_defaultZoom);

    m_antialiasing = new QCheckBox(tr("Enable anti-aliasing"));
    m_antialiasing->setChecked(true);
    displayLayout->addRow(m_antialiasing);

    mainLayout->addWidget(displayGroup);

    // Language settings
    auto* langGroup = new QGroupBox(tr("Language"));
    auto* langLayout = new QFormLayout(langGroup);

    m_languageCombo = new QComboBox;
    m_languageCombo->addItem("English", "en_US");
    m_languageCombo->addItem(QStringLiteral("中文"), "zh_CN");
    langLayout->addRow(tr("Language:"), m_languageCombo);

    mainLayout->addWidget(langGroup);

    // Solver settings
    auto* solverGroup = new QGroupBox(tr("Solver"));
    auto* solverLayout = new QFormLayout(solverGroup);

    m_solverTolerance = new QDoubleSpinBox;
    m_solverTolerance->setRange(1e-12, 1e-1);
    m_solverTolerance->setDecimals(12);
    m_solverTolerance->setSingleStep(1e-6);
    m_solverTolerance->setValue(1e-6);
    solverLayout->addRow(tr("Tolerance:"), m_solverTolerance);

    m_solverMaxIter = new QSpinBox;
    m_solverMaxIter->setRange(10, 10000);
    m_solverMaxIter->setSingleStep(50);
    m_solverMaxIter->setValue(200);
    solverLayout->addRow(tr("Max iterations:"), m_solverMaxIter);

    m_solverRelaxation = new QDoubleSpinBox;
    m_solverRelaxation->setRange(0.1, 1.5);
    m_solverRelaxation->setSingleStep(0.05);
    m_solverRelaxation->setDecimals(2);
    m_solverRelaxation->setValue(1.0);
    solverLayout->addRow(tr("Relaxation factor:"), m_solverRelaxation);

    m_solverCourant = new QDoubleSpinBox;
    m_solverCourant->setRange(0.1, 1.0);
    m_solverCourant->setSingleStep(0.05);
    m_solverCourant->setDecimals(2);
    m_solverCourant->setValue(0.9);
    solverLayout->addRow(tr("Target Courant:"), m_solverCourant);

    m_solverTimeStep = new QDoubleSpinBox;
    m_solverTimeStep->setRange(-1.0, 1.0);
    m_solverTimeStep->setDecimals(6);
    m_solverTimeStep->setSingleStep(0.00001);
    m_solverTimeStep->setValue(-1.0);
    m_solverTimeStep->setSpecialValueText(tr("Auto (CFL)"));
    solverLayout->addRow(tr("Time step (s):"), m_solverTimeStep);

    m_solverGridNodes = new QSpinBox;
    m_solverGridNodes->setRange(10, 500);
    m_solverGridNodes->setSingleStep(10);
    m_solverGridNodes->setValue(50);
    solverLayout->addRow(tr("Grid nodes:"), m_solverGridNodes);

    mainLayout->addWidget(solverGroup);

    mainLayout->addStretch();

    // Buttons
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    loadSettings();
}

void PreferencesDialog::loadSettings()
{
    QSettings s;
    s.beginGroup("Preferences");
    m_gridSize->setValue(s.value("gridSize", 20).toInt());
    m_snapToGrid->setChecked(s.value("snapToGrid", true).toBool());
    m_defaultZoom->setValue(s.value("defaultZoom", 1.0).toDouble());
    m_antialiasing->setChecked(s.value("antialiasing", true).toBool());
    s.endGroup();

    // Language
    QString lang = QSettings().value("Preferences/Language", "en_US").toString();
    int langIdx = m_languageCombo->findData(lang);
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);

    s.beginGroup("Solver");
    m_solverTolerance->setValue(s.value("Tolerance", 1e-6).toDouble());
    m_solverMaxIter->setValue(s.value("MaxIter", 200).toInt());
    m_solverRelaxation->setValue(s.value("Relaxation", 1.0).toDouble());
    m_solverCourant->setValue(s.value("Courant", 0.9).toDouble());
    m_solverTimeStep->setValue(s.value("TimeStep", -1.0).toDouble());
    m_solverGridNodes->setValue(s.value("GridNodes", 50).toInt());
    s.endGroup();
}

void PreferencesDialog::saveSettings()
{
    QSettings s;
    s.beginGroup("Preferences");
    s.setValue("gridSize", m_gridSize->value());
    s.setValue("snapToGrid", m_snapToGrid->isChecked());
    s.setValue("defaultZoom", m_defaultZoom->value());
    s.setValue("antialiasing", m_antialiasing->isChecked());
    s.endGroup();

    // Language preference saved to QSettings; applied by MainWindow after accept
    QString newLang = m_languageCombo->currentData().toString();
    QSettings().setValue("Preferences/Language", newLang);

    s.beginGroup("Solver");
    s.setValue("Tolerance", m_solverTolerance->value());
    s.setValue("MaxIter", m_solverMaxIter->value());
    s.setValue("Relaxation", m_solverRelaxation->value());
    s.setValue("Courant", m_solverCourant->value());
    s.setValue("TimeStep", m_solverTimeStep->value());
    s.setValue("GridNodes", m_solverGridNodes->value());
    s.endGroup();
}
