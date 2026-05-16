#include <catch2/catch_all.hpp>
#include "utils/PipeScheduleDatabase.h"

TEST_CASE("PipeScheduleDatabase has 10 standard sizes") {
    const auto& db = PipeScheduleDatabase::instance();
    REQUIRE(db.availableSizes().size() == 10);
}

TEST_CASE("PipeScheduleDatabase lookup valid NPS + schedule") {
    const auto& db = PipeScheduleDatabase::instance();

    const auto* entry = db.lookup(0.5, "40S");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->schedule == "40S");
    REQUIRE(entry->wallThickness > 2.7);
    REQUIRE(entry->wallThickness < 2.9);
    REQUIRE(entry->innerDiameter > 15.0);
    REQUIRE(entry->innerDiameter < 16.0);
}

TEST_CASE("PipeScheduleDatabase 1in Sch 10S") {
    const auto& db = PipeScheduleDatabase::instance();

    const auto* entry = db.lookup(1.0, "10S");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->wallThickness > 2.0);
    REQUIRE(entry->wallThickness < 3.0);
    REQUIRE(entry->innerDiameter > 27.0);
    REQUIRE(entry->innerDiameter < 29.0);
}

TEST_CASE("PipeScheduleDatabase 2in Sch 80S") {
    const auto& db = PipeScheduleDatabase::instance();

    const auto* entry = db.lookup(2.0, "80S");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->wallThickness > 5.0);
    REQUIRE(entry->wallThickness < 6.0);
    REQUIRE(entry->innerDiameter > 48.0);
    REQUIRE(entry->innerDiameter < 51.0);
}

TEST_CASE("PipeScheduleDatabase lookup invalid NPS returns nullptr") {
    const auto& db = PipeScheduleDatabase::instance();
    REQUIRE(db.lookup(99.0, "40S") == nullptr);
    REQUIRE(db.lookup(0.5, "Nonexistent") == nullptr);
}

TEST_CASE("PipeScheduleDatabase outer/inner/wall convenience methods") {
    const auto& db = PipeScheduleDatabase::instance();

    auto od = db.outerDiameter(1.5, "40S");
    REQUIRE(od.has_value());
    REQUIRE(*od > 48.0);
    REQUIRE(*od < 49.0);

    auto id = db.innerDiameter(1.5, "40S");
    REQUIRE(id.has_value());
    REQUIRE(*id > 40.0);
    REQUIRE(*id < 42.0);

    auto wall = db.wallThickness(1.5, "40S");
    REQUIRE(wall.has_value());
    REQUIRE(*wall > 3.0);
    REQUIRE(*wall < 4.0);
}

TEST_CASE("PipeScheduleDatabase default schedule is 40S") {
    const auto& db = PipeScheduleDatabase::instance();
    REQUIRE(db.defaultSchedule(0.5) == "40S");
    REQUIRE(db.defaultSchedule(1.0) == "40S");
    REQUIRE(db.defaultSchedule(2.0) == "40S");
}

TEST_CASE("PipeScheduleDatabase schedulesForSize") {
    const auto& db = PipeScheduleDatabase::instance();
    auto schedules = db.schedulesForSize(0.5);
    REQUIRE(!schedules.isEmpty());
    REQUIRE(schedules.contains("5S"));
    REQUIRE(schedules.contains("10S"));
    REQUIRE(schedules.contains("40S"));
    REQUIRE(schedules.contains("80S"));
}

TEST_CASE("PipeScheduleDatabase sizeEntry returns correct DN") {
    const auto& db = PipeScheduleDatabase::instance();
    const auto* entry = db.sizeEntry(0.25);
    REQUIRE(entry != nullptr);
    REQUIRE(entry->dn == "DN8");
    REQUIRE(entry->outerDiameter > 13.0);
    REQUIRE(entry->outerDiameter < 14.0);

    const auto* entry2 = db.sizeEntry(2.0);
    REQUIRE(entry2 != nullptr);
    REQUIRE(entry2->dn == "DN50");
}

TEST_CASE("PipeScheduleDatabase availableSizeNames are fractions") {
    const auto& db = PipeScheduleDatabase::instance();
    auto names = db.availableSizeNames();
    REQUIRE(names.size() == 10);
    REQUIRE(names.contains("1/4\""));
    REQUIRE(names.contains("1/2\""));
    REQUIRE(names.contains("1\""));
    REQUIRE(names.contains("2\""));
    REQUIRE(names.contains("3\""));
}

TEST_CASE("PipeScheduleDatabase weight is reasonable") {
    const auto& db = PipeScheduleDatabase::instance();
    const auto* entry = db.lookup(1.0, "40S");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->weightPerMeter > 1.0);
    REQUIRE(entry->weightPerMeter < 5.0);
}

TEST_CASE("PipeScheduleDatabase 1/4in XXS is thick-walled") {
    const auto& db = PipeScheduleDatabase::instance();
    const auto* entry = db.lookup(0.25, "XXS");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->wallThickness > 4.0);
    REQUIRE(entry->wallThickness < 5.0);
    REQUIRE(entry->innerDiameter < 10.0);
}
