#include <catch2/catch_all.hpp>
#include "components/ComponentFactory.h"
#include "components/ComponentInstance.h"

TEST_CASE("ComponentFactory has six categories") {
    auto cats = ComponentFactory::instance().allCategories();
    REQUIRE(cats.size() == 6);
    REQUIRE(cats.contains("Pipes"));
    REQUIRE(cats.contains("Valves"));
    REQUIRE(cats.contains("Pumps"));
    REQUIRE(cats.contains("Sensors"));
    REQUIRE(cats.contains("Tanks"));
    REQUIRE(cats.contains("Combustion"));
}

TEST_CASE("ComponentFactory returns 5 pipes") {
    auto pipes = ComponentFactory::instance().componentsInCategory("Pipes");
    REQUIRE(pipes.size() == 5);
}

TEST_CASE("ComponentFactory returns 6 valves") {
    auto valves = ComponentFactory::instance().componentsInCategory("Valves");
    REQUIRE(valves.size() == 6);
}

TEST_CASE("ComponentFactory returns 2 pumps") {
    auto pumps = ComponentFactory::instance().componentsInCategory("Pumps");
    REQUIRE(pumps.size() == 2);
}

TEST_CASE("ComponentFactory returns 2 sensors") {
    auto sensors = ComponentFactory::instance().componentsInCategory("Sensors");
    REQUIRE(sensors.size() == 2);
}

TEST_CASE("ComponentFactory returns 2 tanks") {
    auto tanks = ComponentFactory::instance().componentsInCategory("Tanks");
    REQUIRE(tanks.size() == 2);
}

TEST_CASE("descriptorForType returns correct descriptor") {
    const auto* desc = ComponentFactory::instance().descriptorForType("pipe.straight");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->typeId == "pipe.straight");
    REQUIRE(desc->displayName == "Straight Pipe");
}

TEST_CASE("descriptorForType returns nullptr for unknown type") {
    const auto* desc = ComponentFactory::instance().descriptorForType("not.exist");
    REQUIRE(desc == nullptr);
}

TEST_CASE("createInstance produces valid instance") {
    auto inst = ComponentFactory::instance().createInstance("pipe.straight");
    REQUIRE(!inst.uuid.isNull());
    REQUIRE(inst.typeId == "pipe.straight");
    REQUIRE(inst.propertyValues.contains("length"));
    REQUIRE(inst.propertyValues["length"].toDouble() == 1.0);
}

TEST_CASE("Each instance has a unique UUID") {
    auto inst1 = ComponentFactory::instance().createInstance("pipe.straight");
    auto inst2 = ComponentFactory::instance().createInstance("pipe.straight");
    REQUIRE(inst1.uuid != inst2.uuid);
}

TEST_CASE("Check valve descriptor has cracking pressure property") {
    const auto* desc = ComponentFactory::instance().descriptorForType("valve.check");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->category == "Valves");
    bool hasCracking = false;
    for (const auto& p : desc->properties) {
        if (p.id == "crackingPressure") { hasCracking = true; break; }
    }
    REQUIRE(hasCracking);
}

TEST_CASE("Butterfly valve descriptor exists") {
    const auto* desc = ComponentFactory::instance().descriptorForType("valve.butterfly");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->category == "Valves");
}

TEST_CASE("Injector has two input ports") {
    const auto* desc = ComponentFactory::instance().descriptorForType("chamber.injector");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->inputPorts.size() == 2);
    REQUIRE(desc->outputPorts.size() == 1);
}

TEST_CASE("Nozzle has chamber properties") {
    const auto* desc = ComponentFactory::instance().descriptorForType("chamber.nozzle");
    REQUIRE(desc != nullptr);
    REQUIRE(desc->category == "Combustion");
    bool hasGamma = false, hasAreaRatio = false;
    for (const auto& p : desc->properties) {
        if (p.id == "gamma") hasGamma = true;
        if (p.id == "areaRatio") hasAreaRatio = true;
    }
    REQUIRE(hasGamma);
    REQUIRE(hasAreaRatio);
}

TEST_CASE("Combustion category has 3 components") {
    auto comps = ComponentFactory::instance().componentsInCategory("Combustion");
    REQUIRE(comps.size() == 3);
}
