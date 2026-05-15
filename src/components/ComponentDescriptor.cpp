#include "ComponentDescriptor.h"

// ─── Helper ──────────────────────────────────────────────────

namespace {

PropertyDescriptor prop(const QString& id, const QString& name,
                        PropertyType type, const QVariant& def,
                        const QVariant& min, const QVariant& max,
                        const QString& unit)
{
    PropertyDescriptor p;
    p.id = id; p.displayName = name; p.type = type;
    p.defaultValue = def; p.minValue = min; p.maxValue = max;
    p.unit = unit;
    return p;
}

PortDescriptor inPort(const QString& id, const QString& name,
                      PortDataType dataType = PortDataType::Fluid)
{
    PortDescriptor p;
    p.id = id; p.displayName = name;
    p.direction = PortDirection::Input; p.dataType = dataType;
    return p;
}

PortDescriptor outPort(const QString& id, const QString& name,
                       PortDataType dataType = PortDataType::Fluid)
{
    PortDescriptor p;
    p.id = id; p.displayName = name;
    p.direction = PortDirection::Output; p.dataType = dataType;
    return p;
}

} // anonymous namespace

// ─── Pipes ──────────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createStraightPipe()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.straight";
    cd.displayName = "Straight Pipe";
    cd.category = "Pipes";
    cd.description = "A straight pipe segment with friction loss";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("length",    "Length",    PropertyType::Double, 1.0,   0.01, 100.0, "m"),
        prop("diameter",  "Diameter",  PropertyType::Double, 0.05,  0.001, 1.0,  "m"),
        prop("roughness", "Roughness", PropertyType::Double, 4.5e-5, 0.0, 0.01, "m"),
        prop("material",  "Material",  PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createElbow()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.elbow";
    cd.displayName = "Elbow";
    cd.category = "Pipes";
    cd.description = "A curved pipe bend";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("angle",    "Angle",    PropertyType::Double, 90.0, 1.0, 180.0, "deg"),
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 1.0,  "m"),
        prop("radius",   "Bend Radius", PropertyType::Double, 0.15, 0.01, 10.0, "m"),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 0.75, 0.0, 100.0, ""),
        prop("material", "Material", PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createTee()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.tee";
    cd.displayName = "Tee";
    cd.category = "Pipes";
    cd.description = "A three-way pipe junction";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = {
        outPort("outlet_a", "Outlet A"),
        outPort("outlet_b", "Outlet B")
    };
    cd.properties = {
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 1.0, "m"),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 1.5, 0.0, 100.0, ""),
        prop("material", "Material", PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createElbow45()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.elbow45";
    cd.displayName = "45-Degree Elbow";
    cd.category = "Pipes";
    cd.description = "A 45-degree curved pipe bend";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("angle",    "Angle",    PropertyType::Double, 45.0, 1.0, 180.0, "deg"),
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 1.0,  "m"),
        prop("radius",   "Bend Radius", PropertyType::Double, 0.15, 0.01, 10.0, "m"),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 0.35, 0.0, 100.0, ""),
        prop("material", "Material", PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createTeeStraight()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.teeStraight";
    cd.displayName = "Tee (Straight-Through)";
    cd.category = "Pipes";
    cd.description = "Tee with flow straight through (not branching)";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = {
        outPort("outlet_straight", "Straight Outlet"),
        outPort("outlet_branch", "Branch Outlet")
    };
    cd.properties = {
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 1.0, "m"),
        prop("lossCoefficient", "Loss Coefficient zeta",
             PropertyType::Double, 0.25, 0.0, 100.0, ""),
        prop("material", "Material", PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

// ─── Valves ─────────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createGateValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.gate";
    cd.displayName = "Gate Valve";
    cd.category = "Valves";
    cd.description = "A gate valve for on/off flow control";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",  "Diameter",  PropertyType::Double, 0.05,  0.001, 1.0, "m"),
        prop("cv",        "Cv (flow coeff)", PropertyType::Double, 10.0, 0.01, 1000.0, ""),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 0.17, 0.0, 100.0, ""),
        prop("actuation", "Actuation", PropertyType::String, "Manual", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createGlobeValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.globe";
    cd.displayName = "Globe Valve";
    cd.category = "Valves";
    cd.description = "A globe valve for flow regulation";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",  "Diameter",  PropertyType::Double, 0.05,  0.001, 1.0, "m"),
        prop("cv",        "Cv (flow coeff)", PropertyType::Double, 5.0, 0.01, 1000.0, ""),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 6.0, 0.0, 100.0, ""),
        prop("actuation", "Actuation", PropertyType::String, "Manual", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createBallValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.ball";
    cd.displayName = "Ball Valve";
    cd.category = "Valves";
    cd.description = "A quarter-turn ball valve";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",  "Diameter",  PropertyType::Double, 0.05,  0.001, 1.0, "m"),
        prop("cv",        "Cv (flow coeff)", PropertyType::Double, 8.0, 0.01, 1000.0, ""),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 0.5, 0.0, 100.0, ""),
        prop("actuation", "Actuation", PropertyType::String, "Manual", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createSolenoidValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.solenoid";
    cd.displayName = "Solenoid Valve";
    cd.category = "Valves";
    cd.description = "An electrically-actuated solenoid valve";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("voltage",      "Voltage",      PropertyType::Double, 24.0, 5.0, 220.0, "V"),
        prop("responseTime", "Response Time",PropertyType::Double, 0.015, 0.001, 0.5, "s"),
        prop("diameter",     "Diameter",     PropertyType::Double, 0.02, 0.001, 0.5, "m"),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 1.0, 0.0, 100.0, "")
    };
    return cd;
}

