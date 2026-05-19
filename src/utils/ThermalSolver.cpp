#include "ThermalSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "FluidDynamics.h"
#include "PropellantProperties.h"
#include "FluidStructureInteraction.h"
#include "PipeScheduleDatabase.h"

#include <QtMath>

namespace {

// Fluid thermal conductivity lookup (W/(m·K))
inline double fluidThermalConductivity_WpmK(const SolverSettings& s)
{
    // Approximate values for common rocket propellants at ~200K
    switch (s.fluidType) {
    case FluidType::LOX:   return 0.152;  // LOX @ 90K
    case FluidType::RP1:   return 0.135;  // RP-1 @ 300K
    case FluidType::CH4:   return 0.190;  // CH4 @ 111K
    case FluidType::LH2:   return 0.100;  // LH2 @ 20K
    case FluidType::Water: return 0.610;  // Water @ 300K
    }
    return 0.15;
}

// Prandtl number approximation
inline double fluidPrandtl(const SolverSettings& s)
{
    switch (s.fluidType) {
    case FluidType::LOX:   return 2.5;
    case FluidType::RP1:   return 18.0;
    case FluidType::CH4:   return 1.8;
    case FluidType::LH2:   return 1.0;
    case FluidType::Water: return 7.0;
    }
    return 5.0;
}

} // anonymous namespace

ThermalStressResult computeThermalStress(
    BlockScene* scene,
    const NetworkSolution& solution,
    const SolverSettings& settings)
{
    ThermalStressResult result;
    if (!scene || !solution.converged) return result;

    // Build block lookup
    QHash<QUuid, BlockItem*> blockByUuid;
    for (auto* b : scene->allBlocks())
        blockByUuid[b->uuid()] = b;

    const double rho = settings.fluidDensity;
    const double mu  = settings.fluidViscosity;
    const double k   = fluidThermalConductivity_WpmK(settings);
    const double Pr  = fluidPrandtl(settings);

    double sumH = 0.0;
    int count = 0;

    for (const auto& edge : solution.edges) {
        ThermalStressEdge te;
        te.sourceUuid = edge.sourceUuid;
        te.destUuid   = edge.destUuid;

        BlockItem* block = blockByUuid.value(edge.sourceUuid);
        if (!block) continue;

        // ── Geometry ─────────────────────────────────────────
        double D = 0.0254; // default 1"
        QVariant dv = block->propertyValue("diameter");
        if (dv.isValid() && dv.toDouble() > 0.0)
            D = dv.toDouble();

        double wallThk = 0.00127; // default 0.05"
        // Try PipeScheduleDatabase first
        double nps = block->propertyValue("nps").toDouble();
        QString sch = block->propertyValue("schedule").toString();
        if (nps > 0.0 && !sch.isEmpty()) {
            auto wt = PipeScheduleDatabase::instance().wallThickness(nps, sch);
            if (wt.has_value()) wallThk = wt.value() / 1000.0; // mm→m
        }

        double A = M_PI * D * D / 4.0;
        double v = (rho > 0.0 && A > 0.0) ? qAbs(edge.massFlowRate) / (rho * A) : 0.0;

        // ── Heat transfer ────────────────────────────────────
        double Re = (mu > 0.0) ? rho * v * D / mu : 0.0;
        te.reynoldsNumber = Re;
        te.prandtlNumber = Pr;

        if (Re > 2300.0 && Pr > 0.5) {
            // Gnielinski correlation (valid: 2300 ≤ Re ≤ 5e6, 0.5 ≤ Pr ≤ 2000)
            double f = FluidDynamics::calculateColebrookWhiteFrictionFactor(
                Re, settings.pipeRoughness / D, D);
            double f8 = f / 8.0;
            double sqrt_f8 = std::sqrt(f8);
            double Pr23 = std::pow(Pr, 2.0/3.0);
            double numer = f8 * (Re - 1000.0) * Pr;
            double denom = 1.0 + 12.7 * sqrt_f8 * (Pr23 - 1.0);
            te.nusseltNumber = (denom > 0.0) ? numer / denom : 0.0;
        } else if (Re > 0.0) {
            te.nusseltNumber = 4.36; // laminar, constant heat flux
        }

        te.heatTransferCoeff_Wpm2K = (D > 0.0) ? te.nusseltNumber * k / D : 0.0;
        sumH += te.heatTransferCoeff_Wpm2K;
        ++count;

        // ── FSI stress ───────────────────────────────────────
        // Find pressure at the source node
        double pressure = 0.0;
        for (const auto& node : solution.nodes) {
            if (node.blockUuid == edge.sourceUuid) {
                pressure = node.pressure;
                break;
            }
        }

        // Material properties
        QString matStr = block->propertyValue("material").toString();
        auto mat = FluidStructureInteraction::materialByName(matStr.isEmpty() ? "316L" : matStr);
        te.materialUsed = matStr.isEmpty() ? "316L SS" : matStr;

        FluidStructureInteraction::PipeMechanics pipeMech;
        pipeMech.innerDiameter_m = D;
        pipeMech.wallThickness_m = wallThk;
        pipeMech.youngsModulus_Pa = mat.youngsModulus_Pa;
        pipeMech.poissonRatio = mat.poissonRatio;
        pipeMech.yieldStrength_Pa = mat.yieldStrength_Pa;
        pipeMech.materialDensity_kgpm3 = mat.density_kgpm3;

        FluidStructureInteraction::FluidCoupling fluidCoup;
        fluidCoup.pressure_Pa = qAbs(pressure);
        // Fluid bulk modulus lookup (Pa) — used for Korteweg wave speed
        switch (settings.fluidType) {
        case FluidType::LOX:   fluidCoup.bulkModulus_Pa = 0.97e9; break;
        case FluidType::RP1:   fluidCoup.bulkModulus_Pa = 1.50e9; break;
        case FluidType::CH4:   fluidCoup.bulkModulus_Pa = 0.85e9; break;
        case FluidType::LH2:   fluidCoup.bulkModulus_Pa = 0.12e9; break;
        case FluidType::Water: fluidCoup.bulkModulus_Pa = 2.18e9; break;
        default:               fluidCoup.bulkModulus_Pa = 1.0e9; break;
        }
        fluidCoup.density_kgpm3 = rho;

        auto stress = FluidStructureInteraction::computeStresses(pipeMech, fluidCoup);

        te.hoopStress_Pa = stress.hoopStress_Pa;
        te.longitudinalStress_Pa = stress.longitudinalStress_Pa;
        te.vonMisesStress_Pa = stress.vonMisesStress_Pa;
        te.safetyFactor = stress.safetyFactor;
        te.yieldExceeded = stress.yieldExceeded;
        te.kortevegWaveSpeed_mps = stress.kortevegWaveSpeed_mps;

        if (stress.safetyFactor < result.minSafetyFactor)
            result.minSafetyFactor = stress.safetyFactor;
        if (stress.yieldExceeded)
            ++result.edgesWithYieldExceeded;

        result.edges.append(te);
    }

    result.avgHeatTransferCoeff = (count > 0) ? sumH / count : 0.0;

    return result;
}
