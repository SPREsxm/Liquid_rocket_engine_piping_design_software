# Liquid Rocket Engine Piping Design Software — User Manual

## Overview

This software provides a graphical CAD-like environment for designing and analyzing piping systems for liquid rocket engines. It supports component placement, connection routing, fluid dynamics simulation, and transient water-hammer analysis.

## System Requirements

- Windows 10/11 (x64)
- Qt 6.8.0 runtime (MSVC 2022)
- No GPU requirements — CPU-only computation

## Quick Start

1. Launch `LiquidRocketPiping.exe`
2. The main window shows a block-diagram editor on the left and a property panel on the right
3. Drag components from the toolbar onto the canvas
4. Connect output ports (green) to input ports (blue) by clicking a source port then a destination port
5. Select a component to edit its properties (diameter, length, material, etc.)

## Component Types

| Category | Types | Description |
|----------|-------|-------------|
| Pipe | Straight, Elbow 90°, Elbow 45°, Tee | Fluid transport |
| Valve | Gate, Globe, Ball, Solenoid | Flow control |
| Pump | Centrifugal, Piston | Pressure boost |
| Sensor | Pressure, Flow | Measurement points |
| Tank | Storage, Buffer | Fluid reservoirs |

## Solver Modes

- **BFS Forward** — Fast forward-propagation solver for linear (non-looped) networks
- **Hardy-Cross** — Iterative loop-balancing solver for networks with cycles
- **Matrix** — Full incidence-matrix solver using Gauss-Seidel iteration
- **Auto** — Automatically selects BFS or Hardy-Cross based on network topology

## Running a Simulation

1. Build a network with at least one inlet, one pipe, and one outlet
2. Set the inlet pressure and mass flow rate in the solver panel
3. Click **Solve** to compute pressures and flows
4. Click **Water Hammer** to run transient analysis (valve closure surge)

## Water Hammer Analysis

The transient solver uses the Method of Characteristics (MOC) to simulate pressure surges from rapid valve closure. Key parameters:

- **Closure Time** (s) — How fast the valve closes
- **Spatial Nodes** — Grid resolution along the pipe
- **CFL Number** — Default 0.9 for MOC stability

Results include peak pressure, Joukowsky estimate, and pressure/velocity time histories.

## Physical Models

- **Fluid**: LOX/RP-1 default; configurable density/viscosity
- **Pipe friction**: Colebrook-White (turbulent), 64/Re (laminar)
- **Heat transfer**: Dittus-Boelter, Sieder-Tate, Gnielinski, Bartz (throat)
- **Turbulence**: SST k-ω model for pipe flows
- **EOS**: Simplified Setzmann-Wagner for methane; ideal gas for other propellants
- **Fluid-structure**: Hoop stress, von Mises, Korteweg wave speed
- **Mixed precision**: Adaptive float32/float64 selection based on condition number

## Expression Engine (ExprTk)

Custom formulas can be evaluated at runtime using the ExprTk expression engine. Supports:
- Scalar expressions with variables and constants
- User-defined unary/binary functions
- Numerical differentiation and integration (trapezoidal rule)
- Standard math functions (sin, cos, exp, log, sqrt, pow, etc.)

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Delete | Remove selected block/connection |
| Ctrl+S | Save project |
| Ctrl+O | Open project |
| Ctrl+N | New project |

## File Format

Projects are saved as JSON (`.lrep` extension) containing:
- Component instances with UUID, type, position, and properties
- Connections between ports
- Solver settings and results

## Building from Source

See [README.md](../README.md) for build instructions.

Requirements:
- CMake 3.20+
- MSVC 2022 (C++20)
- Qt 6.8.0 (Widgets, Svg modules)
- vcpkg (catch2, eigen3, exprtk)

```bash
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=<qt-install-dir>
cmake --build build --config Debug
ctest --test-dir build -C Debug
```
