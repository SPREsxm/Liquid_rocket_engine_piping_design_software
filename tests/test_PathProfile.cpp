#include <catch2/catch_test_macros.hpp>

#include "utils/NetworkSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

namespace {
    struct PipeTestFixture {
        BlockScene* scene;
        BlockItem* tank1;
        BlockItem* pipe;
        BlockItem* valve;
        BlockItem* tank2;

        PipeTestFixture() {
            auto& factory = ComponentFactory::instance();
            scene = new BlockScene(&factory);
            tank1 = scene->addBlock(ComponentDescriptor::createStorageTank(), QPointF(0, 0));
            pipe  = scene->addBlock(ComponentDescriptor::createStraightPipe(), QPointF(150, 0));
            valve = scene->addBlock(ComponentDescriptor::createBallValve(), QPointF(300, 0));
            tank2 = scene->addBlock(ComponentDescriptor::createBufferTank(), QPointF(450, 0));

            scene->addConnection(tank1->outputPorts().first(), pipe->inputPorts().first());
            scene->addConnection(pipe->outputPorts().first(), valve->inputPorts().first());
            scene->addConnection(valve->outputPorts().first(), tank2->inputPorts().first());
        }

        ~PipeTestFixture() { delete scene; }
    };
}

TEST_CASE("computePathProfile on simple pipeline", "[PathProfile]")
{
    PipeTestFixture fix;
    fix.pipe->setPropertyValue("length", 3.0);

    auto sol = solveNetworkAuto(fix.scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    // Path from tank1 to tank2
    PathProfile profile = computePathProfile(
        fix.scene, sol, fix.tank1->uuid(), fix.tank2->uuid());

    REQUIRE(profile.points.size() == 4); // tank1→pipe→valve→tank2
    REQUIRE(profile.totalLength > 0.0);
}

TEST_CASE("computePathProfile null scene returns empty", "[PathProfile]")
{
    NetworkSolution emptySol;
    PathProfile profile = computePathProfile(nullptr, emptySol, QUuid::createUuid(), QUuid::createUuid());
    REQUIRE(profile.points.isEmpty());
    REQUIRE(profile.totalLength == 0.0);
}

TEST_CASE("computePathProfile unreachable end returns empty", "[PathProfile]")
{
    PipeTestFixture fix;
    auto sol = solveNetworkAuto(fix.scene, 1e6, 10.0);

    // Reverse direction: tank2→tank1 (flow goes opposite direction)
    PathProfile profile = computePathProfile(
        fix.scene, sol, fix.tank2->uuid(), fix.tank1->uuid());
    // Should be empty since edges go tank1→...→tank2, not the reverse
    REQUIRE(profile.points.isEmpty());
}

TEST_CASE("computePathProfile same start and end returns single point", "[PathProfile]")
{
    PipeTestFixture fix;
    auto sol = solveNetworkAuto(fix.scene, 1e6, 10.0);

    PathProfile profile = computePathProfile(
        fix.scene, sol, fix.tank1->uuid(), fix.tank1->uuid());
    REQUIRE(profile.points.size() == 1);
    REQUIRE(profile.totalLength == 0.0);
}

TEST_CASE("computePathProfile pressure decreases along path", "[PathProfile]")
{
    PipeTestFixture fix;
    fix.pipe->setPropertyValue("length", 3.0);

    auto sol = solveNetworkAuto(fix.scene, 1e6, 10.0);
    REQUIRE(sol.converged == true);

    PathProfile profile = computePathProfile(
        fix.scene, sol, fix.tank1->uuid(), fix.tank2->uuid());

    // Pressure should be monotonically decreasing along the path
    for (int i = 1; i < profile.points.size(); ++i) {
        REQUIRE(profile.points[i].pressure <= profile.points[i-1].pressure);
    }
}
