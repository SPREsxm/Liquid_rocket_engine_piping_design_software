#include "MainWindow.h"
#include "ui/actions/ActionManager.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/BlockView.h"
#include "ui/graphics/ConnectionItem.h"
#include "ui/graphics/LegendWidget.h"
#include "ui/graphics/PortItem.h"
#include "ui/library/LibraryTreeModel.h"
#include "ui/library/LibraryTreeView.h"
#include "ui/library/ComponentFilterProxy.h"
#include "ui/properties/PropertyEditor.h"
#include "PreferencesDialog.h"
#include "SolverResultsPanel.h"
#include "utils/NetworkValidator.h"
#include "utils/NetworkSolver.h"
#include "utils/TransientSolver.h"
#include "utils/Benchmark.h"
#include "utils/GridRefinement.h"
#include "utils/DesignRules.h"
#include "utils/BomGenerator.h"
#include "utils/PipeOptimizer.h"
#include "utils/ThermalSolver.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "core/Constants.h"
#include "core/PluginManager.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHash>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QSvgGenerator>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Liquid Rocket Engine Piping Designer");
    resize(1600, 1000);

    // Core infrastructure
    m_componentFactory = &ComponentFactory::instance();
    m_undoStack = new QUndoStack(this);
    m_blockScene = new BlockScene(m_componentFactory, this);
    m_blockScene->setUndoStack(m_undoStack);
    m_blockView  = new BlockView(m_blockScene, this);
    m_actionManager = new ActionManager(this);

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    createDockWidgets();
    createCentralWidget();

    // Connect scene dirty signal
    connect(m_blockScene, &BlockScene::sceneModified, this, [this]() {
        setDirty(true);
    });

    // Connect selection change for status bar
    connect(m_blockScene, &BlockScene::blockSelectionChanged, this,
            [this](BlockItem* block) {
        if (block) {
            m_statusLabel->setText(tr("Selected: %1 [%2]")
                .arg(block->customLabel()).arg(block->typeId()));
        } else {
            auto sel = m_blockScene->selectedBlocks();
            if (sel.size() > 1) {
                m_statusLabel->setText(tr("Selected: %1 blocks").arg(sel.size()));
            } else {
                m_statusLabel->setText(tr("Ready"));
            }
        }
    });

    // Connect multi-selection for batch property editing
    connect(m_blockScene, &BlockScene::multiSelectionChanged, this,
            [this]() {
        auto sel = m_blockScene->selectedBlocks();
        m_propertyEditor->showBlocksProperties(sel);
    });

    restoreSettings();

    // Autosave — check for recovery files first, then start timer
    checkAutosaveRecovery();
    m_autosaveTimer = new QTimer(this);
    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosave);
    m_autosaveTimer->start(120000); // 2 minutes

    loadPlugins();

    appendMessage("Application started. Ready.");
}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ─── Plugin loading ──────────────────────────────────────────

void MainWindow::loadPlugins()
{
    QString pluginDir = QCoreApplication::applicationDirPath() + "/plugins";
    int count = PluginManager::instance().discoverPlugins(pluginDir);
    if (count > 0) {
        appendMessage(QStringLiteral("Loaded %1 plugin(s).").arg(count));

        // Register plugin components with ComponentFactory
        auto allInfos = PluginManager::instance().allPluginComponentInfos();
        for (const auto& info : allInfos) {
            ComponentDescriptor cd;
            cd.typeId = info.typeId;
            cd.displayName = info.displayName;
            cd.category = info.category;
            cd.description = info.description;
            // Create default port layout
            for (int i = 0; i < info.inputPortCount; ++i) {
                PortDescriptor p;
                p.id = QStringLiteral("in%1").arg(i + 1);
                p.displayName = QStringLiteral("Input %1").arg(i + 1);
                p.direction = PortDirection::Input;
                cd.inputPorts.append(p);
            }
            for (int i = 0; i < info.outputPortCount; ++i) {
                PortDescriptor p;
                p.id = QStringLiteral("out%1").arg(i + 1);
                p.displayName = QStringLiteral("Output %1").arg(i + 1);
                p.direction = PortDirection::Output;
                cd.outputPorts.append(p);
            }
            m_componentFactory->registerPluginComponent(cd);
            appendMessage(QStringLiteral("  Plugin component: %1 [%2]")
                .arg(info.displayName).arg(info.typeId));
        }
    }
}

// ─── Benchmark execution ────────────────────────────────────

void MainWindow::runBenchmarks()
{
    appendMessage("--- Accuracy Benchmarks ---");

    // Standard analytical benchmark cases
    auto r1 = Benchmark::benchmarkLaminarPipe();
    appendMessage(Benchmark::formatResult(r1));

    auto r2 = Benchmark::benchmarkJoukowsky();
    appendMessage(Benchmark::formatResult(r2));

    appendMessage("--- Benchmarks complete ---");
}

// ─── Actions ────────────────────────────────────────────────

