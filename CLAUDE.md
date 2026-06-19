# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A C++11 / OpenGL 4.3 flight simulator combining real-time GPU aerodynamics (LBM fluid solver + Lagrangian vortex wake) with rendering (volumetric clouds, terrain, particle-based wake visualization). The full target architecture (three-layer hybrid: local moving-grid LBM, vortex extraction, global sparse wake) is documented in `REFACTOR.md` (Chinese). Implementation decisions and their rationale are logged chronologically in `ADR.md` — **read the latest entries there before touching the fluid solver, grid tracking, or aerodynamics code**, and add a new entry for any significant physics/architecture change you make.

## Build

```bash
cmake . && make -j4
```
Binaries land in `flight_sim/` (not a separate `build/` output dir — `CMakeLists.txt` routes `RUNTIME_OUTPUT_DIRECTORY` there). All executables `chdir` into `flight_sim/` at startup to find `shaders/`, `f22/`, `map/` assets relatively, so run them from repo root or from inside `flight_sim/` — both work.

There is no lint config and no automated test runner wired into CMake (no `ctest`/CI). Verification is done via the standalone test executables below.

## Executables

| Target | Source | Purpose |
|---|---|---|
| `flight_sim/flight_sim` | `src/Main.cpp` + `Plane.cpp` | Main interactive simulator |
| `flight_sim/lbm_test` | `src/LBMTest.cpp` | Standalone LBM solver test (Karman vortex / lid-driven cavity / backward step) |
| `flight_sim/aircraft_test` | `src/AircraftTest.cpp` | Standalone aircraft aerodynamics test |
| `flight_sim/particle_test` | `src/ParticleTest.cpp` | Rendering-only integrity test (no physics) |
| `flight_sim/flight_stability_test` | `src/FlightStabilityTest.cpp` | Headless full-flight stability check (real F22 aero + rigid body, full throttle); exit 0 = PASS |

All of these are GLUT apps that still create a real window (no true headless/EGL context despite `GEMINI.md` describing that as an aspiration) — running them requires a display or `Xvfb`. `lbm_test` and `aircraft_test` support a `--headless --steps N` flag pair: with it, they skip interactive input, run `N` fixed-timestep iterations, dump raw field/force data as text to `flight_sim/test_out/`, and exit. This is the way to validate solver changes without eyeballing the renderer — e.g.:

```bash
cd flight_sim && ./lbm_test --headless --mode karman --steps 500
cd flight_sim && ./aircraft_test --headless --steps 5000
cd flight_sim && ./flight_stability_test --steps 500 --verbose   # full-flight stability; returns nonzero on FAIL
cd flight_sim && ./flight_stability_test --maneuver turn-right    # exercise a control-surface maneuver
```

`flight_stability_test` is the closest thing to an end-to-end regression test: it flies a real `F22` forward at full throttle for several simulated seconds and fails (nonzero exit) on NaN/Inf, runaway rotation, speed blow-up, or the fluid grid drifting off the aircraft — i.e. it guards the aerodynamic-force unit scaling, the inertia/torque rigid-body fixes, and the moving-grid visualization tracking (ADR 9, 10, 12, 13). `--maneuver <level|pitch-up|pitch-down|roll-left|roll-right|yaw-left|yaw-right|turn-left|turn-right>` (or explicit `--pitch/--roll/--yaw` in [-1,1]) holds a control-surface deflection through the run, additionally requiring the flight path to actually respond; the per-step trajectory + attitude ("trail") is dumped to `test_out/flight_trail_<maneuver>.txt`.

Inspect the dumped `.txt` files in `test_out/` for NaN/Inf, mass conservation (density should stay near 1.0), and expected force/velocity magnitudes.

`particle_test` isolates the render pipeline from physics — use it to tell apart a physics bug from an SSBO/vertex-pulling/shader bug per the "Isolate & Locate" workflow in `GEMINI.md`.

## Runtime controls (main `flight_sim`, ground truth is `src/Main.cpp`, not `README.md`)

- `W`/`S`: thrust (or pan camera focus, in Wind Tunnel mode)
- `[`/`]`: decrease/increase background wind X velocity
- `F1`: pause/resume
- `F2`: toggle force vector overlay
- `F3`: reload sky/lighting shaders
- `F4`: toggle aircraft visibility
- `F5`: toggle solid-voxel grid debug view
- `F6`: toggle Wind Tunnel mode (resets plane to origin, re-initializes equilibrium)
- `Page Up`/`Page Down`: displace aircraft in Z
- `Q`: quit
- Right-click: context menu (simulation/visualization/wind tunnel submenus)

