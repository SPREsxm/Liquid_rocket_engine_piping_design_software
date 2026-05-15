#include "PluginManager.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

PluginManager& PluginManager::instance() {
    static PluginManager mgr;
    return mgr;
}

PluginManager::~PluginManager() {
    unloadAll();
}

int PluginManager::discoverPlugins(const QString& pluginDir) {
    QDir dir(pluginDir);
    if (!dir.exists()) return 0;

    int count = 0;
    const auto entries = dir.entryInfoList({"*.dll", "*.so"}, QDir::Files);
    for (const auto& fi : entries) {
        if (loadPlugin(fi.absoluteFilePath()))
            ++count;
    }
    return count;
}

IPlugin* PluginManager::loadPlugin(const QString& dllPath) {
    auto lib = std::make_unique<QLibrary>(dllPath);
    if (!lib->load()) {
        qWarning() << "Failed to load plugin:" << dllPath << lib->errorString();
        return nullptr;
    }

    auto createFn = reinterpret_cast<CreatePluginFunc>(
        lib->resolve("createPlugin"));
    if (!createFn) {
        qWarning() << "Plugin missing createPlugin symbol:" << dllPath;
        lib->unload();
        return nullptr;
    }

    auto destroyFn = reinterpret_cast<DestroyPluginFunc>(
        lib->resolve("destroyPlugin"));
    if (!destroyFn) {
        qWarning() << "Plugin missing destroyPlugin symbol:" << dllPath;
        lib->unload();
        return nullptr;
    }

    IPlugin* plugin = createFn();
    if (!plugin || !plugin->initialize()) {
        qWarning() << "Plugin initialization failed:" << dllPath;
        if (plugin) destroyFn(plugin);
        lib->unload();
        return nullptr;
    }

    PluginEntry entry;
    entry.library = lib.release();  // transfer ownership
    entry.instance = plugin;
    entry.destroy = destroyFn;

    m_plugins[plugin->pluginId()] = entry;
    qDebug() << "Loaded plugin:" << plugin->pluginName()
             << "v" << plugin->pluginVersion();

    registerPluginComponents(plugin->pluginId());
    return plugin;
}

bool PluginManager::unloadPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;

    it->instance->shutdown();
    it->destroy(it->instance);
    it->library->unload();
    delete it->library;
    m_plugins.erase(it);
    return true;
}

void PluginManager::unloadAll() {
    const auto ids = m_plugins.keys();
    for (const auto& id : ids)
        unloadPlugin(id);
}

QStringList PluginManager::loadedPluginIds() const {
    return m_plugins.keys();
}

IPlugin* PluginManager::plugin(const QString& pluginId) const {
    auto it = m_plugins.find(pluginId);
    return (it != m_plugins.end()) ? it->instance : nullptr;
}

int PluginManager::pluginCount() const {
    return m_plugins.size();
}

QJsonObject PluginManager::execute(const QString& pluginId,
                                    const QString& action,
                                    const QJsonObject& args) {
    auto* p = plugin(pluginId);
    if (!p) {
        QJsonObject err;
        err["error"] = QStringLiteral("Plugin not found: %1").arg(pluginId);
        return err;
    }
    return p->execute(action, args);
}

void PluginManager::registerPluginComponents(const QString& pluginId)
{
    auto* p = plugin(pluginId);
    if (!p) return;

    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return;

    it->componentInfos = p->providedComponentInfo();
    qDebug() << "Plugin" << pluginId << "provides"
             << it->componentInfos.size() << "component type(s)";
}

QList<PluginComponentInfo> PluginManager::allPluginComponentInfos() const
{
    QList<PluginComponentInfo> all;
    for (const auto& entry : m_plugins)
        all.append(entry.componentInfos);
    return all;
}

QList<PluginComponentInfo> PluginManager::pluginComponentInfos(const QString& pluginId) const
{
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end())
        return it->componentInfos;
    return {};
}