void MainWindow::createActions()
{
    auto& am = *m_actionManager;

    // File
    connect(am.action(ActionId::New),    &QAction::triggered, this, &MainWindow::onNew);
    connect(am.action(ActionId::Open),   &QAction::triggered, this, &MainWindow::onOpen);
    connect(am.action(ActionId::Save),   &QAction::triggered, this, &MainWindow::onSave);
    connect(am.action(ActionId::SaveAs), &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(am.action(ActionId::Export_),&QAction::triggered, this, &MainWindow::onExport);
    connect(am.action(ActionId::Print),  &QAction::triggered, this, &MainWindow::onPrint);

    // View — Zoom
    connect(am.action(ActionId::ZoomIn),  &QAction::triggered, this, &MainWindow::onZoomIn);
    connect(am.action(ActionId::ZoomOut), &QAction::triggered, this, &MainWindow::onZoomOut);
    connect(am.action(ActionId::ZoomFit), &QAction::triggered, this, &MainWindow::onZoomFit);

    // View — Dock toggles
    auto* toggleLibrary = am.action(ActionId::ToggleLibrary);
    if (toggleLibrary && m_libraryDock) {
        connect(toggleLibrary, &QAction::toggled, m_libraryDock, &QDockWidget::setVisible);
        connect(m_libraryDock, &QDockWidget::visibilityChanged, toggleLibrary, &QAction::setChecked);
        toggleLibrary->setChecked(true);
    }

    auto* toggleProperties = am.action(ActionId::ToggleProperties);
    if (toggleProperties && m_propertyDock) {
        connect(toggleProperties, &QAction::toggled, m_propertyDock, &QDockWidget::setVisible);
        connect(m_propertyDock, &QDockWidget::visibilityChanged, toggleProperties, &QAction::setChecked);
        toggleProperties->setChecked(true);
    }

    auto* toggleMessages = am.action(ActionId::ToggleMessages);
    if (toggleMessages && m_messageDock) {
        connect(toggleMessages, &QAction::toggled, m_messageDock, &QDockWidget::setVisible);
        connect(m_messageDock, &QDockWidget::visibilityChanged, toggleMessages, &QAction::setChecked);
        toggleMessages->setChecked(true);
    }

    auto* toggleGrid = am.action(ActionId::ToggleGrid);
    if (toggleGrid) {
        connect(toggleGrid, &QAction::toggled, m_blockView, &BlockView::setGridVisible);
        toggleGrid->setChecked(true);
    }

    // Edit — Undo/Redo connected to QUndoStack
    QAction* undoAction = am.action(ActionId::Undo);
    QAction* redoAction = am.action(ActionId::Redo);
    connect(undoAction, &QAction::triggered, m_undoStack, &QUndoStack::undo);
    connect(redoAction, &QAction::triggered, m_undoStack, &QUndoStack::redo);
    connect(m_undoStack, &QUndoStack::canUndoChanged, undoAction, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, redoAction, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::undoTextChanged, undoAction, [undoAction](const QString& t) {
        undoAction->setText(QObject::tr("Undo %1").arg(t));
    });
    connect(m_undoStack, &QUndoStack::redoTextChanged, redoAction, [redoAction](const QString& t) {
        redoAction->setText(QObject::tr("Redo %1").arg(t));
    });
    undoAction->setEnabled(false);
    redoAction->setEnabled(false);

    // Edit — Cut/Copy/Paste/Delete
    connect(am.action(ActionId::Cut),  &QAction::triggered, m_blockView, &BlockView::cutSelected);
    connect(am.action(ActionId::Copy), &QAction::triggered, m_blockView, &BlockView::copySelected);
    connect(am.action(ActionId::Paste),&QAction::triggered, m_blockView, &BlockView::pasteClipboard);
    connect(am.action(ActionId::Delete), &QAction::triggered, m_blockView, &BlockView::deleteSelected);

    // Tools
    connect(am.action(ActionId::RunAnalysis), &QAction::triggered, this, &MainWindow::onRunAnalysis);
    connect(am.action(ActionId::Validate), &QAction::triggered, this, &MainWindow::onValidate);
    connect(am.action(ActionId::GenerateBom), &QAction::triggered, this, &MainWindow::onGenerateBom);
    connect(am.action(ActionId::OptimizePipes), &QAction::triggered, this, &MainWindow::onOptimize);
    connect(am.action(ActionId::Preferences), &QAction::triggered, this, &MainWindow::onPreferences);

    // Help
    connect(am.action(ActionId::About), &QAction::triggered, this, &MainWindow::onAbout);
}

// ─── Menus ──────────────────────────────────────────────────

void MainWindow::createMenus()
{
    auto& am = *m_actionManager;

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(am.action(ActionId::New));
    m_fileMenu->addAction(am.action(ActionId::Open));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(am.action(ActionId::Save));
    m_fileMenu->addAction(am.action(ActionId::SaveAs));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(am.action(ActionId::Export_));
    m_fileMenu->addAction(am.action(ActionId::Print));
    m_fileMenu->addSeparator();
    m_recentFilesMenu = m_fileMenu->addMenu(tr("&Recent Files"));
    updateRecentFilesMenu();
    m_fileMenu->addSeparator();
    {
        auto* exitAction = new QAction(tr("E&xit"), this);
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
        m_fileMenu->addAction(exitAction);
    }

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(am.action(ActionId::Undo));
    m_editMenu->addAction(am.action(ActionId::Redo));
    m_editMenu->addSeparator();
    m_editMenu->addAction(am.action(ActionId::Cut));
    m_editMenu->addAction(am.action(ActionId::Copy));
    m_editMenu->addAction(am.action(ActionId::Paste));
    m_editMenu->addAction(am.action(ActionId::Delete));

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(am.action(ActionId::ZoomIn));
    m_viewMenu->addAction(am.action(ActionId::ZoomOut));
    m_viewMenu->addAction(am.action(ActionId::ZoomFit));
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(am.action(ActionId::ToggleLibrary));
    m_viewMenu->addAction(am.action(ActionId::ToggleProperties));
    m_viewMenu->addAction(am.action(ActionId::ToggleMessages));
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(am.action(ActionId::ToggleGrid));

    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(am.action(ActionId::RunAnalysis));
    m_toolsMenu->addAction(am.action(ActionId::Validate));
    m_toolsMenu->addAction(am.action(ActionId::GenerateBom));
    m_toolsMenu->addAction(am.action(ActionId::OptimizePipes));
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction(am.action(ActionId::Preferences));

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(am.action(ActionId::About));
    m_helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

// ─── ToolBar ────────────────────────────────────────────────

void MainWindow::createToolBar()
{
    auto& am = *m_actionManager;

    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setMovable(false);

    m_mainToolBar->addAction(am.action(ActionId::New));
    m_mainToolBar->addAction(am.action(ActionId::Open));
    m_mainToolBar->addAction(am.action(ActionId::Save));
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(am.action(ActionId::Undo));
    m_mainToolBar->addAction(am.action(ActionId::Redo));
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(am.action(ActionId::RunAnalysis));
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(am.action(ActionId::ZoomIn));
    m_mainToolBar->addAction(am.action(ActionId::ZoomOut));

    // ── Boundary condition inputs ──
    m_mainToolBar->addSeparator();

    auto* inletLabel = new QLabel(tr(" P_in:"));
    m_mainToolBar->addWidget(inletLabel);

    m_inletPressureSpin = new QDoubleSpinBox;
    m_inletPressureSpin->setRange(0.01, 100.0);
    m_inletPressureSpin->setDecimals(3);
    m_inletPressureSpin->setSuffix(" MPa");
    m_inletPressureSpin->setValue(1.0);  // default 1 MPa
    m_inletPressureSpin->setToolTip(tr("Inlet total pressure (MPa)"));
    m_inletPressureSpin->setMaximumWidth(110);
    m_mainToolBar->addWidget(m_inletPressureSpin);

    auto* flowLabel = new QLabel(tr(" ṁ:"));
    m_mainToolBar->addWidget(flowLabel);

    m_inletFlowSpin = new QDoubleSpinBox;
    m_inletFlowSpin->setRange(0.01, 10000.0);
    m_inletFlowSpin->setDecimals(2);
    m_inletFlowSpin->setSuffix(" kg/s");
    m_inletFlowSpin->setValue(10.0);
    m_inletFlowSpin->setToolTip(tr("Inlet mass flow rate (kg/s)"));
    m_inletFlowSpin->setMaximumWidth(110);
    m_mainToolBar->addWidget(m_inletFlowSpin);

    // ── Fluid type selector ──
    auto* fluidLabel = new QLabel(tr(" Fluid:"));
    m_mainToolBar->addWidget(fluidLabel);

    m_fluidTypeCombo = new QComboBox;
    m_fluidTypeCombo->addItem("LOX",       static_cast<int>(FluidType::LOX));
    m_fluidTypeCombo->addItem("RP-1",      static_cast<int>(FluidType::RP1));
    m_fluidTypeCombo->addItem("Methane",   static_cast<int>(FluidType::CH4));
    m_fluidTypeCombo->addItem("LH2",       static_cast<int>(FluidType::LH2));
    m_fluidTypeCombo->addItem("Water",     static_cast<int>(FluidType::Water));
    m_fluidTypeCombo->setCurrentIndex(0);
    m_fluidTypeCombo->setToolTip(tr("Working fluid — sets density and viscosity defaults"));
    m_fluidTypeCombo->setMaximumWidth(100);
    m_mainToolBar->addWidget(m_fluidTypeCombo);

    // ── Max pressure drop budget ──
    m_mainToolBar->addSeparator();
    auto* maxDpLabel = new QLabel(tr(" ΔP_max:"));
    m_mainToolBar->addWidget(maxDpLabel);

    m_maxPressureDropSpin = new QDoubleSpinBox;
    m_maxPressureDropSpin->setRange(0.0, 100.0);
    m_maxPressureDropSpin->setDecimals(3);
    m_maxPressureDropSpin->setSuffix(" MPa");
    m_maxPressureDropSpin->setSpecialValueText(tr("Off"));
    m_maxPressureDropSpin->setValue(0.0);
    m_maxPressureDropSpin->setToolTip(tr("Max allowed pressure drop (0 = disabled)"));
    m_maxPressureDropSpin->setMaximumWidth(110);
    m_mainToolBar->addWidget(m_maxPressureDropSpin);
}

// ─── StatusBar ──────────────────────────────────────────────

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(m_statusLabel, 1);

    m_zoomLabel = new QLabel(tr("100%"));
    statusBar()->addPermanentWidget(m_zoomLabel);

    connect(m_blockView, &BlockView::zoomChanged, this, [this](double factor) {
        m_zoomLabel->setText(QStringLiteral("%1%").arg(qRound(factor * 100)));
    });
}

// ─── Dock Widgets ───────────────────────────────────────────

void MainWindow::createDockWidgets()
{
    // Library browser (left)
    m_libraryDock = new QDockWidget(tr("Component Library"), this);
    m_libraryDock->setObjectName("LibraryDock");
    m_libraryDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_libraryModel = new LibraryTreeModel(m_componentFactory, this);
    m_libraryView  = new LibraryTreeView;

    // Filter proxy
    m_libraryFilterProxy = new ComponentFilterProxy(this);
    m_libraryFilterProxy->setSourceModel(m_libraryModel);
    m_libraryFilterProxy->setFilterKeyColumn(0);
    m_libraryView->setModel(m_libraryFilterProxy);

    // Search box
    m_librarySearchBox = new QLineEdit;
    m_librarySearchBox->setPlaceholderText(tr("Filter components..."));
    m_librarySearchBox->setClearButtonEnabled(true);
    connect(m_librarySearchBox, &QLineEdit::textChanged, this,
            [this](const QString& text) {
        m_libraryFilterProxy->setFilterWildcard(text);
        m_libraryView->expandAll();
    });

    auto* libContainer = new QWidget;
    auto* libLayout = new QVBoxLayout(libContainer);
    libLayout->setContentsMargins(4, 4, 4, 4);
    libLayout->setSpacing(4);
    libLayout->addWidget(m_librarySearchBox);
    libLayout->addWidget(m_libraryView);
    m_libraryDock->setWidget(libContainer);

    addDockWidget(Qt::LeftDockWidgetArea, m_libraryDock);

    // Property editor (right)
    m_propertyEditor = new PropertyEditor;
    m_propertyDock = m_propertyEditor;  // PropertyEditor IS a QDockWidget
    m_propertyDock->setObjectName("PropertyDock");
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);

    // Property editor now driven by multiSelectionChanged in constructor above

    // Message log (bottom)
    m_messageDock = new QDockWidget(tr("Messages"), this);
    m_messageDock->setObjectName("MessageDock");
    m_messageDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_messageLog = new QPlainTextEdit;
    m_messageLog->setReadOnly(true);
    m_messageLog->setMaximumBlockCount(1000);
    m_messageLog->setFont(QFont("Consolas", 9));
    m_messageDock->setWidget(m_messageLog);
    addDockWidget(Qt::BottomDockWidgetArea, m_messageDock);

    // Solver results (bottom, tabbed with messages)
    m_resultsDock = new SolverResultsPanel(this);
    m_resultsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_resultsDock);
    tabifyDockWidget(m_messageDock, m_resultsDock);
    m_messageDock->raise(); // show messages by default

    // Flow/Pressure color legend (right)
    m_legendWidget = new LegendWidget;
    m_legendDock = new QDockWidget(tr("Flow Legend"), this);
    m_legendDock->setObjectName("LegendDock");
    m_legendDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_legendDock->setWidget(m_legendWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_legendDock);
}