// ─── Pumps ──────────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createCentrifugalPump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.centrifugal";
    cd.displayName = "Centrifugal Pump";
    cd.category = "Pumps";
    cd.description = "A centrifugal pump for propellant feed";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("designHead", "Design Head", PropertyType::Double, 50.0, 1.0, 5000.0, "m"),
        prop("designFlow", "Design Flow", PropertyType::Double, 10.0, 0.01, 1000.0, "kg/s"),
        prop("rpm",        "RPM",         PropertyType::Double, 3000.0, 100.0, 100000.0, "rpm")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createPistonPump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.piston";
    cd.displayName = "Piston Pump";
    cd.category = "Pumps";
    cd.description = "A positive-displacement piston pump";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("stroke",   "Stroke",   PropertyType::Double, 0.1, 0.001, 1.0, "m"),
        prop("frequency","Frequency",PropertyType::Double, 10.0, 0.1, 100.0, "Hz"),
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 0.5, "m")
    };
    return cd;
}

// ─── Sensors ────────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createPressureSensor()
{
    ComponentDescriptor cd;
    cd.typeId = "sensor.pressure";
    cd.displayName = "Pressure Sensor";
    cd.category = "Sensors";
    cd.description = "A pressure transducer";
    cd.inputPorts  = { inPort("inlet", "Inlet", PortDataType::Fluid) };
    cd.outputPorts = { outPort("signal_out", "Signal Out", PortDataType::Signal) };
    cd.properties = {
        prop("rangeLow",  "Range Low",  PropertyType::Double, 0.0,    0.0,  1.0e9, "Pa"),
        prop("rangeHigh", "Range High", PropertyType::Double, 1.0e7,  0.0,  1.0e9, "Pa"),
        prop("accuracy",  "Accuracy",   PropertyType::Double, 0.5,    0.01, 100.0, "%")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFlowSensor()
{
    ComponentDescriptor cd;
    cd.typeId = "sensor.flow";
    cd.displayName = "Flow Sensor";
    cd.category = "Sensors";
    cd.description = "A flow rate sensor";
    cd.inputPorts  = { inPort("inlet", "Inlet", PortDataType::Fluid) };
    cd.outputPorts = { outPort("signal_out", "Signal Out", PortDataType::Signal) };
    cd.properties = {
        prop("rangeLow",  "Range Low",  PropertyType::Double, 0.0,   0.0,   10000.0, "kg/s"),
        prop("rangeHigh", "Range High", PropertyType::Double, 100.0, 0.0,   10000.0, "kg/s"),
        prop("accuracy",  "Accuracy",   PropertyType::Double, 0.5,   0.01,  100.0,   "%")
    };
    return cd;
}

// ─── Tanks ──────────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createStorageTank()
{
    ComponentDescriptor cd;
    cd.typeId = "tank.storage";
    cd.displayName = "Storage Tank";
    cd.category = "Tanks";
    cd.description = "A propellant storage tank";
    cd.inputPorts  = {};
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("volume",       "Volume",        PropertyType::Double, 10.0,  0.01, 10000.0, "m^3"),
        prop("designPressure","Design Pressure",PropertyType::Double, 5.0e5, 0.0, 1.0e8, "Pa"),
        prop("material",     "Material",      PropertyType::String, "Aluminum 2219", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createBufferTank()
{
    ComponentDescriptor cd;
    cd.typeId = "tank.buffer";
    cd.displayName = "Buffer Tank";
    cd.category = "Tanks";
    cd.description = "A buffer / accumulator tank";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("volume",        "Volume",         PropertyType::Double, 1.0,   0.01, 1000.0, "m^3"),
        prop("designPressure","Design Pressure",PropertyType::Double, 5.0e5, 0.0,  1.0e8,  "Pa"),
        prop("material",      "Material",       PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}
