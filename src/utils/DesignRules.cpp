#include "DesignRules.h"
#include "PipeScheduleDatabase.h"
#include "PropellantProperties.h"
#include "NetworkSolver.h"
#include "ThermalSolver.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "ui/graphics/PortItem.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"

#include <QtMath>

// ─── Helpers ──────────────────────────────────────────────────

static double velocityLimit(FluidType ft)
{
    switch (ft) {
    case FluidType::LOX:  return 10.0;
    case FluidType::RP1:  return 7.0;
    case FluidType::LH2:  return 30.0;
    case FluidType::CH4:  return 8.0;
    case FluidType::Water:return 3.0;
    default:              return 10.0;
    }
}

static const char* fluidTypeName(FluidType ft)
{
    switch (ft) {
    case FluidType::LOX:  return "LOX";
    case FluidType::RP1:  return "RP-1";
    case FluidType::LH2:  return "LH2";
    case FluidType::CH4:  return "CH4";
    case FluidType::Water:return "Water";
    default:              return "Unknown";
    }
}

static double getPipeDiameter(BlockItem* block)
{
    if (!block) return 0.05;
    // Try diameter property first
    QVariant dia = block->propertyValue("diameter");
    if (dia.isValid() && dia.toDouble() > 0.0)
        return dia.toDouble();
    // Try NPS lookup
    QVariant nps = block->propertyValue("nps");
    QVariant sch = block->propertyValue("schedule");
    if (nps.isValid() && sch.isValid()) {
        double npsVal = nps.toDouble();
        if (npsVal > 0.0) {
            auto od = PipeScheduleDatabase::instance().outerDiameter(npsVal, sch.toString());
            if (od.has_value()) return od.value() / 1000.0; // mm → m
        }
    }
    return 0.05; // default fallback
}

static double getPipeWallThickness(BlockItem* block)
{
    if (!block) return 0.001;
    QVariant nps = block->propertyValue("nps");
    QVariant sch = block->propertyValue("schedule");
    if (nps.isValid() && sch.isValid()) {
        double npsVal = nps.toDouble();
        if (npsVal > 0.0) {
            auto wt = PipeScheduleDatabase::instance().wallThickness(npsVal, sch.toString());
            if (wt.has_value()) return wt.value() / 1000.0; // mm → m
        }
    }
    return 0.001; // fallback 1mm
}

static bool isPumpType(const QString& typeId)
{
    return typeId.startsWith("pump.") || typeId.startsWith("turbopump.") || typeId == "turbine.gas";
}

// ─── Check implementations ────────────────────────────────────

static void checkFlowVelocity(BlockScene* scene, const NetworkSolution& sol,
                              const SolverSettings& settings, DesignCheckResult& result)
{
    const double vLimit = velocityLimit(settings.fluidType);
    const double rho = settings.fluidDensity;
    if (rho <= 0.0) return;

    for (const auto& edge : sol.edges) {
        BlockItem* srcBlock = scene->blockByUuid(edge.sourceUuid);
        double dia = getPipeDiameter(srcBlock);
        double area = M_PI * dia * dia / 4.0;
        if (area <= 0.0) continue;
        double vel = qAbs(edge.massFlowRate) / (rho * area);

        if (vel > vLimit * 2.0) {
            result.items.append({DesignCheckResult::Error, "Flow Velocity",
                edge.sourceUuid.toString(), edge.sourceUuid,
                QString("Velocity %1 m/s exceeds %2 limit %3 m/s (>2×)")
                    .arg(vel, 0, 'f', 2).arg(fluidTypeName(settings.fluidType)).arg(vLimit),
                vel, vLimit * 2.0, "m/s"});
        } else if (vel > vLimit) {
            result.items.append({DesignCheckResult::Warning, "Flow Velocity",
                edge.sourceUuid.toString(), edge.sourceUuid,
                QString("Velocity %1 m/s exceeds %2 limit %3 m/s")
                    .arg(vel, 0, 'f', 2).arg(fluidTypeName(settings.fluidType)).arg(vLimit),
                vel, vLimit, "m/s"});
        }
    }
}