// ─── Central Widget ─────────────────────────────────────────

void MainWindow::createCentralWidget()
{
    setCentralWidget(m_blockView);
}

// ─── View Actions ───────────────────────────────────────────

void MainWindow::onZoomIn()  { m_blockView->zoomIn(); }
void MainWindow::onZoomOut() { m_blockView->zoomOut(); }
void MainWindow::onZoomFit() { m_blockView->zoomToFit(); }

// ─── File Operations ────────────────────────────────────────

void MainWindow::onNew()
{
    if (!maybeSave()) return;
    m_blockScene->clearScene();
    m_undoStack->clear();
    m_currentFilePath.clear();
    setDirty(false);
    m_statusLabel->setText(tr("New project created"));
    appendMessage("New project.");
}

void MainWindow::onOpen()
{
    if (!maybeSave()) return;
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Project"), QString(),
        tr("LRE Project (*.lrep);;All Files (*)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file: %1").arg(file.errorString()));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isObject()) {
        m_blockScene->fromJson(doc.object());
        m_currentFilePath = path;
        setDirty(false);
        m_statusLabel->setText(tr("Opened: %1").arg(path));
        appendMessage("Opened: " + path);
        addToRecentFiles(path);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Invalid project file format."));
    }
}

void MainWindow::onSave()
{
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
        return;
    }

    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot save file: %1").arg(file.errorString()));
        return;
    }

    QJsonDocument doc(m_blockScene->toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    setDirty(false);
    m_statusLabel->setText(tr("Saved: %1").arg(m_currentFilePath));
    appendMessage("Saved: " + m_currentFilePath);
    addToRecentFiles(m_currentFilePath);
    removeAutosaveFile();
}

