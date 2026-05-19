#include "ComponentDescriptor.h"
#include "utils/PipeScheduleDatabase.h"

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

PortDescriptor bidirPort(const QString& id, const QString& name,
                         PortDataType dataType = PortDataType::Fluid)
{
    PortDescriptor p;
    p.id = id; p.displayName = name;
    p.direction = PortDirection::Bidirectional; p.dataType = dataType;
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
    cd.description = "A straight pipe segment with friction loss. Select NPS and schedule to auto-fill standard dimensions.";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };

    // Build NPS enum options
    PropertyDescriptor npsProp;
    npsProp.id = "nps";
    npsProp.displayName = "Nominal Size";
    npsProp.type = PropertyType::Enum;
    npsProp.defaultValue = "Custom";
    npsProp.enumOptions = QStringList{"Custom"};
    npsProp.enumOptions.append(PipeScheduleDatabase::instance().availableSizeNames());

    // Build schedule enum options
    PropertyDescriptor schProp;
    schProp.id = "schedule";
    schProp.displayName = "Schedule";
    schProp.type = PropertyType::Enum;
    schProp.defaultValue = "Custom";
    schProp.enumOptions = QStringList{"Custom", "5S", "10S", "40S", "80S", "STD", "XS", "XXS"};

    cd.properties = {
        prop("length",    "Length",    PropertyType::Double, 1.0,   0.01, 100.0, "m"),
        prop("diameter",  "Diameter",  PropertyType::Double, 0.05,  0.001, 1.0,  "m"),
        prop("roughness", "Roughness", PropertyType::Double, 4.5e-5, 0.0, 0.01, "m"),
        prop("material",  "Material",  PropertyType::String, "316L Stainless", {}, {}, ""),
        npsProp,
        schProp
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
    cd.inputPorts  = { bidirPort("inlet", "Inlet") };
    cd.outputPorts = {
        bidirPort("outlet_a", "Outlet A"),
        bidirPort("outlet_b", "Outlet B")
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
    cd.inputPorts  = { bidirPort("inlet", "Inlet") };
    cd.outputPorts = {
        bidirPort("outlet_straight", "Straight Outlet"),
        bidirPort("outlet_branch", "Branch Outlet")
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
        prop("rpm",        "RPM",         PropertyType::Double, 3000.0, 100.0, 100000.0, "rpm"),
        prop("npshr",      "NPSHr",       PropertyType::Double, 3.0,   0.1, 200.0,   "m")
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
        prop("diameter", "Diameter", PropertyType::Double, 0.05, 0.001, 0.5, "m"),
        prop("npshr",    "NPSHr",    PropertyType::Double, 2.0,   0.1, 200.0,   "m")
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
        prop("volume",          "Volume",                   PropertyType::Double, 10.0,   0.01, 10000.0, "m^3"),
        prop("designPressure",  "Design Pressure",          PropertyType::Double, 5.0e5,  0.0,  1.0e8,  "Pa"),
        prop("material",        "Material",                 PropertyType::String, "Aluminum 2219", {}, {}, ""),
        prop("storedMass",      "Stored Propellant Mass",   PropertyType::Double, 9128.0, 0.1,  1e6,    "kg"),
        prop("ullagePressure",  "Initial Ullage Pressure",  PropertyType::Double, 10.0e6, 1.0e5, 5.0e7, "Pa"),
        prop("ullageFraction",  "Ullage Volume Fraction",   PropertyType::Double, 0.2,    0.01, 0.9,   "")
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

// ─── Check Valve ───────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createCheckValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.check";
    cd.displayName = "Check Valve";
    cd.category = "Valves";
    cd.description = "A one-way check valve (swing type) — allows flow in one direction only";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",        "Diameter",        PropertyType::Double, 0.05,  0.001, 1.0,   "m"),
        prop("crackingPressure","Cracking Pressure",PropertyType::Double, 5000.0,0.0,   1.0e7, "Pa"),
        prop("lossCoefficient", "Loss Coefficient zeta", PropertyType::Double, 1.5, 0.0, 100.0, "")
    };
    return cd;
}

// ─── Butterfly Valve ────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createButterflyValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.butterfly";
    cd.displayName = "Butterfly Valve";
    cd.category = "Valves";
    cd.description = "A compact quarter-turn butterfly valve for flow control";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",       "Diameter",   PropertyType::Double, 0.1,   0.005, 2.0, "m"),
        prop("openingAngle",   "Opening",    PropertyType::Double, 90.0,  0.0,   90.0, "deg"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 0.8, 0.0, 100.0, "")
    };
    return cd;
}

