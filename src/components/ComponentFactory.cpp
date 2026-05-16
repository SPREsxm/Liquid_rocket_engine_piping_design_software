#include "ComponentFactory.h"
#include "ComponentInstance.h"

ComponentFactory& ComponentFactory::instance()
{
    static ComponentFactory factory;
    return factory;
}

ComponentFactory::ComponentFactory()
{
    registerBuiltins();
}

void ComponentFactory::registerComponent(const ComponentDescriptor& descriptor)
{
    if (descriptor.isValid()) {
        m_registry[descriptor.typeId] = descriptor;
    }
}

QStringList ComponentFactory::allCategories() const
{
    QSet<QString> cats;
    for (const auto& desc : m_registry) {
        cats.insert(desc.category);
    }
    QStringList list(cats.begin(), cats.end());
    list.sort();
    return list;
}

QList<ComponentDescriptor> ComponentFactory::componentsInCategory(const QString& category) const
{
    QList<ComponentDescriptor> result;
    for (const auto& desc : m_registry) {
        if (desc.category == category)
            result.append(desc);
    }
    return result;
}

const ComponentDescriptor* ComponentFactory::descriptorForType(const QString& typeId) const
{
    auto it = m_registry.find(typeId);
    if (it != m_registry.end()) return &it.value();
    return nullptr;
}

ComponentInstance ComponentFactory::createInstance(const QString& typeId, const QPointF& pos) const
{
    const auto* desc = descriptorForType(typeId);
    if (!desc) return {};
    return ComponentInstance::create(*desc, pos);
}

void ComponentFactory::registerPluginComponent(const ComponentDescriptor& descriptor)
{
    if (descriptor.isValid()) {
        m_registry[descriptor.typeId] = descriptor;
        m_pluginTypeIds.insert(descriptor.typeId);
    }
}

void ComponentFactory::unregisterPluginComponents(const QStringList& typeIds)
{
    for (const auto& tid : typeIds) {
        m_registry.remove(tid);
        m_pluginTypeIds.remove(tid);
    }
}

bool ComponentFactory::isPluginComponent(const QString& typeId) const
{
    return m_pluginTypeIds.contains(typeId);
}

void ComponentFactory::registerBuiltins()
{
    registerComponent(ComponentDescriptor::createStraightPipe());
    registerComponent(ComponentDescriptor::createElbow());
    registerComponent(ComponentDescriptor::createElbow45());
    registerComponent(ComponentDescriptor::createTee());
    registerComponent(ComponentDescriptor::createTeeStraight());
    registerComponent(ComponentDescriptor::createGateValve());
    registerComponent(ComponentDescriptor::createGlobeValve());
    registerComponent(ComponentDescriptor::createBallValve());
    registerComponent(ComponentDescriptor::createSolenoidValve());
    registerComponent(ComponentDescriptor::createCheckValve());
    registerComponent(ComponentDescriptor::createButterflyValve());
    registerComponent(ComponentDescriptor::createCentrifugalPump());
    registerComponent(ComponentDescriptor::createPistonPump());
    registerComponent(ComponentDescriptor::createPressureSensor());
    registerComponent(ComponentDescriptor::createFlowSensor());
    registerComponent(ComponentDescriptor::createStorageTank());
    registerComponent(ComponentDescriptor::createBufferTank());
    registerComponent(ComponentDescriptor::createInjector());
    registerComponent(ComponentDescriptor::createNozzle());
    registerComponent(ComponentDescriptor::createDiffuser());

    // ── 2.1 推进剂供应系统 ──
    registerComponent(ComponentDescriptor::createTurbopump());
    registerComponent(ComponentDescriptor::createGasTurbine());
    registerComponent(ComponentDescriptor::createOxidizerPump());
    registerComponent(ComponentDescriptor::createFuelPump());
    registerComponent(ComponentDescriptor::createBoostPump());
    registerComponent(ComponentDescriptor::createMainValve());
    registerComponent(ComponentDescriptor::createSecondaryValve());
    registerComponent(ComponentDescriptor::createFillValve());
    registerComponent(ComponentDescriptor::createVentValve());

    // ── 2.2 增压系统 ──
    registerComponent(ComponentDescriptor::createHighPressureBottle());
    registerComponent(ComponentDescriptor::createPressureRegulator());
    registerComponent(ComponentDescriptor::createSelectorValve());

    // ── 2.3 输送管路系统 ──
    registerComponent(ComponentDescriptor::createCrossFitting());
    registerComponent(ComponentDescriptor::createFilter());
    registerComponent(ComponentDescriptor::createAccumulator());
    registerComponent(ComponentDescriptor::createCompensator());
    registerComponent(ComponentDescriptor::createTemperatureSensor());

    // ── 2.4 发动机本体供应系统 ──
    registerComponent(ComponentDescriptor::createGasGenerator());
    registerComponent(ComponentDescriptor::createIgniter());
    registerComponent(ComponentDescriptor::createHeatExchanger());
    registerComponent(ComponentDescriptor::createFlowRegulator());
    registerComponent(ComponentDescriptor::createThrottleValve());
    registerComponent(ComponentDescriptor::createCavitatingVenturi());
    registerComponent(ComponentDescriptor::createThrottleOrifice());

    // ── 2.5 安全与辅助系统 ──
    registerComponent(ComponentDescriptor::createPurgeValve());
    registerComponent(ComponentDescriptor::createReliefValve());
    registerComponent(ComponentDescriptor::createBurstDisk());
    registerComponent(ComponentDescriptor::createFlowMeter());
}