void MainWindow::onSaveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Project As"), QString(),
        tr("LRE Project (*.lrep);;All Files (*)"));
    if (path.isEmpty()) return;

    m_currentFilePath = path;
    onSave();
}

void MainWindow::onExport()
{
    QStringList formats;
    formats << tr("PNG Image (*.png)")
            << tr("SVG Vector (*.svg)")
            << tr("PDF Document (*.pdf)");

    QString selectedFilter;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Diagram"), QString(), formats.join(";;"), &selectedFilter);
    if (path.isEmpty()) return;

    QRectF bounds = m_blockScene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    if (bounds.isEmpty()) bounds = QRectF(-200, -200, 400, 400);

    if (selectedFilter.contains("svg")) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(bounds.size().toSize());
        generator.setViewBox(bounds);
        QPainter painter(&generator);
        painter.setRenderHint(QPainter::Antialiasing);
        m_blockScene->render(&painter, QRectF(), bounds);
        painter.end();
    } else if (selectedFilter.contains("pdf")) {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(bounds.size().toSize(), QPageSize::Point));
        QPainter painter(&writer);
        painter.setRenderHint(QPainter::Antialiasing);
        m_blockScene->render(&painter, QRectF(), bounds);
        painter.end();
    } else {
        // PNG (default)
        QImage image(bounds.size().toSize() * 2, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        m_blockScene->render(&painter, QRectF(), bounds);
        painter.end();
        image.save(path);
    }

    appendMessage("Exported to: " + path);
    m_statusLabel->setText(tr("Exported to: %1").arg(path));
}

