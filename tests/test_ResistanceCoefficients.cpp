#include <catch2/catch_all.hpp>
#include "utils/ResistanceCoefficients.h"

using namespace ResistanceCoefficients;

namespace {
    bool approx(double a, double b, double tol = 1e-9) {
        return std::abs(a - b) < tol;
    }
}

// ─── Le/D Values ──────────────────────────────────────────

TEST_CASE("Elbow90 Threaded Le/D = 30", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Elbow90_Threaded, 0.05) == 30.0);
}

TEST_CASE("Elbow90 Flanged Le/D = 20", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Elbow90_Flanged, 0.05) == 20.0);
}

TEST_CASE("Elbow45 Threaded Le/D = 16", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Elbow45_Threaded, 0.05) == 16.0);
}

TEST_CASE("Elbow45 Flanged Le/D = 14", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Elbow45_Flanged, 0.05) == 14.0);
}

TEST_CASE("Elbow90 Long Radius Le/D = 14", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Elbow90_LongRadius, 0.05) == 14.0);
}

TEST_CASE("Tee Branch Le/D = 60", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Tee_Branch, 0.05) == 60.0);
}

TEST_CASE("Tee Straight Through Le/D = 20", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::Tee_StraightThrough, 0.05) == 20.0);
}

TEST_CASE("Gate Valve Le/D = 8", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::GateValve_FullyOpen, 0.05) == 8.0);
}

TEST_CASE("Ball Valve Le/D = 3", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::BallValve_FullyOpen, 0.05) == 3.0);
}

TEST_CASE("Swing Check Valve Le/D = 50", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::SwingCheckValve, 0.05) == 50.0);
}

TEST_CASE("Butterfly Valve Le/D = 45", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::ButterflyValve, 0.05) == 45.0);
}

TEST_CASE("Sudden Expansion Le/D = 0", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::SuddenExpansion, 0.05) == 0.0);
}

TEST_CASE("Pipe Exit Le/D = 0", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::PipeExit, 0.05) == 0.0);
}

TEST_CASE("Pipe Entry Inward Le/D = 32", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::PipeEntry_Inward, 0.05) == 32.0);
}

TEST_CASE("Pipe Entry Flush Le/D = 16", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::PipeEntry_Flush, 0.05) == 16.0);
}

// ─── Globe Valve size-dependent ───────────────────────────

TEST_CASE("Globe Valve small diameter returns 150", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::GlobeValve_FullyOpen, 0.01) == 150.0);
}

TEST_CASE("Globe Valve large diameter returns 340", "[ResistanceCoefficients]") {
    REQUIRE(leOverD(FittingType::GlobeValve_FullyOpen, 0.15) == 340.0);
}

// ─── zetaFromLeD ──────────────────────────────────────────

TEST_CASE("zetaFromLeD zero Le/D returns zero", "[ResistanceCoefficients]") {
    REQUIRE(zetaFromLeD(0.0, 0.05) == 0.0);
}

TEST_CASE("zetaFromLeD positive for typical fitting", "[ResistanceCoefficients]") {
    double zeta = zetaFromLeD(30.0, 0.05);
    REQUIRE(zeta > 0.0);
    REQUIRE(zeta < 2.0);
}

// ─── directZeta ───────────────────────────────────────────

TEST_CASE("directZeta Elbow45 = 0.35", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::Elbow45_Threaded), 0.35));
}

TEST_CASE("directZeta Tee Straight = 0.25", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::Tee_StraightThrough), 0.25));
}

TEST_CASE("directZeta Pipe Exit = 1.0", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::PipeExit), 1.0));
}

TEST_CASE("directZeta Ball Valve = 0.04", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::BallValve_FullyOpen), 0.04));
}

TEST_CASE("directZeta Sudden Expansion = 1.0", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::SuddenExpansion), 1.0));
}

TEST_CASE("directZeta Sudden Contraction = 0.5", "[ResistanceCoefficients]") {
    REQUIRE(approx(directZeta(FittingType::SuddenContraction), 0.5));
}

// ─── fittingName ──────────────────────────────────────────

TEST_CASE("fittingName returns non-empty for all 17 types", "[ResistanceCoefficients]") {
    for (int i = 0; i <= 16; ++i) {
        auto ft = static_cast<FittingType>(i);
        QString name = fittingName(ft);
        REQUIRE_FALSE(name.isEmpty());
        REQUIRE(name.length() > 3);
    }
}

TEST_CASE("fittingName Elbow90 contains '90 deg'", "[ResistanceCoefficients]") {
    QString name = fittingName(FittingType::Elbow90_Threaded);
    REQUIRE(name.contains("90 deg"));
    REQUIRE(name.contains("Elbow"));
}
