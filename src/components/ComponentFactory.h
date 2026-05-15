#pragma once

#include "ComponentDescriptor.h"
#include "ComponentInstance.h"
#include <QList>
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QStringList>

class ComponentFactory {
public:
    static ComponentFactory& instance();

    void registerComponent(const ComponentDescriptor& descriptor);

    // Register a component descriptor from a plugin (tracked separately)
    void registerPluginComponent(const ComponentDescriptor& descriptor);

    // Unregister all components from a given plugin
    void unregisterPluginComponents(const QStringList& typeIds);

    // Check if a component type is from a plugin
    bool isPluginComponent(const QString& typeId) const;

    QStringList allCategories() const;
    QList<ComponentDescriptor> componentsInCategory(const QString& category) const;
    const ComponentDescriptor* descriptorForType(const QString& typeId) const;
    ComponentInstance createInstance(const QString& typeId, const QPointF& pos = {}) const;

private:
    ComponentFactory();
    void registerBuiltins();

    QMap<QString, ComponentDescriptor> m_registry;
    QSet<QString> m_pluginTypeIds;
};
