#pragma once

#include "IPlugin.h"
#include <QString>
#include <QStringList>
#include <QHash>
#include <QLibrary>
#include <memory>

// Manages plugin discovery, loading, and lifecycle.
// Plugins are .dll/.so files placed in a "plugins/" directory
// next to the executable or in a system search path.

class PluginManager {
public:
    static PluginManager& instance();

    // Scan a directory for plugin DLLs and load all valid ones
    int discoverPlugins(const QString& pluginDir);

    // Load and register a specific plugin DLL
    IPlugin* loadPlugin(const QString& dllPath);

    // Unload a plugin by ID
    bool unloadPlugin(const QString& pluginId);

    // Unload all plugins
    void unloadAll();

    // Query
    QStringList loadedPluginIds() const;
    IPlugin* plugin(const QString& pluginId) const;
    int pluginCount() const;

    // Execute an action on a specific plugin
    QJsonObject execute(const QString& pluginId,
                        const QString& action,
                        const QJsonObject& args = {});

    // Register a plugin's components with ComponentFactory
    void registerPluginComponents(const QString& pluginId);

    // Get all component infos from all loaded plugins
    QList<PluginComponentInfo> allPluginComponentInfos() const;

    // Get component infos from a specific plugin
    QList<PluginComponentInfo> pluginComponentInfos(const QString& pluginId) const;

private:
    PluginManager() = default;
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    struct PluginEntry {
        std::shared_ptr<QLibrary> library;
        IPlugin* instance = nullptr;
        DestroyPluginFunc destroy = nullptr;
        QList<PluginComponentInfo> componentInfos;
    };

    QHash<QString, PluginEntry> m_plugins;
};
