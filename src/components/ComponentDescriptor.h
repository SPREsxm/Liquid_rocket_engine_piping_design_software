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
    static ComponentDescriptor createCheckValve();
    static ComponentDescriptor createButterflyValve();

    static ComponentDescriptor createCentrifugalPump();
    static ComponentDescriptor createPistonPump();

    static ComponentDescriptor createPressureSensor();
    static ComponentDescriptor createFlowSensor();

    static ComponentDescriptor createStorageTank();
    static ComponentDescriptor createBufferTank();

    static ComponentDescriptor createInjector();
    static ComponentDescriptor createNozzle();
    static ComponentDescriptor createDiffuser();

    // ── 2.1 推进剂供应系统 ──
    static ComponentDescriptor createTurbopump();
    static ComponentDescriptor createGasTurbine();
    static ComponentDescriptor createOxidizerPump();
    static ComponentDescriptor createFuelPump();
    static ComponentDescriptor createBoostPump();
    static ComponentDescriptor createMainValve();
    static ComponentDescriptor createSecondaryValve();
    static ComponentDescriptor createFillValve();
    static ComponentDescriptor createVentValve();

    // ── 2.2 增压系统 ──
    static ComponentDescriptor createHighPressureBottle();
    static ComponentDescriptor createPressureRegulator();
    static ComponentDescriptor createSelectorValve();

    // ── 2.3 输送管路系统 ──
    static ComponentDescriptor createCrossFitting();
    static ComponentDescriptor createFilter();
    static ComponentDescriptor createAccumulator();
    static ComponentDescriptor createCompensator();
    static ComponentDescriptor createTemperatureSensor();

    // ── 2.4 发动机本体供应系统 ──
    static ComponentDescriptor createGasGenerator();
    static ComponentDescriptor createIgniter();
    static ComponentDescriptor createHeatExchanger();
    static ComponentDescriptor createFlowRegulator();
    static ComponentDescriptor createThrottleValve();
    static ComponentDescriptor createCavitatingVenturi();
    static ComponentDescriptor createThrottleOrifice();

    // ── 2.5 安全与辅助系统 ──
    static ComponentDescriptor createPurgeValve();
    static ComponentDescriptor createReliefValve();
    static ComponentDescriptor createBurstDisk();
    static ComponentDescriptor createFlowMeter();
};
