#include <catch2/catch_all.hpp>
#include <QApplication>

// Placeholder — real tests added in Phase 2
TEST_CASE("Application builds correctly") {
    REQUIRE(true);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    return Catch::Session().run(argc, argv);
}
