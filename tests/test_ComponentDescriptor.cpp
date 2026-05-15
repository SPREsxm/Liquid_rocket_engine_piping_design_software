#include <catch2/catch_all.hpp>
#include "components/ComponentDescriptor.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("StraightPipe descriptor is valid") {
    auto d = ComponentDescriptor::createStraightPipe();

    REQUIRE(d.isValid());
    REQUIRE(d.typeId == "pipe.straight");
    REQUIRE(d.category == "Pipes");
    REQUIRE(d.inputPorts.size() == 1);
    REQUIRE(d.outputPorts.size() == 1);
    REQUIRE(d.inputPorts[0].id == "inlet");
    REQUIRE(d.inputPorts[0].direction == PortDirection::Input);
    REQUIRE(d.outputPorts[0].id == "outlet");
    REQUIRE(d.outputPorts[0].direction == PortDirection::Output);
    REQUIRE(d.properties.size() >= 4);
}

TEST_CASE("Tee has two output ports") {
    auto d = ComponentDescriptor::createTee();

    REQUIRE(d.inputPorts.size() == 1);
    REQUIRE(d.outputPorts.size() == 2);
    REQUIRE(d.outputPorts[0].id == "outlet_a");
    REQUIRE(d.outputPorts[1].id == "outlet_b");
}

TEST_CASE("PressureSensor has Fluid input and Signal output") {
    auto d = ComponentDescriptor::createPressureSensor();

    REQUIRE(d.inputPorts.size() == 1);
    REQUIRE(d.inputPorts[0].dataType == PortDataType::Fluid);
    REQUIRE(d.outputPorts.size() == 1);
    REQUIRE(d.outputPorts[0].dataType == PortDataType::Signal);
}

TEST_CASE("StorageTank has no input, one output") {
    auto d = ComponentDescriptor::createStorageTank();

    REQUIRE(d.inputPorts.empty());
    REQUIRE(d.outputPorts.size() == 1);
    REQUIRE(d.outputPorts[0].id == "outlet");
}

TEST_CASE("StraightPipe property defaults are correct") {
    auto d = ComponentDescriptor::createStraightPipe();

    auto lengthProp = d.properties[0];
    REQUIRE(lengthProp.id == "length");
    REQUIRE(lengthProp.defaultValue.toDouble() == 1.0);
    REQUIRE(lengthProp.unit == "m");

    auto diamProp = d.properties[1];
    REQUIRE(diamProp.id == "diameter");
    REQUIRE(diamProp.defaultValue.toDouble() == 0.05);
}

TEST_CASE("All 13 built-in descriptors are valid") {
    auto all = {
        ComponentDescriptor::createStraightPipe(),
        ComponentDescriptor::createElbow(),
        ComponentDescriptor::createTee(),
        ComponentDescriptor::createGateValve(),
        ComponentDescriptor::createGlobeValve(),
        ComponentDescriptor::createBallValve(),
        ComponentDescriptor::createSolenoidValve(),
        ComponentDescriptor::createCentrifugalPump(),
        ComponentDescriptor::createPistonPump(),
        ComponentDescriptor::createPressureSensor(),
        ComponentDescriptor::createFlowSensor(),
        ComponentDescriptor::createStorageTank(),
        ComponentDescriptor::createBufferTank()
    };

    for (const auto& d : all) {
        REQUIRE(d.isValid());
        REQUIRE(!d.typeId.isEmpty());
        REQUIRE(!d.category.isEmpty());
        REQUIRE(!d.displayName.isEmpty());
    }
}
