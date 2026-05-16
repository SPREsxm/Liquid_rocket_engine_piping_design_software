#pragma once

#include <vector>
#include <QString>

class BlockScene;
struct NetworkSolution;

struct TransientState {
    double time;
    std::vector<double> pressures;
    std::vector<double> velocities;
};

struct TransientResult {
    std::vector<TransientState> history;
    QString message;
    double maxPressure = 0.0;
    double maxPressureTime = 0.0;
    int spatialNodes = 0;
    int timeSteps = 0;
};

struct PipeSegment {
    double length;
    double diameter;
    double wallThickness;
    double youngsModulus;
    double roughness;
    double density;
    double viscosity;
    double bulkModulus;
};

struct WaterHammerParams {
    double waveSpeed;
    double pipeLength;
    double inletPressure;
    double initialVelocity;
    double closureTime; // valve closing time in seconds
    std::vector<PipeSegment> segments;
};

class TransientSolver {
public:
    TransientResult simulateWaterHammer(const NetworkSolution &steady,
                                        BlockScene *scene,
                                        double closureTime,
                                        int spatialNodes,
                                        double timeStepSeconds = -1.0); // -1 = auto CFL

    // Set target Courant number (default 0.9 for MOC stability)
    void setTargetCourant(double cfl) { m_targetCFL = cfl; }
    double targetCourant() const { return m_targetCFL; }

    // Pipe material defaults (overridden by per-block properties when available)
    void setDefaultRoughness(double r)     { m_defaultRoughness = r; }
    void setDefaultYoungsModulus(double e) { m_defaultYoungsModulus = e; }
    void setDefaultWallThickness(double t) { m_defaultWallThickness = t; }

    double computeWaveSpeed(const PipeSegment &seg) const;
    double frictionSlope(double velocity, double diameter,
                         double roughness, double density, double viscosity) const;
    double computeAdaptiveDt(double waveSpeed, double dx,
                             const std::vector<double>& velocities) const;

private:
    struct PipeParams {
        double diameter = 0.0254;
        double roughness = 0.000045;
        double density = 1141.0;
        double viscosity = 1.96e-4;
        double bulkModulus = 1.0e9;
        double youngsModulus = 2.0e11;
        double wallThickness = 0.00127;
        double inletPressure = 1.0e6;
        double initialVelocity = 0.0;
        double waveSpeed = 0.0;
    };

    PipeParams preparePipeParams(const NetworkSolution& steady,
                                  BlockScene* scene,
                                  int spatialNodes) const;
    QString formatWaterHammerMessage(const PipeParams& pp,
                                      int pathBlocks, double totalLength,
                                      double closureTime,
                                      int spatialNodes, int totalSteps,
                                      double dt, double maxPressure,
                                      double maxPressureTime) const;

    double m_targetCFL = 0.9;
    double m_defaultRoughness = 4.5e-5;
    double m_defaultYoungsModulus = 2.0e11;
    double m_defaultWallThickness = -1.0; // -1 = auto (d/20)
};
