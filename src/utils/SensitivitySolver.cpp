#include "SensitivitySolver.h"
#include "ui/graphics/BlockScene.h"

#include <QtMath>
#include <algorithm>

// ── Helpers: read/write SolverSettings fields by string key ──────────

static double readParam(const SolverSettings& s, const QString& key)
{
    if (key == "fluidDensity")       return s.fluidDensity;
    if (key == "fluidViscosity")     return s.fluidViscosity;
    if (key == "pipeRoughness")      return s.pipeRoughness;
    if (key == "pipeYoungsModulus")  return s.pipeYoungsModulus;
    if (key == "pipeWallThickness")  return s.pipeWallThickness;
    if (key == "tolerance")          return s.tolerance;
    if (key == "relaxationFactor")   return s.relaxationFactor;
    if (key == "targetCourant")      return s.targetCourant;
    return 0.0;
}

static void writeParam(SolverSettings& s, const QString& key, double val)
{
    if (key == "fluidDensity")       s.fluidDensity = val;
    else if (key == "fluidViscosity") s.fluidViscosity = val;
    else if (key == "pipeRoughness")  s.pipeRoughness = val;
    else if (key == "pipeYoungsModulus") s.pipeYoungsModulus = val;
    else if (key == "pipeWallThickness") s.pipeWallThickness = val;
    else if (key == "tolerance")      s.tolerance = val;
    else if (key == "relaxationFactor") s.relaxationFactor = val;
    else if (key == "targetCourant")  s.targetCourant = val;
}

static QString unitForKey(const QString& key)
{
    if (key == "fluidDensity")       return QStringLiteral("kg/m³");
    if (key == "fluidViscosity")     return QStringLiteral("Pa·s");
    if (key == "pipeRoughness")      return QStringLiteral("m");
    if (key == "pipeYoungsModulus")  return QStringLiteral("Pa");
    if (key == "pipeWallThickness")  return QStringLiteral("m");
    if (key == "tolerance")          return QStringLiteral("—");
    if (key == "relaxationFactor")   return QStringLiteral("—");
    if (key == "targetCourant")      return QStringLiteral("—");
    if (key == "inletPressurePa")    return QStringLiteral("Pa");
    if (key == "inletMassFlow")      return QStringLiteral("kg/s");
    return {};
}

// ── SensitivityResult helpers ────────────────────────────────────────

QVector<double> SensitivityResult::totalPressureDropSeries() const
{
    QVector<double> r;
    r.reserve(points.size());
    for (const auto& p : points)
        r.append(p.solution.totalPressureDrop);
    return r;
}

QVector<double> SensitivityResult::maxPressureSeries() const
{
    QVector<double> r;
    r.reserve(points.size());
    for (const auto& p : points)
        r.append(p.solution.maxPressure());
    return r;
}

QVector<double> SensitivityResult::thrustSeries() const
{
    QVector<double> r;
    r.reserve(points.size());
    for (const auto& p : points) {
        if (p.solution.hasThrustResults)
            r.append(p.solution.thrustResult.thrust_N);
        else
            r.append(0.0);
    }
    return r;
}

QVector<double> SensitivityResult::ispSeries() const
{
    QVector<double> r;
    r.reserve(points.size());
    for (const auto& p : points) {
        if (p.solution.hasThrustResults)
            r.append(p.solution.thrustResult.specificImpulse_s);
        else
            r.append(0.0);
    }
    return r;
}

// ── Parameter sweep ──────────────────────────────────────────────────

SensitivityResult runParameterSweep(
    BlockScene* scene,
    const SolverSettings& baseSettings,
    const QString& paramKey,
    double paramMin, double paramMax,
    int steps,
    double inletPressurePa,
    double inletMassFlowKgPerS)
{
    SensitivityResult result;
    result.sweptParamName = paramKey;
    result.sweptParamUnit = unitForKey(paramKey);

    if (!scene) return result;

    const int actualSteps = qMax(steps, 2);
    for (int i = 0; i < actualSteps; ++i) {
        double t = static_cast<double>(i) / (actualSteps - 1);
        double val = paramMin + t * (paramMax - paramMin);

        double pIn = inletPressurePa;
        double mIn = inletMassFlowKgPerS;
        SolverSettings s = baseSettings;

        if (paramKey == "inletPressurePa") {
            pIn = val;
        } else if (paramKey == "inletMassFlow") {
            mIn = val;
        } else {
            writeParam(s, paramKey, val);
        }

        NetworkSolution sol = solveNetworkAuto(scene, s, pIn, mIn);
        result.points.append({val, sol});
    }

    return result;
}