## Architecture

### Physics: two solvers, GPU one is live

`includes/FluidSolver.h` is an older CPU Navier-Stokes solver, mostly superseded. **`GPUFluidSolver`** (`includes/GPUFluidSolver.h` / `src/GPUFluidSolver.cpp`) is the active solver — a D3Q19 Lattice Boltzmann Method (LBM) implementation running entirely in compute shaders under `flight_sim/shaders/compute/`. Key shaders:

- `lbm_collision.comp` — BGK collision with Smagorinsky LES turbulence model (dynamically adjusts relaxation time `tau` from local strain rate).
- `lbm_stream.comp` — streaming step; handles inflow/outflow boundaries (extrapolated/non-reflective outflow) and a sponge layer that relaxes cells near grid edges toward background equilibrium to prevent reflected-vortex blowup.
- `lbm_force.comp` — momentum-exchange method (CMEM) computing aerodynamic force *and* torque (`r × f` about grid center) from solid-boundary interactions.
- `voxelize.comp` / `voxelize_cylinder.comp` / `voxelize_ground.comp` — rasterize triangle meshes / primitives into the solid-cell texture each substep.
- `particle_advect.comp` + `shaders/vertex/particle.vs` — advects visual tracer particles through the velocity field for wake visualization; rendered via vertex pulling (`gl_VertexID` indexes an SSBO directly, no VBO upload).
- `wake_extract.comp` / `wake_inject.comp` / `vortex_physics.comp` — Layer 2/3 of the hybrid architecture (extracting vorticity at grid boundaries into a global sparse `WakeManager`/vortex-particle system; see `includes/WakeSystem.h`). This layer is partially implemented relative to the full design in `REFACTOR.md`.

### Moving grid, world-space coordinates

The LBM grid is a fixed-resolution box that translates (not rotates) with the aircraft. `GPUFluidSolver::setGridBounds` decides when to shift the grid by whole lattice cells (`dispatchShift` → `shift_grid.comp`) and tracks the cumulative integer cell offset from a fixed `initialGridMin`, rather than accumulating floating-point deltas — this avoids long-run float drift in grid registration ("phantom voxelization", see ADR 7). `quantizedGridMin/Max` (derived from the integer offset) are what get passed to the voxelization/advection/render shaders, not the raw float `gridMin/Max`. Tracer particle positions are stored in **world space**, not grid-relative space.

### Rigid body / control surfaces

`F22` (`includes/Plane.h` / `src/Plane.cpp`) owns a hierarchy of `DynamicModel` parts (`includes/PhysicalModel.h`) connected by `Joint`s (`includes/Joints.h`: `FixedJoint`, `RevoluteJoint` for ailerons/flaps/rudder/elevators). `F22::updatePhysic` runs a fixed sub-step loop (10 iterations of `dt=0.001s` per call) that, **in order each substep**: (1) recomputes hierarchical world transforms (`updatePositionVelocity`) so control-surface deflections are reflected immediately (ADR 8 — this must happen *before* voxelization, not after), (2) moves the grid to follow the plane, (3) re-voxelizes every part using its current `waxis`, (4) steps the LBM, (5) reads back CFD force/torque, checks for NaN (triggers `resetParticles()` rather than propagating), and integrates rigid-body dynamics. Aerodynamics use **True Air Speed** (`plane velocity - wind`, ADR 4), not raw world velocity, so wind tunnel mode and free flight share the same physics path.

### Math/GL conventions

Project-wide types (`M3DVector3f`, `M3DMatrix44f`, etc.) and helpers (`m3dCopyVector3`, `m3dAddVectors3`, ...) come from `includes/gl/shared/math3d.h` via `includes/baseHeader.h`, which every header includes first. Don't reach for `glm` — it's not used in this codebase.

## Working notes

- When changing LBM stability parameters (`tau` floor, sponge layer width/strength, density damping in `lbm_collision.comp`/`lbm_stream.comp`), be aware these were tuned together for stability at the current grid resolution/timestep — changing one in isolation (e.g. lowering `tau` floor for less numerical viscosity) can reintroduce the divergence issues ADR 6 fixed. Validate with `lbm_test --headless` before committing.
- Grid box dimensions (`bboxMin`/`bboxMax` in `F22::updatePhysic`) must stay consistent with `NX/NY/NZ` passed to `GPUFluidSolver`'s constructor and the `dx_lbm` used for lattice-unit wind scaling — these are currently hardcoded in multiple places rather than derived from one source of truth.