void MainWindow::onPrint()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize::A4);

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle(tr("Print Schematic Diagram"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    QRectF bounds = m_blockScene->itemsBoundingRect().adjusted(-10, -10, 10, 10);
    if (bounds.isEmpty()) bounds = QRectF(-200, -200, 400, 400);

    QPainter painter(&printer);
    painter.setRenderHint(QPainter::Antialiasing);

    // Scale scene to fit page
    QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);
    double scaleX = pageRect.width() / bounds.width();
    double scaleY = pageRect.height() / bounds.height();
    double scale = std::min(scaleX, scaleY);

    painter.save();
    painter.scale(scale, scale);
    painter.translate(-bounds.topLeft());
    m_blockScene->render(&painter, QRectF(), bounds);
    painter.restore();

    painter.end();
    appendMessage("Printed schematic diagram.");
    m_statusLabel->setText(tr("Printed schematic diagram"));
}

// ─── Tools ──────────────────────────────────────────────────

void MainWindow::onRunAnalysis()
{
    // Quick topology check — stop on errors
    auto topo = validateTopology(m_blockScene);
    if (topo.hasErrors()) {
        QMessageBox::warning(this, tr("Analysis"),
            tr("Topology errors detected. Fix them first.\n\n%1")
                .arg(topo.issues.first().message));
        return;
    }

    SolverSettings solverSettings = SolverSettings::fromQSettings();

    // Read boundary conditions from toolbar controls
    const double inletPressurePa = m_inletPressureSpin->value() * 1.0e6; // MPa → Pa
    const double inletMassFlow = m_inletFlowSpin->value();

    // Set fluid properties from fluid type selector
    FluidType fType = static_cast<FluidType>(m_fluidTypeCombo->currentData().toInt());
    auto fProps = fluidDefaults(fType);
    QSettings settings;
    solverSettings.fluidDensity = settings.value("Solver/FluidDensity", fProps.density).toDouble();
    solverSettings.fluidViscosity = settings.value("Solver/FluidViscosity", fProps.viscosity).toDouble();
    solverSettings.fluidType = fType;

    NetworkSolution sol = solveNetworkAuto(m_blockScene, solverSettings,
                                           inletPressurePa, inletMassFlow);
    appendMessage(sol.message);

    // Display results in panel
    m_resultsDock->setResults(sol);
    m_resultsDock->raise();

    if (!sol.converged) {
        m_statusLabel->setText(tr("Analysis did not converge"));
        return;
    }

    applySolutionVisualization(sol);

    // Provide context for sensitivity analysis
    m_resultsDock->setAnalysisContext(m_blockScene, solverSettings,
                                      inletPressurePa, inletMassFlow);

    // Run design rule checks
    double maxDpPa = m_maxPressureDropSpin ? m_maxPressureDropSpin->value() * 1.0e6 : -1.0;
    DesignCheckResult designResult = runDesignChecks(m_blockScene, sol, solverSettings, maxDpPa);
    m_resultsDock->setDesignCheckResults(designResult);
    if (designResult.errorCount() > 0)
        appendMessage(QString("Design checks: %1 error(s), %2 warning(s)")
            .arg(designResult.errorCount()).arg(designResult.warningCount()));

    // Generate BOM data for report inclusion
    BomResult bom = generateBom(m_blockScene);
    m_resultsDock->setBomData(bom);

    // Run transient water hammer simulation
    TransientSolver transSolver;
    transSolver.setTargetCourant(solverSettings.targetCourant);
    transSolver.setDefaultRoughness(solverSettings.pipeRoughness);
    transSolver.setDefaultYoungsModulus(solverSettings.pipeYoungsModulus);
    transSolver.setDefaultWallThickness(solverSettings.pipeWallThickness);
    TransientResult trans = transSolver.simulateWaterHammer(
        sol, m_blockScene, 0.050, solverSettings.gridBaseNodes,
        solverSettings.timeStepSeconds);
    m_resultsDock->setTransientResults(trans);
    appendMessage(trans.message);

    // Compute thermal-structural post-processing
    ThermalStressResult tsResult = computeThermalStress(m_blockScene, sol, solverSettings);
    m_resultsDock->setThermalStressResults(tsResult);
    appendMessage(QString("Thermal/Stress: %1 edges, min SF=%2, yield exceeded=%3")
        .arg(tsResult.edges.size())
        .arg(tsResult.minSafetyFactor, 0, 'f', 2)
        .arg(tsResult.edgesWithYieldExceeded));

    // Run extended design checks including pipe stress
    DesignCheckResult extDesignResult = runDesignChecks(
        m_blockScene, sol, solverSettings, maxDpPa, tsResult);
    if (extDesignResult.items.size() > designResult.items.size()) {
        m_resultsDock->setDesignCheckResults(extDesignResult);
        if (extDesignResult.errorCount() > designResult.errorCount())
            appendMessage(QString("Extended design checks: %1 error(s), %2 warning(s)")
                .arg(extDesignResult.errorCount()).arg(extDesignResult.warningCount()));
    }

    m_blockScene->update();
    m_statusLabel->setText(tr("Analysis complete — %1 nodes, %2 edges, ΔP=%3 Pa")
        .arg(sol.nodes.size()).arg(sol.edges.size())
        .arg(sol.totalPressureDrop, 0, 'f', 1));
}