// ─── Injector / Nozzle / Diffuser ───────────────────────────────

ComponentDescriptor ComponentDescriptor::createInjector()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.injector";
    cd.displayName = "Injector Plate";
    cd.category = "Combustion";
    cd.description = "A propellant injector — defines chamber entrance boundary for GasDynamics analysis";
    cd.inputPorts  = { inPort("fuelInlet", "Fuel Inlet"), inPort("oxidInlet", "Oxidizer Inlet") };
    cd.outputPorts = { outPort("chamber", "To Chamber") };
    cd.properties = {
        prop("injectionArea",    "Injection Area",   PropertyType::Double, 0.01,   1e-6,   1.0,    "m^2"),
        prop("injectionVelocity","Injection Velocity",PropertyType::Double, 30.0,   1.0,    500.0,  "m/s"),
        prop("mixtureRatio",     "O/F Ratio",        PropertyType::Double, 2.56,   0.5,    10.0,   ""),
        prop("chamberPressure", "Chamber Pressure",  PropertyType::Double, 7.0e6,  1.0e5,  3.0e7,  "Pa"),
        prop("diameter",        "Diameter",          PropertyType::Double, 0.1,    0.01,   2.0,    "m")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createNozzle()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.nozzle";
    cd.displayName = "Expansion Nozzle";
    cd.category = "Combustion";
    cd.description = "A supersonic de Laval nozzle — uses GasDynamics for thrust/area-ratio calculations";
    cd.inputPorts  = { inPort("inlet", "Chamber Inlet") };
    cd.outputPorts = { outPort("exit", "Nozzle Exit") };
    cd.properties = {
        prop("throatDiameter", "Throat Diameter", PropertyType::Double, 0.05,   0.001, 1.0,   "m"),
        prop("exitDiameter",   "Exit Diameter",   PropertyType::Double, 0.15,   0.005, 3.0,   "m"),
        prop("areaRatio",      "Area Ratio (Ae/At)",PropertyType::Double, 9.0,   2.0,   200.0, ""),
        prop("gamma",          "Specific Heat Ratio γ", PropertyType::Double, 1.2, 1.1,   1.67,  ""),
        prop("chamberTemp",    "Chamber Temperature",PropertyType::Double, 3500.0, 1500.0, 5000.0,"K"),
        prop("chamberPressure","Chamber Pressure",  PropertyType::Double, 7.0e6,  1.0e5, 3.0e7, "Pa"),
        prop("molarMass",      "Molar Mass",        PropertyType::Double, 22.3e-3, 2.0e-3, 0.2, "kg/mol")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createDiffuser()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.diffuser";
    cd.displayName = "Subsonic Diffuser";
    cd.category = "Combustion";
    cd.description = "A subsonic diffuser for pressure recovery in ramjet/scramjet inlet ducts";
    cd.inputPorts  = { inPort("inlet", "Supersonic Inlet") };
    cd.outputPorts = { outPort("exit", "Subsonic Exit") };
    cd.properties = {
        prop("inletDiameter",  "Inlet Diameter",   PropertyType::Double, 0.2,    0.01,  5.0,   "m"),
        prop("exitDiameter",   "Exit Diameter",    PropertyType::Double, 0.4,    0.02,  5.0,   "m"),
        prop("inletMach",      "Inlet Mach",       PropertyType::Double, 2.0,    0.3,   5.0,   ""),
        prop("gamma",          "Specific Heat Ratio γ", PropertyType::Double, 1.4, 1.1,  1.67,  ""),
        prop("efficiency",     "Diffuser Efficiency",PropertyType::Double, 0.85,  0.5,   0.98,  "")
    };
    return cd;
}

// ─── 2.1 推进剂供应系统 ──────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createTurbopump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.turbopump";
    cd.displayName = "Turbopump Assembly";
    cd.category = "Pumps";
    cd.description = "Coaxial turbopump with turbine, oxidizer pump, and fuel pump on a common shaft";
    cd.inputPorts  = { inPort("fuelInlet", "Fuel Inlet"), inPort("oxidInlet", "Oxidizer Inlet") };
    cd.outputPorts = { outPort("fuelOut", "Fuel Outlet"), outPort("oxidOut", "Oxidizer Outlet") };
    cd.properties = {
        prop("rpm",        "Shaft RPM",     PropertyType::Double, 18000.0, 1000.0, 100000.0, "rpm"),
        prop("shaftPower", "Shaft Power",   PropertyType::Double, 5.0e6,   1.0e3,  5.0e7,    "W"),
        prop("efficiency", "Overall Efficiency", PropertyType::Double, 0.72, 0.3, 0.95, ""),
        prop("weight",     "Dry Weight",    PropertyType::Double, 120.0,   1.0,    5000.0,   "kg")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createGasTurbine()
{
    ComponentDescriptor cd;
    cd.typeId = "turbine.gas";
    cd.displayName = "Gas Turbine";
    cd.category = "Pumps";
    cd.description = "A hot-gas turbine stage for driving the turbopump assembly";
    cd.inputPorts  = { inPort("inlet", "Hot Gas Inlet") };
    cd.outputPorts = { outPort("outlet", "Turbine Exhaust") };
    cd.properties = {
        prop("inletPressure", "Inlet Pressure",  PropertyType::Double, 6.7e6,  1.0e5, 3.0e7, "Pa"),
        prop("inletTemp",     "Inlet Temperature",PropertyType::Double, 900.0,  400.0, 1500.0,"K"),
        prop("power",         "Output Power",    PropertyType::Double, 5.0e6,  1.0e3, 5.0e7, "W"),
        prop("rpm",           "Turbine RPM",     PropertyType::Double, 18000.0, 1000.0, 100000.0, "rpm"),
        prop("efficiency",    "Isentropic Efficiency", PropertyType::Double, 0.78, 0.4, 0.95, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createOxidizerPump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.oxidizer";
    cd.displayName = "LOX Pump";
    cd.category = "Pumps";
    cd.description = "Centrifugal liquid-oxygen pump, typically operating at cryogenic temperatures";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("designHead",  "Design Head",  PropertyType::Double, 800.0,  10.0, 5000.0, "m"),
        prop("designFlow",  "Design Flow",  PropertyType::Double, 150.0,  0.1,  1000.0, "kg/s"),
        prop("rpm",         "Shaft RPM",    PropertyType::Double, 18000.0,1000.0,100000.0,"rpm"),
        prop("npshRequired","NPSH Required",PropertyType::Double, 15.0,   0.1,  200.0,  "m"),
        prop("inletTemp",   "Inlet Temperature",PropertyType::Double, 90.0, 55.0, 300.0,"K")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFuelPump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.fuel";
    cd.displayName = "Fuel Pump";
    cd.category = "Pumps";
    cd.description = "Centrifugal kerosene (RP-1) fuel pump";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("designHead",  "Design Head",  PropertyType::Double, 500.0,  10.0, 5000.0, "m"),
        prop("designFlow",  "Design Flow",  PropertyType::Double, 60.0,   0.1,  1000.0, "kg/s"),
        prop("rpm",         "Shaft RPM",    PropertyType::Double, 18000.0,1000.0,100000.0,"rpm"),
        prop("npshRequired","NPSH Required",PropertyType::Double, 10.0,   0.1,  200.0,  "m")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createBoostPump()
{
    ComponentDescriptor cd;
    cd.typeId = "pump.boost";
    cd.displayName = "Boost Pre-Pump";
    cd.category = "Pumps";
    cd.description = "Axial-flow inducer / boost pump for improving main pump cavitation margin";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("designHead", "Design Head", PropertyType::Double, 25.0,  1.0, 200.0,  "m"),
        prop("designFlow", "Design Flow", PropertyType::Double, 150.0, 0.1, 1000.0, "kg/s"),
        prop("rpm",        "Shaft RPM",   PropertyType::Double, 6000.0, 500.0, 50000.0,"rpm"),
        prop("npshr",      "NPSHr",       PropertyType::Double, 1.5,   0.1, 100.0,   "m")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createMainValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.main";
    cd.displayName = "Main Propellant Valve";
    cd.category = "Valves";
    cd.description = "Primary oxidizer or fuel main valve controlling propellant flow to the thrust chamber";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.08,  0.005, 0.5,  "m"),
        prop("actuation",    "Actuation Type", PropertyType::String, "Pneumatic", {}, {}, ""),
        prop("responseTime", "Response Time",  PropertyType::Double, 0.05,  0.005, 2.0,  "s"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 0.25, 0.0, 100.0, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createSecondaryValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.secondary";
    cd.displayName = "Secondary Valve";
    cd.category = "Valves";
    cd.description = "Secondary propellant valve controlling flow to the gas generator";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.025, 0.002, 0.2,  "m"),
        prop("actuation",    "Actuation Type", PropertyType::String, "Pneumatic", {}, {}, ""),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 0.5, 0.0, 100.0, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFillValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.fill";
    cd.displayName = "Fill/Drain Valve";
    cd.category = "Valves";
    cd.description = "Fill-and-drain valve for propellant loading and system draining operations";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.04,  0.005, 0.3,  "m"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 0.3, 0.0, 100.0, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createVentValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.vent";
    cd.displayName = "Vent Valve";
    cd.category = "Valves";
    cd.description = "Vent and relief valve for tank pressurization control and gas venting";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.02,  0.002, 0.15, "m"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 2.0, 0.0, 100.0, "")
    };
    return cd;
}

// ─── 2.2 增压系统 ──────────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createHighPressureBottle()
{
    ComponentDescriptor cd;
    cd.typeId = "tank.highPressure";
    cd.displayName = "HP Gas Bottle";
    cd.category = "Tanks";
    cd.description = "High-pressure gas storage vessel for propellant tank pressurization (He or N2)";
    cd.inputPorts  = {};
    cd.outputPorts = { outPort("outlet", "Gas Outlet") };
    cd.properties = {
        prop("volume",        "Volume",         PropertyType::Double, 0.05,  0.001, 10.0,  "m^3"),
        prop("designPressure","Design Pressure",PropertyType::Double, 3.0e7, 5.0e5, 7.0e7, "Pa"),
        prop("gasType",       "Gas Type",       PropertyType::String, "Helium", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createPressureRegulator()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.regulator";
    cd.displayName = "Pressure Regulator";
    cd.category = "Valves";
    cd.description = "Dome-loaded or spring-loaded pressure regulator reducing HP gas to tank operating pressure";
    cd.inputPorts  = { inPort("inlet", "HP Inlet") };
    cd.outputPorts = { outPort("outlet", "Regulated Outlet") };
    cd.properties = {
        prop("setPressure", "Set Pressure",  PropertyType::Double, 5.0e5, 1.0e4, 2.0e7, "Pa"),
        prop("diameter",    "Orifice Diameter",PropertyType::Double, 0.01, 0.001, 0.1, "m"),
        prop("accuracy",    "Regulation Accuracy",PropertyType::Double, 2.0, 0.1, 20.0, "%")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createSelectorValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.selector";
    cd.displayName = "Selector Valve (3-Way)";
    cd.category = "Valves";
    cd.description = "Two-position three-way solenoid valve for switching gas supply paths";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outletA", "Port A"), outPort("outletB", "Port B") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.015, 0.002, 0.1,  "m"),
        prop("position",     "Active Position",PropertyType::String, "A", {}, {}, ""),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 1.0, 0.0, 100.0, "")
    };
    return cd;
}

// ─── 2.3 输送管路系统 ──────────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createCrossFitting()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.cross";
    cd.displayName = "Cross Fitting";
    cd.category = "Pipes";
    cd.description = "A four-way pipe cross connector for splitting or combining flow";
    cd.inputPorts  = { bidirPort("inlet", "Inlet") };
    cd.outputPorts = {
        bidirPort("outA", "Branch A"),
        bidirPort("outB", "Branch B"),
        bidirPort("outC", "Branch C")
    };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.05,  0.001, 1.0,   "m"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 2.2, 0.0, 100.0, ""),
        prop("material",     "Material",      PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFilter()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.filter";
    cd.displayName = "Filter";
    cd.category = "Pipes";
    cd.description = "In-line propellant filter protecting downstream precision components from particulate contamination";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Diameter",      PropertyType::Double, 0.05,  0.005, 0.5,   "m"),
        prop("meshSize",     "Mesh Size",     PropertyType::Double, 40.0,  1.0,   500.0,  "um"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 3.0, 0.0, 100.0, ""),
        prop("material",     "Material",      PropertyType::String, "316L Stainless", {}, {}, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createAccumulator()
{
    ComponentDescriptor cd;
    cd.typeId = "tank.accumulator";
    cd.displayName = "Pogo Accumulator";
    cd.category = "Tanks";
    cd.description = "Gas-charged accumulator installed at pump inlet to suppress pogo oscillations in the propellant feed system";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("volume",        "Total Volume",    PropertyType::Double, 0.02,  0.001, 1.0,   "m^3"),
        prop("gasVolume",     "Gas Charge Volume",PropertyType::Double, 0.015, 0.001, 0.5,  "m^3"),
        prop("designPressure","Design Pressure", PropertyType::Double, 1.0e6, 1.0e4, 2.0e7, "Pa")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createCompensator()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.compensator";
    cd.displayName = "Bellows Compensator";
    cd.category = "Pipes";
    cd.description = "Bellows expansion joint compensating thermal expansion, misalignment, and vibration in propellant ducts";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",     "Nominal Diameter",PropertyType::Double, 0.08,  0.01,  1.0,   "m"),
        prop("axialStroke",  "Axial Stroke",    PropertyType::Double, 0.015, 0.001, 0.1,  "m"),
        prop("springRate",   "Spring Rate",     PropertyType::Double, 5000.0,100.0, 50000.0,"N/m")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createTemperatureSensor()
{
    ComponentDescriptor cd;
    cd.typeId = "sensor.temperature";
    cd.displayName = "Temperature Sensor";
    cd.category = "Sensors";
    cd.description = "Resistance temperature detector (RTD) or thermocouple for cryogenic or high-temperature propellant monitoring";
    cd.inputPorts  = { inPort("inlet", "Inlet", PortDataType::Fluid) };
    cd.outputPorts = { outPort("signal_out", "Signal Out", PortDataType::Signal) };
    cd.properties = {
        prop("rangeLow",  "Range Low",  PropertyType::Double, 20.0,  1.0,   5000.0, "K"),
        prop("rangeHigh", "Range High", PropertyType::Double, 500.0, 10.0,  5000.0, "K"),
        prop("accuracy",  "Accuracy",   PropertyType::Double, 0.5,   0.01,  10.0,   "%")
    };
    return cd;
}

// ─── 2.4 发动机本体供应系统 ───────────────────────────────────

ComponentDescriptor ComponentDescriptor::createGasGenerator()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.gasGenerator";
    cd.displayName = "Gas Generator";
    cd.category = "Combustion";
    cd.description = "Fuel-rich or oxidizer-rich pre-burner producing hot gas to drive the turbine";
    cd.inputPorts  = { inPort("fuelInlet", "Fuel Inlet"), inPort("oxidInlet", "Oxidizer Inlet") };
    cd.outputPorts = { outPort("hotGas", "Hot Gas Outlet") };
    cd.properties = {
        prop("mixtureRatio",    "O/F Mixture Ratio", PropertyType::Double, 0.4,   0.1,   5.0,   ""),
        prop("chamberPressure", "Chamber Pressure",  PropertyType::Double, 6.7e6, 1.0e5, 3.0e7, "Pa"),
        prop("chamberTemp",     "Chamber Temperature",PropertyType::Double, 900.0, 400.0, 2000.0,"K"),
        prop("massFlow",        "Hot Gas Mass Flow", PropertyType::Double, 5.0,   0.1,   50.0,   "kg/s")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createIgniter()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.igniter";
    cd.displayName = "Ignition Device";
    cd.category = "Combustion";
    cd.description = "Pyrotechnic or spark-torch igniter initiating combustion in the gas generator or thrust chamber";
    cd.inputPorts  = {};
    cd.outputPorts = { outPort("flame", "Flame / Ignition Output") };
    cd.properties = {
        prop("ignitionEnergy", "Ignition Energy", PropertyType::Double, 5.0,  0.1,  500.0, "J"),
        prop("delayTime",      "Ignition Delay",  PropertyType::Double, 0.02, 0.001, 1.0,  "s")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createHeatExchanger()
{
    ComponentDescriptor cd;
    cd.typeId = "heat.exchanger";
    cd.displayName = "Heat Exchanger";
    cd.category = "Pipes";
    cd.description = "Shell-and-tube or plate-fin heat exchanger for propellant thermal conditioning";
    cd.inputPorts  = { inPort("hotInlet", "Hot Side Inlet"), inPort("coldInlet", "Cold Side Inlet") };
    cd.outputPorts = { outPort("hotOutlet", "Hot Side Outlet"), outPort("coldOutlet", "Cold Side Outlet") };
    cd.properties = {
        prop("effectiveness", "Effectiveness (epsilon)", PropertyType::Double, 0.75, 0.1,  0.98,  ""),
        prop("pressureLoss",  "Pressure Loss",     PropertyType::Double, 5000.0, 0.0, 1.0e6,"Pa"),
        prop("heatLoad",      "Heat Load",         PropertyType::Double, 1.0e5, 1.0e3, 1.0e8,"W")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFlowRegulator()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.flowRegulator";
    cd.displayName = "Flow Regulator";
    cd.category = "Valves";
    cd.description = "Precision flow-regulating valve maintaining a constant mass flow rate under varying pressure conditions";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("setpointFlow",  "Setpoint Flow",  PropertyType::Double, 10.0,  0.01,  500.0, "kg/s"),
        prop("diameter",      "Orifice Diameter",PropertyType::Double, 0.02, 0.002, 0.2,   "m"),
        prop("accuracy",      "Regulation Accuracy",PropertyType::Double, 2.0, 0.1, 10.0, "%")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createThrottleValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.throttle";
    cd.displayName = "Throttle Valve";
    cd.category = "Valves";
    cd.description = "Variable-position throttle valve for engine thrust modulation and mixture ratio trimming";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",      "Diameter",       PropertyType::Double, 0.05,  0.005, 0.3,   "m"),
        prop("throttleRange", "Throttle Range", PropertyType::Double, 50.0,  10.0,  100.0, "%"),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 4.0, 0.0, 100.0, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createCavitatingVenturi()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.cavitatingVenturi";
    cd.displayName = "Cavitating Venturi";
    cd.category = "Pipes";
    cd.description = "Cavitating venturi for passive flow regulation — flow becomes independent of downstream pressure when cavitating";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("throatDiameter", "Throat Diameter",   PropertyType::Double, 0.01,  0.001, 0.2,   "m"),
        prop("dischargeCoeff", "Discharge Coefficient Cd",PropertyType::Double, 0.85, 0.5, 0.98, ""),
        prop("cavitationMargin","Cavitation Margin",PropertyType::Double, 0.15,  0.05,  0.5,   ""),
        prop("inletDiameter", "Inlet Diameter",     PropertyType::Double, 0.025, 0.005, 0.3,  "m")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createThrottleOrifice()
{
    ComponentDescriptor cd;
    cd.typeId = "pipe.throttleOrifice";
    cd.displayName = "Throttle Orifice";
    cd.category = "Pipes";
    cd.description = "Fixed-area throttle orifice for trimming flow resistance in specific branches of the feed system";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("orificeDiameter","Orifice Diameter",PropertyType::Double, 0.008, 0.001, 0.2,  "m"),
        prop("pipeDiameter",   "Pipe Diameter",   PropertyType::Double, 0.05,  0.005, 0.5,  "m"),
        prop("dischargeCoeff", "Discharge Coefficient Cd",PropertyType::Double, 0.62, 0.4, 0.95, "")
    };
    return cd;
}

// ─── 2.5 安全与辅助系统 ───────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createPurgeValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.purge";
    cd.displayName = "Purge Valve";
    cd.category = "Valves";
    cd.description = "Nitrogen purge valve for clearing residual propellant from lines after engine shutdown";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Outlet") };
    cd.properties = {
        prop("diameter",      "Diameter",      PropertyType::Double, 0.015, 0.002, 0.1,  "m"),
        prop("purgeMedium",   "Purge Medium",  PropertyType::String, "GN2", {}, {}, ""),
        prop("lossCoefficient","Loss Coefficient zeta", PropertyType::Double, 1.2, 0.0, 100.0, "")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createReliefValve()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.relief";
    cd.displayName = "Safety Relief Valve";
    cd.category = "Valves";
    cd.description = "Spring-loaded safety relief valve protecting the system from over-pressurization";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Vent Outlet") };
    cd.properties = {
        prop("setPressure",      "Set Pressure",       PropertyType::Double, 1.1e7, 1.0e5, 5.0e7, "Pa"),
        prop("diameter",         "Orifice Diameter",   PropertyType::Double, 0.025, 0.005, 0.2,  "m"),
        prop("certifiedCapacity","Certified Capacity", PropertyType::Double, 2.0,   0.1,   50.0,  "kg/s")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createBurstDisk()
{
    ComponentDescriptor cd;
    cd.typeId = "valve.burstDisk";
    cd.displayName = "Burst Disk";
    cd.category = "Valves";
    cd.description = "Single-use rupture disc providing ultimate over-pressure protection with zero leakage";
    cd.inputPorts  = { inPort("inlet", "Inlet") };
    cd.outputPorts = { outPort("outlet", "Vent Outlet") };
    cd.properties = {
        prop("burstPressure", "Burst Pressure",  PropertyType::Double, 1.5e7, 1.0e5, 1.0e8, "Pa"),
        prop("diameter",      "Disc Diameter",   PropertyType::Double, 0.05,  0.01,  0.5,   "m"),
        prop("tolerance",     "Burst Tolerance", PropertyType::Double, 5.0,   0.5,   20.0,  "%")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createFlowMeter()
{
    ComponentDescriptor cd;
    cd.typeId = "sensor.flowMeter";
    cd.displayName = "Flow Meter";
    cd.category = "Sensors";
    cd.description = "Turbine or Coriolis-type flow meter for precision propellant mass flow measurement";
    cd.inputPorts  = { inPort("inlet", "Inlet", PortDataType::Fluid) };
    cd.outputPorts = { outPort("signal_out", "Signal Out", PortDataType::Signal) };
    cd.properties = {
        prop("rangeLow",  "Range Low",  PropertyType::Double, 0.0,   0.0,   10000.0, "kg/s"),
        prop("rangeHigh", "Range High", PropertyType::Double, 100.0, 0.0,   10000.0, "kg/s"),
        prop("accuracy",  "Accuracy",   PropertyType::Double, 0.25,  0.01,  10.0,    "%"),
        prop("diameter",  "Pipe Diameter",PropertyType::Double, 0.05, 0.005, 0.5,    "m")
    };
    return cd;
}

// ─── 2.6 燃烧室出口边界 ───────────────────────────────────────

ComponentDescriptor ComponentDescriptor::createFuelOutlet()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.fuelOutlet";
    cd.displayName = "Fuel Outlet";
    cd.category = "Combustion";
    cd.description = "Fuel outlet boundary — represents where fuel exits "
                     "the piping system into the combustion chamber at a known back-pressure";
    cd.inputPorts  = { inPort("inlet", "Fuel Inlet") };
    cd.outputPorts = {};
    cd.properties = {
        prop("outletFlowRate",            "Outlet Flow Rate",            PropertyType::Double, 30.0,  0.01,  10000.0, "kg/s"),
        prop("outletEnvironmentPressure", "Outlet Environment Pressure", PropertyType::Double, 7.0e6, 1.0e5, 3.0e7,  "Pa")
    };
    return cd;
}

ComponentDescriptor ComponentDescriptor::createOxidizerOutlet()
{
    ComponentDescriptor cd;
    cd.typeId = "chamber.oxidizerOutlet";
    cd.displayName = "Oxidizer Outlet";
    cd.category = "Combustion";
    cd.description = "Oxidizer outlet boundary — represents where oxidizer exits "
                     "the piping system into the combustion chamber at a known back-pressure";
    cd.inputPorts  = { inPort("inlet", "Oxidizer Inlet") };
    cd.outputPorts = {};
    cd.properties = {
        prop("outletFlowRate",            "Outlet Flow Rate",            PropertyType::Double, 80.0,  0.01,  10000.0, "kg/s"),
        prop("outletEnvironmentPressure", "Outlet Environment Pressure", PropertyType::Double, 7.0e6, 1.0e5, 3.0e7,  "Pa")
    };
    return cd;
}