// ── Multi-condition comparison ───────────────────────────────────────

SensitivityResult runMultiCondition(
    BlockScene* scene,
    const QVector<SolverSettings>& conditions,
    double inletPressurePa,
    double inletMassFlowKgPerS)
{
    SensitivityResult result;
    result.sweptParamName = QStringLiteral("Condition");
    result.sweptParamUnit = QStringLiteral("#");

    if (!scene) return result;

    for (int i = 0; i < conditions.size(); ++i) {
        NetworkSolution sol = solveNetworkAuto(
            scene, conditions[i], inletPressurePa, inletMassFlowKgPerS);
        result.points.append({static_cast<double>(i + 1), sol});
    }

    return result;
}

// ── Tornado chart computation ────────────────────────────────────────

static double extractOutputMetric(const NetworkSolution& sol,
                                  const QString& metric)
{
    if (metric == "totalPressureDrop")
        return sol.totalPressureDrop;
    if (metric == "thrust")
        return sol.hasThrustResults ? sol.thrustResult.thrust_N : 0.0;
    if (metric == "isp")
        return sol.hasThrustResults ? sol.thrustResult.specificImpulse_s : 0.0;
    if (metric == "maxPressure")
        return sol.maxPressure();
    return sol.totalPressureDrop;
}

QVector<SensitivityResult::TornadoBar> computeTornado(
    BlockScene* scene,
    const SolverSettings& baseSettings,
    const QStringList& paramKeys,
    const QString& outputMetric,
    double inletPressurePa,
    double inletMassFlowKgPerS)
{
    QVector<SensitivityResult::TornadoBar> bars;
    if (!scene) return bars;

    // Run nominal case
    NetworkSolution nominal = solveNetworkAuto(
        scene, baseSettings, inletPressurePa, inletMassFlowKgPerS);
    double nominalOutput = extractOutputMetric(nominal, outputMetric);

    for (const auto& key : paramKeys) {
        double baseVal;
        double loVal, hiVal;

        if (key == "inletPressurePa") {
            baseVal = inletPressurePa;
            loVal = baseVal * 0.5;
            hiVal = baseVal * 2.0;
        } else if (key == "inletMassFlow") {
            baseVal = inletMassFlowKgPerS;
            loVal = baseVal * 0.5;
            hiVal = baseVal * 2.0;
        } else {
            baseVal = readParam(baseSettings, key);
            loVal = baseVal * 0.5;
            hiVal = baseVal * 2.0;
        }

        double loOutput = 0.0, hiOutput = 0.0;

        // Low-side
        {
            SolverSettings s = baseSettings;
            double pIn = inletPressurePa;
            double mIn = inletMassFlowKgPerS;
            if (key == "inletPressurePa")       pIn = loVal;
            else if (key == "inletMassFlow")    mIn = loVal;
            else                                writeParam(s, key, loVal);
            NetworkSolution sol = solveNetworkAuto(scene, s, pIn, mIn);
            loOutput = extractOutputMetric(sol, outputMetric);
        }

        // High-side
        {
            SolverSettings s = baseSettings;
            double pIn = inletPressurePa;
            double mIn = inletMassFlowKgPerS;
            if (key == "inletPressurePa")       pIn = hiVal;
            else if (key == "inletMassFlow")    mIn = hiVal;
            else                                writeParam(s, key, hiVal);
            NetworkSolution sol = solveNetworkAuto(scene, s, pIn, mIn);
            hiOutput = extractOutputMetric(sol, outputMetric);
        }

        SensitivityResult::TornadoBar bar;
        bar.paramName = key;
        bar.negativeImpact = loOutput - nominalOutput;
        bar.positiveImpact = hiOutput - nominalOutput;
        bars.append(bar);
    }

    // Sort by max absolute impact (largest first)
    std::sort(bars.begin(), bars.end(),
              [](const SensitivityResult::TornadoBar& a,
                 const SensitivityResult::TornadoBar& b) {
                  double aMax = qMax(qAbs(a.negativeImpact), qAbs(a.positiveImpact));
                  double bMax = qMax(qAbs(b.negativeImpact), qAbs(b.positiveImpact));
                  return aMax > bMax;
              });

    return bars;
}
