#pragma once

#include <QObject>
#include <QHash>
#include <QKeySequence>

class QAction;

enum class ActionId {
    // File
    New,
    Open,
    Save,
    SaveAs,
    Export_,
    // Edit
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Delete,
    // View
    ZoomIn,
    ZoomOut,
    ZoomFit,
    ToggleLibrary,
    ToggleProperties,
    ToggleMessages,
    ToggleGrid,
    // Tools
    RunAnalysis,
    Validate,
    Preferences,
    // Help
    About,
};

class ActionManager : public QObject {
    Q_OBJECT
public:
    explicit ActionManager(QObject* parent = nullptr);

    QAction* action(ActionId id) const;

private:
    void createAll();
    QAction* add(ActionId id, const QString& text,
                 const QKeySequence& shortcut, const QString& statusTip);

    QHash<ActionId, QAction*> m_actions;
};
