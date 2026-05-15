#pragma once

#include <QString>
#include <QJsonObject>
#include <QVariant>
#include <QList>

// Lightweight component info that a plugin can provide without depending on components/ library
struct PluginComponentInfo {
    QString typeId;
    QString displayName;
    QString category;
    QString description;
    int inputPortCount = 1;
    int outputPortCount = 1;
};

// Standard plugin interface for liquid rocket piping design software.
// Plugins are loaded dynamically (.dll/.so) and registered via PluginManager.

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Unique plugin identifier (e.g. "com.example.turbopump")
    virtual QString pluginId() const = 0;

    // Human-readable name
    virtual QString pluginName() const = 0;

    // Version string (SemVer recommended)
    virtual QString pluginVersion() const = 0;

    // Short description
    virtual QString pluginDescription() const = 0;

    // Called after loading; return true on success
    virtual bool initialize() { return true; }

    // Called before unloading
    virtual void shutdown() {}

    // Execute a named action with JSON arguments, return JSON result.
    // This is the primary RPC mechanism for plugins.
    virtual QJsonObject execute(const QString& action, const QJsonObject& args) = 0;

    // List of actions this plugin supports (for UI discovery)
    virtual QStringList supportedActions() const = 0;

    // Optional: return list of component descriptors this plugin provides.
    // PluginManager will register these with ComponentFactory after loading.
    virtual QList<PluginComponentInfo> providedComponentInfo() const { return {}; }
};

// Factory function type — each plugin DLL exports one of these
using CreatePluginFunc = IPlugin* (*)();
using DestroyPluginFunc = void (*)(IPlugin*);

// Macros for plugin DLL entry points
#define PLUGIN_EXPORT_CREATE(PluginClass) \
    extern "C" __declspec(dllexport) IPlugin* createPlugin() { return new PluginClass(); }

#define PLUGIN_EXPORT_DESTROY \
    extern "C" __declspec(dllexport) void destroyPlugin(IPlugin* p) { delete p; }
