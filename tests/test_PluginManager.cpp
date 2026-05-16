#include <catch2/catch_all.hpp>
#include "core/PluginManager.h"
#include "core/IPlugin.h"
#include <QDir>

TEST_CASE("PluginManager is singleton") {
    PluginManager& m1 = PluginManager::instance();
    PluginManager& m2 = PluginManager::instance();
    REQUIRE(&m1 == &m2);
}

TEST_CASE("PluginManager starts with zero plugins") {
    auto& mgr = PluginManager::instance();
    REQUIRE(mgr.pluginCount() == 0);
    REQUIRE(mgr.loadedPluginIds().isEmpty());
}

TEST_CASE("PluginManager discoverPlgins with non-existent directory") {
    auto& mgr = PluginManager::instance();
    int loaded = mgr.discoverPlugins("C:/nonexistent_plugin_dir_xyz");
    REQUIRE(loaded == 0);
}

TEST_CASE("PluginManager loadPlugin non-existent file returns nullptr") {
    auto& mgr = PluginManager::instance();
    IPlugin* p = mgr.loadPlugin("C:/nonexistent_plugin.dll");
    REQUIRE(p == nullptr);
}

TEST_CASE("PluginManager plugin() for unknown id returns nullptr") {
    auto& mgr = PluginManager::instance();
    REQUIRE(mgr.plugin("unknown.plugin") == nullptr);
}

TEST_CASE("PluginManager unloadPlugin unknown id returns false") {
    auto& mgr = PluginManager::instance();
    REQUIRE(mgr.unloadPlugin("unknown.plugin") == false);
}

TEST_CASE("PluginManager execute unknown plugin returns error") {
    auto& mgr = PluginManager::instance();
    auto result = mgr.execute("unknown.plugin", "solve");
    REQUIRE(result.contains("error"));
}

TEST_CASE("PluginManager allPluginComponentInfos is empty initially") {
    auto& mgr = PluginManager::instance();
    auto infos = mgr.allPluginComponentInfos();
    REQUIRE(infos.isEmpty());
}

TEST_CASE("PluginManager pluginComponentInfos unknown id returns empty") {
    auto& mgr = PluginManager::instance();
    auto infos = mgr.pluginComponentInfos("unknown.plugin");
    REQUIRE(infos.isEmpty());
}

TEST_CASE("PluginManager unloadAll on empty manager does not crash") {
    auto& mgr = PluginManager::instance();
    REQUIRE_NOTHROW(mgr.unloadAll());
    REQUIRE(mgr.pluginCount() == 0);
}