void MainWindow::onValidate()
{
    appendMessage("--- Validation started ---");

    // 1. Topology check
    ValidationResult topoResult = validateTopology(m_blockScene);
    for (const auto& issue : topoResult.issues) {
        QString prefix;
        switch (issue.severity) {
        case ValidationIssue::Error:   prefix = "ERROR: ";   break;
        case ValidationIssue::Warning: prefix = "WARNING: "; break;
        default:                       prefix = "INFO: ";
        }
        appendMessage(prefix + issue.message);
    }
    if (topoResult.issues.isEmpty()) {
        appendMessage("Topology validation passed — no issues found.");
    }

    // 2. Flow continuity check
    ValidationResult flowResult = validateFlowContinuity(m_blockScene);
    for (const auto& issue : flowResult.issues) {
        appendMessage("FLOW: " + issue.message);
    }

    // 3. Network solver
    if (!topoResult.hasErrors()) {
        SolverSettings solverSettings = SolverSettings::fromQSettings();

        // Read boundary conditions from toolbar controls
        const double inletPressurePa = m_inletPressureSpin->value() * 1.0e6;
        const double inletMassFlow = m_inletFlowSpin->value();

        // Set fluid properties from fluid type selector
        FluidType fType = static_cast<FluidType>(m_fluidTypeCombo->currentData().toInt());
        auto fProps = fluidDefaults(fType);
        QSettings settings;
        solverSettings.fluidDensity = settings.value("Solver/FluidDensity", fProps.density).toDouble();
        solverSettings.fluidViscosity = settings.value("Solver/FluidViscosity", fProps.viscosity).toDouble();
        solverSettings.fluidType = fType;

        NetworkSolution sol = solveNetworkAuto(m_blockScene, solverSettings,
                                               inletPressurePa, inletMassFlow);
        appendMessage(sol.message);
        if (sol.converged) {
            for (const auto& node : sol.nodes) {
                appendMessage(QStringLiteral("  Node %1: p=%2 Pa, ṁ_in=%3, ṁ_out=%4 kg/s")
                    .arg(node.blockLabel)
                    .arg(node.pressure, 0, 'f', 1)
                    .arg(node.inletFlow, 0, 'g', 4)
                    .arg(node.outletFlow, 0, 'g', 4));
            }

            // 3a. Transient water hammer simulation
            TransientSolver transSolver;
            transSolver.setTargetCourant(solverSettings.targetCourant);
            transSolver.setDefaultRoughness(solverSettings.pipeRoughness);
            transSolver.setDefaultYoungsModulus(solverSettings.pipeYoungsModulus);
            transSolver.setDefaultWallThickness(solverSettings.pipeWallThickness);
            TransientResult trans = transSolver.simulateWaterHammer(
                sol, m_blockScene, 0.050, solverSettings.gridBaseNodes,
                solverSettings.timeStepSeconds);
            appendMessage(trans.message);
            if (trans.maxPressure > 0) {
                appendMessage(QStringLiteral("  *** Max water hammer pressure: %1 MPa ***")
                    .arg(trans.maxPressure / 1.0e6, 0, 'f', 3));
            }

            // 3b. Flow visualization
            applySolutionVisualization(sol);

            // 3c. Accuracy benchmarks
            runBenchmarks();
        }
    } else {
        appendMessage("Solver skipped — fix topology errors first.");
    }

    appendMessage("--- Validation complete ---");
    m_statusLabel->setText(tr("Validation complete — see Messages panel"));
}

void MainWindow::onPreferences()
{
    PreferencesDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        m_blockView->viewport()->update();
        appendMessage("Preferences updated.");
    }
}