static void checkCavitation(BlockScene* scene, const NetworkSolution& sol,
                            const SolverSettings& settings, DesignCheckResult& result)
{
    const double rho = settings.fluidDensity;
    const double g = 9.80665;
    if (rho <= 0.0) return;

    // Get vapor pressure from Wagner equation at a reference temperature
    // Use boiling point as approximation for operating temperature
    double Tref = 90.0; // LOX boiling ~90K
    double pVapor = 0.0;
    switch (settings.fluidType) {
    case FluidType::LOX:
        pVapor = PropellantProperties::wagnerVaporPressure(Tref, PropellantProperties::wagnerLOX());
        break;
    case FluidType::CH4:
        pVapor = PropellantProperties::wagnerVaporPressure(111.6, PropellantProperties::wagnerMethane());
        break;
    case FluidType::LH2:
        pVapor = PropellantProperties::wagnerVaporPressure(20.4, PropellantProperties::wagnerLH2());
        break;
    case FluidType::Water:
        pVapor = PropellantProperties::wagnerVaporPressure(373.15, PropellantProperties::wagnerWater());
        break;
    case FluidType::RP1:
        pVapor = PropellantProperties::wagnerVaporPressure(490.0, PropellantProperties::wagnerRP1());
        break;
    default: break;
    }

    for (const auto& node : sol.nodes) {
        BlockItem* block = scene->blockByUuid(node.blockUuid);
        if (!block || !isPumpType(block->typeId())) continue;

        // Get NPSHr from pump property (check both naming conventions)
        QVariant npshrVar = block->propertyValue("npshr");
        if (!npshrVar.isValid())
            npshrVar = block->propertyValue("npshRequired");
        double npshr = npshrVar.isValid() ? npshrVar.toDouble() : 0.5;

        double pInlet = node.pressure; // Pa gauge — assume atmospheric reference
        double npsha = (pInlet + 101325.0 - pVapor) / (rho * g); // convert gauge to absolute

        const double margin = 0.5; // m safety margin
        if (npsha < npshr) {
            result.items.append({DesignCheckResult::Error, "Cavitation (NPSH)",
                node.blockLabel, node.blockUuid,
                QString("NPSHa %1 m < NPSHr %2 m — cavitation risk")
                    .arg(npsha, 0, 'f', 2).arg(npshr),
                npsha, npshr, "m"});
        } else if (npsha < npshr + margin) {
            result.items.append({DesignCheckResult::Warning, "Cavitation (NPSH)",
                node.blockLabel, node.blockUuid,
                QString("NPSHa %1 m close to NPSHr %2 m (margin < %3 m)")
                    .arg(npsha, 0, 'f', 2).arg(npshr).arg(margin),
                npsha, npshr + margin, "m"});
        }
    }
}

