#include <catch2/catch_all.hpp>
#include "utils/IdGenerator.h"
#include <QSet>

TEST_CASE("IdGenerator generates unique UUIDs", "[IdGenerator]") {
    QUuid a = IdGenerator::generate();
    QUuid b = IdGenerator::generate();
    REQUIRE_FALSE(a.isNull());
    REQUIRE_FALSE(b.isNull());
    REQUIRE(a != b);
}

TEST_CASE("IdGenerator generate returns valid v4 UUID format", "[IdGenerator]") {
    QUuid id = IdGenerator::generate();
    // QUuid::createUuid produces v4 random UUIDs
    REQUIRE(id.variant() == QUuid::DCE);
    REQUIRE(id.version() == QUuid::Random);
}

TEST_CASE("IdGenerator fromString parses valid UUID", "[IdGenerator]") {
    QUuid id = QUuid::createUuid();
    QString str = id.toString();
    QUuid parsed = IdGenerator::fromString(str);
    REQUIRE_FALSE(parsed.isNull());
    REQUIRE(parsed == id);
}

TEST_CASE("IdGenerator fromString returns null for malformed string", "[IdGenerator]") {
    QUuid parsed = IdGenerator::fromString("not-a-uuid");
    REQUIRE(parsed.isNull());
}

TEST_CASE("IdGenerator fromString returns null for empty string", "[IdGenerator]") {
    QUuid parsed = IdGenerator::fromString("");
    REQUIRE(parsed.isNull());
}

TEST_CASE("IdGenerator fromString handles QUuid::WithoutBraces format", "[IdGenerator]") {
    QUuid id = QUuid::createUuid();
    QString withoutBraces = id.toString(QUuid::WithoutBraces);
    QUuid parsed = IdGenerator::fromString(withoutBraces);
    REQUIRE_FALSE(parsed.isNull());
    REQUIRE(parsed == id);
}

TEST_CASE("IdGenerator multiple generations are all unique", "[IdGenerator]") {
    const int N = 100;
    QSet<QString> seen;
    for (int i = 0; i < N; ++i) {
        QString s = IdGenerator::generate().toString();
        REQUIRE_FALSE(seen.contains(s));
        seen.insert(s);
    }
}

TEST_CASE("IdGenerator generate produces non-null unique IDs rapidly", "[IdGenerator]") {
    QUuid prev = IdGenerator::generate();
    for (int i = 0; i < 50; ++i) {
        QUuid next = IdGenerator::generate();
        REQUIRE_FALSE(next.isNull());
        REQUIRE(next != prev);
        prev = next;
    }
}