void MainWindow::onGenerateBom()
{
    BomResult bom = generateBom(m_blockScene);
    if (bom.lines.isEmpty()) {
        QMessageBox::information(this, tr("BOM"),
            tr("No components in the scene."));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export BOM"), "bom.csv",
        tr("CSV Files (*.csv)"));
    if (filePath.isEmpty()) return;

    if (exportBomCsv(bom, filePath)) {
        appendMessage(QString("BOM exported to %1 — %2 items, total %3 kg")
            .arg(filePath).arg(bom.lines.size()).arg(bom.totalWeight, 0, 'f', 1));
        QMessageBox::information(this, tr("BOM Export"),
            tr("BOM saved to:\n%1\n\n%2 items, total %3 kg")
                .arg(filePath).arg(bom.lines.size())
                .arg(bom.totalWeight, 0, 'f', 1));
    } else {
        QMessageBox::warning(this, tr("BOM Export"),
            tr("Failed to write BOM file."));
    }
}

void MainWindow::onOptimize()
{
    // Quick topology check
    auto topo = validateTopology(m_blockScene);
    if (topo.hasErrors()) {
        QMessageBox::warning(this, tr("Optimization"),
            tr("Fix topology errors before optimizing."));
        return;
    }

    SolverSettings solverSettings = SolverSettings::fromQSettings();
    const double inletPressurePa = m_inletPressureSpin->value() * 1.0e6;
    const double inletMassFlow = m_inletFlowSpin->value();
    double maxDpPa = m_maxPressureDropSpin ? m_maxPressureDropSpin->value() * 1.0e6 : -1.0;

    FluidType fType = static_cast<FluidType>(m_fluidTypeCombo->currentData().toInt());
    auto fProps = fluidDefaults(fType);
    QSettings qs;
    solverSettings.fluidDensity = qs.value("Solver/FluidDensity", fProps.density).toDouble();
    solverSettings.fluidViscosity = qs.value("Solver/FluidViscosity", fProps.viscosity).toDouble();
    solverSettings.fluidType = fType;

    appendMessage("--- Pipe optimization started ---");
    m_statusLabel->setText(tr("Optimizing pipe schedules..."));

    OptimizationResult result = optimizePipeSchedules(
        m_blockScene, solverSettings, inletPressurePa, inletMassFlow, maxDpPa);

    m_resultsDock->setOptimizationResults(result);
    m_resultsDock->raise();

    appendMessage(QString("Optimization complete — weight: %1 → %2 kg (saved %3 kg, %4%)")
        .arg(result.originalTotalWeight_kg, 0, 'f', 1)
        .arg(result.optimizedTotalWeight_kg, 0, 'f', 1)
        .arg(result.weightSaved_kg, 0, 'f', 1)
        .arg(result.originalTotalWeight_kg > 0.0
             ? result.weightSaved_kg / result.originalTotalWeight_kg * 100.0 : 0.0, 0, 'f', 1));

    if (!result.allConstraintsSatisfied)
        appendMessage(QString("WARNING: %1 constraint(s) violated")
            .arg(result.violatedConstraints.size()));

    m_statusLabel->setText(tr("Optimization complete"));
    m_blockScene->update();
}

// ─── Help ───────────────────────────────────────────────────

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About"),
        tr("<h3>Liquid Rocket Engine Piping Designer</h3>"
           "<p>Version %1</p>"
           "<p>A professional piping design platform for "
           "liquid rocket engines.</p>"
           "<p>Built with Qt %2 and C++20.</p>")
        .arg(QApplication::applicationVersion())
        .arg(QString::fromLatin1(qVersion())));
}

// ─── Recent Files ────────────────────────────────────────────

void MainWindow::addToRecentFiles(const QString& filePath)
{
    QSettings settings;
    QStringList recent = settings.value("RecentFiles/files").toStringList();
    recent.removeAll(filePath);
    recent.prepend(filePath);
    while (recent.size() > 10)
        recent.removeLast();
    settings.setValue("RecentFiles/files", recent);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();

    QSettings settings;
    const QStringList recent = settings.value("RecentFiles/files").toStringList();
    for (const QString& path : recent) {
        auto* action = m_recentFilesMenu->addAction(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }
    m_recentFilesMenu->setEnabled(!recent.isEmpty());
}

void MainWindow::openRecentFile()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    const QString path = action->text();

    if (!maybeSave()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file: %1").arg(file.errorString()));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isObject()) {
        m_blockScene->fromJson(doc.object());
        m_currentFilePath = path;
        setDirty(false);
        m_statusLabel->setText(tr("Opened: %1").arg(path));
        appendMessage("Opened: " + path);
        addToRecentFiles(path);
    }
}

// ─── Settings Persistence ───────────────────────────────────

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();

    settings.beginGroup("BoundaryConditions");
    settings.setValue("InletPressureMPa", m_inletPressureSpin->value());
    settings.setValue("InletFlowKgPerS", m_inletFlowSpin->value());
    settings.setValue("FluidType", m_fluidTypeCombo->currentIndex());
    settings.endGroup();
}

