#include "ActionManager.h"

#include <QAction>

ActionManager::ActionManager(QObject* parent)
    : QObject(parent)
{
    createAll();
}

QAction* ActionManager::action(ActionId id) const
{
    return m_actions.value(id, nullptr);
}

void ActionManager::retranslate()
{
    // File
    auto a = [this](ActionId id, const QString& text, const QString& tip) {
        if (auto* act = m_actions.value(id)) {
            act->setText(text);
            act->setStatusTip(tip);
        }
    };
    a(ActionId::New,    tr("&New"),    tr("Create a new project"));
    a(ActionId::Open,   tr("&Open..."), tr("Open an existing project"));
    a(ActionId::Save,   tr("&Save"),   tr("Save the current project"));
    a(ActionId::SaveAs, tr("Save &As..."), tr("Save project to a new file"));
    a(ActionId::Export_,tr("&Export..."), tr("Export to external format"));
    a(ActionId::Print,  tr("&Print..."),  tr("Print the schematic diagram"));
    // Edit
    a(ActionId::Undo, tr("&Undo"), tr("Undo last action"));
    a(ActionId::Redo, tr("&Redo"), tr("Redo last undone action"));
    a(ActionId::Cut,  tr("Cu&t"),  tr("Cut selected items"));
    a(ActionId::Copy, tr("&Copy"), tr("Copy selected items"));
    a(ActionId::Paste,tr("&Paste"), tr("Paste from clipboard"));
    a(ActionId::Delete, tr("&Delete"), tr("Delete selected items"));
    // View
    a(ActionId::ZoomIn,  tr("Zoom &In"),  tr("Zoom in"));
    a(ActionId::ZoomOut, tr("Zoom &Out"), tr("Zoom out"));
    a(ActionId::ZoomFit, tr("Zoom to &Fit"), tr("Fit diagram to window"));
    a(ActionId::ToggleLibrary, tr("Component &Library"), tr("Show or hide component library"));
    a(ActionId::ToggleProperties, tr("&Properties"), tr("Show or hide property editor"));
    a(ActionId::ToggleMessages, tr("&Messages"), tr("Show or hide message log"));
    a(ActionId::ToggleGrid, tr("&Grid"), tr("Show or hide grid"));
    // Tools
    a(ActionId::RunAnalysis, tr("&Run Analysis"), tr("Run piping network analysis"));
    a(ActionId::Validate,    tr("&Validate"),    tr("Validate the piping network"));
    a(ActionId::GenerateBom, tr("Generate &BOM..."), tr("Generate bill of materials"));
    a(ActionId::OptimizePipes, tr("&Optimize Pipes..."), tr("Optimize pipe schedules for minimum weight"));
    a(ActionId::Preferences, tr("&Preferences..."), tr("Open preferences"));
    // Help
    a(ActionId::About, tr("&About..."), tr("About this application"));
}

QAction* ActionManager::add(ActionId id, const QString& text,
                            const QKeySequence& shortcut, const QString& statusTip)
{
    auto* action = new QAction(text, this);
    action->setShortcut(shortcut);
    action->setStatusTip(statusTip);
    m_actions.insert(id, action);
    return action;
}

void ActionManager::createAll()
{
    // File
    add(ActionId::New,    tr("&New"),    QKeySequence::New,    tr("Create a new project"));
    add(ActionId::Open,   tr("&Open..."), QKeySequence::Open,  tr("Open an existing project"));
    add(ActionId::Save,   tr("&Save"),   QKeySequence::Save,   tr("Save the current project"));
    add(ActionId::SaveAs, tr("Save &As..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), tr("Save project to a new file"));
    add(ActionId::Export_,tr("&Export..."), QKeySequence(),    tr("Export to external format"));
    add(ActionId::Print,  tr("&Print..."),  QKeySequence::Print, tr("Print the schematic diagram"));

    // Edit
    add(ActionId::Undo, tr("&Undo"), QKeySequence::Undo, tr("Undo last action"));
    add(ActionId::Redo, tr("&Redo"), QKeySequence::Redo, tr("Redo last undone action"));
    add(ActionId::Cut,  tr("Cu&t"),  QKeySequence::Cut,  tr("Cut selected items"));
    add(ActionId::Copy, tr("&Copy"), QKeySequence::Copy, tr("Copy selected items"));
    add(ActionId::Paste,tr("&Paste"), QKeySequence::Paste, tr("Paste from clipboard"));
    add(ActionId::Delete, tr("&Delete"), QKeySequence::Delete, tr("Delete selected items"));

    // View
    add(ActionId::ZoomIn,  tr("Zoom &In"),  QKeySequence::ZoomIn,  tr("Zoom in"));
    add(ActionId::ZoomOut, tr("Zoom &Out"), QKeySequence::ZoomOut, tr("Zoom out"));
    add(ActionId::ZoomFit, tr("Zoom to &Fit"), QKeySequence(Qt::CTRL | Qt::Key_0), tr("Fit diagram to window"));
    auto* toggleLib = add(ActionId::ToggleLibrary, tr("Component &Library"), QKeySequence(), tr("Show or hide component library"));
    toggleLib->setCheckable(true);
    auto* toggleProp = add(ActionId::ToggleProperties, tr("&Properties"), QKeySequence(), tr("Show or hide property editor"));
    toggleProp->setCheckable(true);
    auto* toggleMsg = add(ActionId::ToggleMessages, tr("&Messages"), QKeySequence(), tr("Show or hide message log"));
    toggleMsg->setCheckable(true);
    auto* toggleGrid = add(ActionId::ToggleGrid, tr("&Grid"), QKeySequence(Qt::CTRL | Qt::Key_G), tr("Show or hide grid"));
    toggleGrid->setCheckable(true);
    toggleGrid->setChecked(true);

    // Tools
    add(ActionId::RunAnalysis, tr("&Run Analysis"), QKeySequence(Qt::Key_F5), tr("Run piping network analysis"));
    add(ActionId::Validate,    tr("&Validate"),    QKeySequence(Qt::CTRL | Qt::Key_F7), tr("Validate the piping network"));
    add(ActionId::GenerateBom, tr("Generate &BOM..."), QKeySequence(), tr("Generate bill of materials"));
    add(ActionId::OptimizePipes, tr("&Optimize Pipes..."), QKeySequence(), tr("Optimize pipe schedules for minimum weight"));
    add(ActionId::Preferences, tr("&Preferences..."), QKeySequence(), tr("Open preferences"));

    // Help
    add(ActionId::About, tr("&About..."), QKeySequence(), tr("About this application"));
}
