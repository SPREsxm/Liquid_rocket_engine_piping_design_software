#include <catch2/catch_all.hpp>
#include "components/ComponentFactory.h"
#include "components/ComponentInstance.h"

TEST_CASE("ComponentFactory has five categories") {
    auto cats = ComponentFactory::instance().allCategories();
    REQUIRE(cats.size() == 5);
    REQUIRE(cats.contains("Pipes"));
    REQUIRE(cats.contains("Valves"));
    REQUIRE(cats.contains("Pumps"));
    REQUIRE(cats.contains("Sensors"));
    REQUIRE(cats.contains("Tanks"));
}

TEST_CASE("ComponentFactory returns 5 pipes") {
    auto pipes = ComponentFactory::instance().componentsInCategory("Pipes");
    REQUIRE(pipes.size() == 5);
}

TEST_CASE("ComponentFactory returns 4 valves") {
    auto valves = ComponentFactory::instance().componentsInCategory("Valves");
    REQUIRE(valves.size() == 4);
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
