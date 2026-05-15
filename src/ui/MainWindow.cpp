#include "MainWindow.h"
#include "ui/actions/ActionManager.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/BlockView.h"
#include "ui/graphics/ConnectionItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/library/LibraryTreeModel.h"
#include "ui/library/LibraryTreeView.h"
#include "ui/properties/PropertyEditor.h"
#include "PreferencesDialog.h"
#include "utils/NetworkValidator.h"
#include "utils/NetworkSolver.h"
#include "utils/TransientSolver.h"
#include "utils/Benchmark.h"
#include "utils/GridRefinement.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "core/Constants.h"
#include "core/PluginManager.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QHash>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QSvgGenerator>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
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
            m_statusLabel->setText(tr("Ready"));
        }
    });

    restoreSettings();

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

    // Delete action
    connect(am.action(ActionId::Delete), &QAction::triggered, m_blockView, &BlockView::deleteSelected);

    // Tools
    connect(am.action(ActionId::Validate), &QAction::triggered, this, &MainWindow::onValidate);
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

    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_toolsMenu->addAction(am.action(ActionId::Validate));
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
    m_mainToolBar->addAction(am.action(ActionId::ZoomIn));
    m_mainToolBar->addAction(am.action(ActionId::ZoomOut));
}

// ─── StatusBar ──────────────────────────────────────────────

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(m_statusLabel, 1);
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
    m_libraryView->setModel(m_libraryModel);
    m_libraryDock->setWidget(m_libraryView);
    addDockWidget(Qt::LeftDockWidgetArea, m_libraryDock);

    // Property editor (right)
    m_propertyEditor = new PropertyEditor;
    m_propertyDock = m_propertyEditor;  // PropertyEditor IS a QDockWidget
    m_propertyDock->setObjectName("PropertyDock");
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);

    // Connect scene selection to property editor
    connect(m_blockScene, &BlockScene::blockSelectionChanged,
            m_propertyEditor, &PropertyEditor::showBlockProperties);

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

// ─── Tools ──────────────────────────────────────────────────

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
        // Load solver settings
        QSettings settings;
        SolverSettings solverSettings;
        solverSettings.tolerance = settings.value("Solver/Tolerance", 1e-6).toDouble();
        solverSettings.maxIterations = settings.value("Solver/MaxIter", 200).toInt();
        solverSettings.relaxationFactor = settings.value("Solver/Relaxation", 1.0).toDouble();
        solverSettings.targetCourant = settings.value("Solver/Courant", 0.9).toDouble();
        solverSettings.timeStepSeconds = settings.value("Solver/TimeStep", -1.0).toDouble();
        solverSettings.gridBaseNodes = settings.value("Solver/GridNodes", 50).toInt();

        NetworkSolution sol = solveNetworkAuto(m_blockScene, solverSettings);
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
            TransientResult trans = transSolver.simulateWaterHammer(
                sol, m_blockScene, 0.050, solverSettings.gridBaseNodes,
                solverSettings.timeStepSeconds);
            appendMessage(trans.message);
            if (trans.maxPressure > 0) {
                appendMessage(QStringLiteral("  *** Max water hammer pressure: %1 MPa ***")
                    .arg(trans.maxPressure / 1.0e6, 0, 'f', 3));
            }

            // 3b. Flow visualization
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
                        break;
                    }
                }
            }

            for (const auto& node : sol.nodes) {
                auto* b = blockMap.value(node.blockUuid);
                if (b) b->setPressure(node.pressure);
            }

            m_blockScene->update();

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

// ─── Settings Persistence ───────────────────────────────────

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
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

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}