static void checkWallThickness(BlockScene* scene, const NetworkSolution& sol,
                               const SolverSettings&, DesignCheckResult& result)
{
    // ASME B31.3: t_min = P*D / (2*S*E + P*Y)
    const double S = 115.0e6;   // 304L SS allowable stress @ 200°C (Pa)
    const double E = 1.0;       // joint factor (seamless)
    const double Y = 0.4;       // temperature coefficient
    const double corrAllowance = 0.0005; // 0.5 mm corrosion allowance (m)

    for (const auto& node : sol.nodes) {
        BlockItem* block = scene->blockByUuid(node.blockUuid);
        if (!block) continue;

        double wallThick = getPipeWallThickness(block);
        if (wallThick <= 0.0) continue; // not a pipe with schedule

        double od = getPipeDiameter(block); // OD for scheduled pipe
        double pressure = qMax(node.pressure, 1.0e5); // at least 1 bar design

        double tMin = pressure * od / (2.0 * S * E + pressure * Y);
        double tRequired = tMin + corrAllowance;

        if (wallThick < tMin) {
            result.items.append({DesignCheckResult::Error, "Wall Thickness (ASME B31.3)",
                node.blockLabel, node.blockUuid,
                QString("Wall %1 mm < ASME B31.3 t_min %2 mm @ %3 MPa")
                    .arg(wallThick * 1000.0, 0, 'f', 2)
                    .arg(tMin * 1000.0, 0, 'f', 2)
                    .arg(pressure / 1.0e6, 0, 'f', 3),
                wallThick * 1000.0, tMin * 1000.0, "mm"});
        } else if (wallThick < tRequired) {
            result.items.append({DesignCheckResult::Warning, "Wall Thickness (ASME B31.3)",
                node.blockLabel, node.blockUuid,
                QString("Wall %1 mm < t_min+corrosion %2 mm")
                    .arg(wallThick * 1000.0, 0, 'f', 2)
                    .arg(tRequired * 1000.0, 0, 'f', 2),
                wallThick * 1000.0, tRequired * 1000.0, "mm"});
        }
    }
}

static void checkPressureDropBudget(BlockScene*, const NetworkSolution& sol,
                                    double maxDp, DesignCheckResult& result)
{
    if (maxDp <= 0.0) return;

    double totalDp = sol.totalPressureDrop;
    if (totalDp > maxDp) {
        result.items.append({DesignCheckResult::Error, "Pressure Drop Budget",
            "System", {},
            QString("Total ΔP %1 MPa exceeds budget %2 MPa")
                .arg(totalDp / 1.0e6, 0, 'f', 3).arg(maxDp / 1.0e6, 0, 'f', 3),
            totalDp, maxDp, "Pa"});
    }
}

// ─── Public entry point ───────────────────────────────────────

DesignCheckResult runDesignChecks(
    BlockScene* scene,
    const NetworkSolution& solution,
    const SolverSettings& settings,
    double maxPressureDropPa)
{
    DesignCheckResult result;

    if (!scene) return result;

    checkFlowVelocity(scene, solution, settings, result);
    checkCavitation(scene, solution, settings, result);
    checkWallThickness(scene, solution, settings, result);
    checkPressureDropBudget(scene, solution, maxPressureDropPa, result);

    return result;
}

// ─── Pipe structure stress check ──────────────────────────────────

static void checkPipeStress(const ThermalStressResult& tsr,
                             DesignCheckResult& result)
{
    for (const auto& te : tsr.edges) {
        if (te.safetyFactor < 1.5 && te.safetyFactor >= 1.0) {
            result.items.append({DesignCheckResult::Warning, "Pipe Stress (FSI)",
                QString(), te.sourceUuid,
                QString("Safety factor %1 < 1.5 (material: %2)")
                    .arg(te.safetyFactor, 0, 'f', 2).arg(te.materialUsed),
                te.safetyFactor, 1.5, "—"});
        } else if (te.safetyFactor < 1.0) {
            result.items.append({DesignCheckResult::Error, "Pipe Stress (FSI)",
                QString(), te.sourceUuid,
                QString("YIELD EXCEEDED: safety factor %1 (material: %2, σ_vm=%3 MPa)")
                    .arg(te.safetyFactor, 0, 'f', 2)
                    .arg(te.materialUsed)
                    .arg(te.vonMisesStress_Pa / 1.0e6, 0, 'f', 1),
                te.safetyFactor, 1.0, "—"});
        }
    }
}

DesignCheckResult runDesignChecks(
    BlockScene* scene,
    const NetworkSolution& solution,
    const SolverSettings& settings,
    double maxPressureDropPa,
    const ThermalStressResult& tsr)
{
    DesignCheckResult result = runDesignChecks(scene, solution, settings, maxPressureDropPa);
    checkPipeStress(tsr, result);
    return result;
}
