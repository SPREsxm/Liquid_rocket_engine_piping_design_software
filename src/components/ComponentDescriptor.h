#pragma once

#include "PortDescriptor.h"
#include "core/Types.h"

#include <QList>
#include <QString>

struct ComponentDescriptor {
    QString typeId;            // e.g. "pipe.straight"
    QString displayName;       // e.g. "Straight Pipe"
    QString category;          // e.g. "Pipes"
    QString description;       // tooltip text
    QString iconName;           // resource path, overridden by category-generated icon if empty

    QList<PortDescriptor>     inputPorts;
    QList<PortDescriptor>     outputPorts;
    QList<PropertyDescriptor> properties;

    bool isValid() const { return !typeId.isEmpty(); }

    // Factory methods for built-in types
    static ComponentDescriptor createStraightPipe();
    static ComponentDescriptor createElbow();
    static ComponentDescriptor createElbow45();
    static ComponentDescriptor createTee();
    static ComponentDescriptor createTeeStraight();

    static ComponentDescriptor createGateValve();
    static ComponentDescriptor createGlobeValve();
    static ComponentDescriptor createBallValve();
    static ComponentDescriptor createSolenoidValve();

    static ComponentDescriptor createCentrifugalPump();
    static ComponentDescriptor createPistonPump();

    static ComponentDescriptor createPressureSensor();
    static ComponentDescriptor createFlowSensor();

    static ComponentDescriptor createStorageTank();
    static ComponentDescriptor createBufferTank();
};
