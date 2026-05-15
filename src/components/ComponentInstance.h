#pragma once

#include "ComponentDescriptor.h"
#include "PortDescriptor.h"

#include <QPointF>
#include <QUuid>
#include <QVariant>

struct ComponentInstance {
    QUuid uuid;
    QString typeId;
    QPointF position;
    QMap<QString, QVariant> propertyValues;
    QString customLabel;

    static ComponentInstance create(const ComponentDescriptor& desc,
                                    const QPointF& pos = {},
                                    const QUuid& forcedUuid = {}) {
        ComponentInstance inst;
        inst.uuid = forcedUuid.isNull() ? QUuid::createUuid() : forcedUuid;
        inst.typeId = desc.typeId;
        inst.position = pos;
        for (const auto& prop : desc.properties) {
            inst.propertyValues[prop.id] = prop.defaultValue;
        }
        inst.customLabel = desc.displayName;
        return inst;
    }
};
