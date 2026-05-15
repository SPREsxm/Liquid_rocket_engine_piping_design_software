#pragma once

#include <QMainWindow>

class QDockWidget;
class QPlainTextEdit;
class QLabel;
class QAction;
class ActionManager;
class BlockScene;
class BlockView;
class ComponentFactory;
class LibraryTreeView;
class LibraryTreeModel;
class PropertyEditor;
class QUndoStack;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createDockWidgets();
    void createCentralWidget();
    void saveSettings();
    void restoreSettings();

    // File actions
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExport();

    // View actions
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();

    // Tools
    void onValidate();
    void onPreferences();

    // Plugin & solver
    void loadPlugins();
    void runBenchmarks();

    // Help
    void onAbout();

    // Menu bar
    QMenu* m_fileMenu    = nullptr;
    QMenu* m_editMenu    = nullptr;
    QMenu* m_viewMenu    = nullptr;
    QMenu* m_toolsMenu   = nullptr;
    QMenu* m_helpMenu    = nullptr;

    // Toolbar
    QToolBar* m_mainToolBar = nullptr;

    // Docks
    QDockWidget* m_libraryDock    = nullptr;
    QDockWidget* m_propertyDock   = nullptr;
    QDockWidget* m_messageDock    = nullptr;

    // Status bar
    QLabel* m_statusLabel = nullptr;

    // Message log
    QPlainTextEdit* m_messageLog = nullptr;

    // Action manager
    ActionManager* m_actionManager = nullptr;

    // Core
    ComponentFactory* m_componentFactory = nullptr;
    BlockScene* m_blockScene = nullptr;
    BlockView*  m_blockView  = nullptr;

    // Library
    LibraryTreeModel* m_libraryModel = nullptr;
    LibraryTreeView*  m_libraryView  = nullptr;

    // Properties
    PropertyEditor* m_propertyEditor = nullptr;

    // Undo/Redo
    QUndoStack* m_undoStack = nullptr;

    // Dirty state
    bool m_isDirty = false;
    QString m_currentFilePath;

    void setDirty(bool dirty);
    bool maybeSave();
    void appendMessage(const QString& message);
};