void MainWindow::restoreSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    const QByteArray geom = settings.value("geometry").toByteArray();
    if (!geom.isEmpty()) restoreGeometry(geom);
    const QByteArray state = settings.value("windowState").toByteArray();
    if (!state.isEmpty()) restoreState(state);
    settings.endGroup();

    // Ensure docks are always shown on startup (user may have closed them last session)
    if (m_libraryDock && m_libraryDock->isHidden()) m_libraryDock->setVisible(true);
    if (m_propertyDock && m_propertyDock->isHidden()) m_propertyDock->setVisible(true);
    if (m_messageDock && m_messageDock->isHidden()) m_messageDock->setVisible(true);

    // Sync View menu toggle state with actual dock visibility
    if (m_actionManager) {
        if (auto* a = m_actionManager->action(ActionId::ToggleLibrary)) a->setChecked(m_libraryDock && m_libraryDock->isVisible());
        if (auto* a = m_actionManager->action(ActionId::ToggleProperties)) a->setChecked(m_propertyDock && m_propertyDock->isVisible());
        if (auto* a = m_actionManager->action(ActionId::ToggleMessages)) a->setChecked(m_messageDock && m_messageDock->isVisible());
    }

    // Restore boundary condition values
    QSettings bcSettings;
    bcSettings.beginGroup("BoundaryConditions");
    if (m_inletPressureSpin)
        m_inletPressureSpin->setValue(bcSettings.value("InletPressureMPa", 1.0).toDouble());
    if (m_inletFlowSpin)
        m_inletFlowSpin->setValue(bcSettings.value("InletFlowKgPerS", 10.0).toDouble());
    if (m_fluidTypeCombo)
        m_fluidTypeCombo->setCurrentIndex(bcSettings.value("FluidType", 0).toInt());
    bcSettings.endGroup();
}

// ─── Helpers ────────────────────────────────────────────────

void MainWindow::setDirty(bool dirty)
{
    m_isDirty = dirty;
    setWindowModified(dirty);
}

bool MainWindow::maybeSave()
{
    if (!m_isDirty) return true;

    const auto btn = QMessageBox::warning(this, tr("Unsaved Changes"),
        tr("The project has unsaved changes. Save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (btn) {
    case QMessageBox::Save: onSave(); return !m_isDirty;
    case QMessageBox::Discard: return true;
    default: return false;
    }
}

void MainWindow::appendMessage(const QString& message)
{
    if (m_messageLog) {
        m_messageLog->appendPlainText(
            QTime::currentTime().toString("[hh:mm:ss] ") + message);
    }
}

void MainWindow::applySolutionVisualization(const NetworkSolution& sol)
{
    QHash<QUuid, BlockItem*> blockMap;
    for (auto* b : m_blockScene->allBlocks())
        blockMap[b->uuid()] = b;

    double maxFlow = 0.0;
    for (const auto& edge : sol.edges)
        maxFlow = std::max(maxFlow, std::abs(edge.massFlowRate));

    for (auto* conn : m_blockScene->allConnections()) {
        auto* sp = conn->sourcePort();
        auto* dp = conn->destPort();
        if (!sp || !dp) continue;
        auto* sb = sp->parentBlock();
        auto* db = dp->parentBlock();
        if (!sb || !db) continue;

        for (const auto& edge : sol.edges) {
            if (edge.sourceUuid == sb->uuid() &&
                edge.destUuid == db->uuid()) {
                conn->setFlowData(std::abs(edge.massFlowRate), maxFlow);
                conn->setAnalysisTooltip(edge.pressureDrop, edge.massFlowRate);
                break;
            }
        }
    }

    for (const auto& node : sol.nodes) {
        auto* b = blockMap.value(node.blockUuid);
        if (b) {
            b->setPressure(node.pressure);
            b->setAnalysisTooltip(node.pressure, node.inletFlow, node.outletFlow);
        }
    }

    if (m_legendWidget) {
        m_legendWidget->setFlowRange(0.0, maxFlow);
        m_legendWidget->setPressureRange(sol.minPressure(), sol.maxPressure());
    }

    m_blockScene->update();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

// ─── Autosave ───────────────────────────────────────────────────

QString MainWindow::autosavePath() const
{
    if (!m_currentFilePath.isEmpty())
        return m_currentFilePath + ".autosave";
    return QCoreApplication::applicationDirPath() + "/.autosave.lrep";
}

void MainWindow::onAutosave()
{
    if (!m_isDirty) return;

    QFile file(autosavePath());
    if (!file.open(QIODevice::WriteOnly)) {
        appendMessage("Autosave: failed to write " + autosavePath());
        return;
    }

    QJsonDocument doc(m_blockScene->toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    appendMessage("Autosave: saved to " + autosavePath());
}

void MainWindow::checkAutosaveRecovery()
{
    QString path = autosavePath();
    if (!QFile::exists(path)) return;

    const auto btn = QMessageBox::question(this, tr("Recover Unsaved Work"),
        tr("An autosave file was found from a previous session.\n\n"
           "File: %1\n\nRecover this file?").arg(path),
        QMessageBox::Yes | QMessageBox::No);

    if (btn != QMessageBox::Yes) {
        removeAutosaveFile();
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError) {
        appendMessage("Autosave recovery: JSON parse error");
        removeAutosaveFile();
        return;
    }

    m_blockScene->fromJson(doc.object());
    setDirty(true);
    appendMessage("Recovered autosave from " + path);
    m_statusLabel->setText(tr("Recovered autosave"));
}

void MainWindow::removeAutosaveFile()
{
    QString path = autosavePath();
    if (QFile::exists(path))
        QFile::remove(path);
}
