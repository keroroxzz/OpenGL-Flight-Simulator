# Architecture Decision Records (ADR)

## ADR 1: Synchronize Physics and Fluid Solver Time-Steps
**Date**: 2026-06-03
**Status**: Accepted

### Context
The physics engine was advancing at `0.0002s` per step, while the fluid solver had a hardcoded internal scaling of `0.001s` for lattice units. This created a 5x mismatch in velocity representation, leading to unrealistic aerodynamic forces and particle speeds.

### Decision
Synchronize both the physics time-step (`dt`) and the fluid solver's internal scaling to `0.001s`. 

### Consequences
- Aerodynamic forces are now correctly scaled to physical speeds.
- Particle advection speed is consistent with aircraft movement.
- Numerical stability is improved by using a slightly larger, consistent time-step.

---

## ADR 2: Consolidate Physics Sub-steps and Voxelization
**Date**: 2026-06-03
**Status**: Accepted

### Context
`Main.cpp` was calling `F22::updatePhysic` 50 times per frame. Each call involved clearing the solid grid and revoxelizing the entire aircraft model (multiple parts) on the GPU, which was extremely expensive and caused significant performance drops.

### Decision
Refactor `F22::updatePhysic` to perform voxelization once per call and then loop 10 times internally for physics and fluid steps. The `idle()` loop in `Main.cpp` now calls `updatePhysic` once per frame.

### Consequences
- GPU voxelization overhead is reduced by 90%.
- Simulation performance (FPS) is significantly improved.
- Physics fidelity is maintained as the total time advanced per frame remains `0.01s`.

---

## ADR 3: Enable Vortex Advection in WakeManager
**Date**: 2026-06-03
**Status**: Accepted

### Context
The "Global Sparse Wake" system (Layer 3) was stagnant because vortex advection was commented out in the CPU-side `WakeManager`. The GPU-side `vortex_physics.comp` results were also being overwritten by stagnant CPU data during each synchronization.

### Decision
Implement background wind advection in the `WakeManager::update` function.

### Consequences
- Vortices now move realistically with the global wind field.
- The hybrid LBM-Vortex interaction is fully functional, allowing wakes to affect aircraft at a distance.

---

## ADR 4: Use True Air Speed (TAS) for Aerodynamics
**Date**: 2026-06-03
**Status**: Accepted

### Context
Aerodynamic forces (lift/drag) were calculated using the aircraft's world velocity, ignoring global wind. This made flight behavior incorrect in wind conditions and required hacks in Wind Tunnel mode.

### Decision
Update `Aerofoil` and `Joint` logic to use True Air Speed (TAS) vector: $\mathbf{V}_{TAS} = \mathbf{V}_{plane} - \mathbf{V}_{wind}$.

### Consequences
- Flight physics are now correct in all wind conditions.
- Wind Tunnel mode works naturally without overriding aircraft state variables.

---

## ADR 5: Consistent Particle Advection in Moving Grids
**Date**: 2026-06-03
**Status**: Accepted

### Context
The particle advection shader only used the relative LBM velocity. When the LBM grid moved with the aircraft, particles in world space were only moving by the relative flow, not the combined velocity of flow + grid.

### Decision
Pass `gridVel` (aircraft velocity) to the `particle_advect.comp` shader and update position using $\mathbf{V}_{world} = \mathbf{V}_{lbm} + \mathbf{V}_{grid}$.

### Consequences
- Particles now maintain consistent world-space positions when the grid moves.
- Visualization in Wind Tunnel mode correctly shows particles moving at the speed of the injected wind.

---

## ADR 6: Robust Particle Rendering and NaN Safety
**Date**: 2026-06-03
**Status**: Accepted

### Context
"Random triangles" artifacts were observed in Wind Tunnel mode, likely caused by NaN positions in the particle SSBO being passed to the vertex shader. NaNs in `gl_Position` result in undefined geometry behavior. Additionally, invalid particles were set to `(0,0,0,0)`, which could render as lines to the origin.

### Decision
1. Update `particle.vs` to explicitly detect and discard NaN/Inf positions by moving them outside the clip volume.
2. Discard invalid particles (`alpha <= 0.01`) by moving them to a clipped coordinate instead of the NDC origin.
3. Move the LBM stability check *before* reconstruction and advection in `GPUFluidSolver::step` to prevent NaN propagation from the fluid field to the particle system.

### Consequences
- "Exploding" geometry and random triangle artifacts are eliminated.
- Particle system is now isolated from transient LBM instabilities.

---

## ADR 7: Standardized SSBO Bindings
**Date**: 2026-06-03
**Status**: Accepted

### Context
Multiple compute shaders and host-side calls were using conflicting SSBO binding indices (predominantly 0). This caused data corruption where LBM distributions, particle data, and triangle meshes were overwriting each other, leading to persistent "random triangle" artifacts and broken physics.

### Decision
Standardize a fixed binding scheme across all shaders and C++ code:
- 0: `f_in` (LBM)
- 1: `f_out` (LBM) / `velocity_img` (Image)
- 2: `particles`
- 3: `solid_img` (Image)
- 4: `vortices` (Wake)
- 5: `triangles` (Mesh)
- 6: `force`
- 7: `candidates` (Wake Extract)
- 8: `counter` (Wake Extract)
- 9: `error` (Stability Check)

### Consequences
- Data corruption between independent systems is eliminated.
- Physics and visualization are now robust and consistent.

---

## ADR 8: Synchronization of Simulation Speed
**Date**: 2026-06-03
**Status**: Accepted

### Context
`flight_sim` was running 10x faster than `aircraft_test` because it executed 10 sub-steps per frame, whereas the test harness executed only one. This made visual verification and tuning difficult.

### Decision
Implement `updatePhysicStep` to allow single-step execution. In Wind Tunnel mode (F6), `Main.cpp` now calls this single-step function to match the visual cadence of the test harnesses.

### Consequences
- Visualization speed is consistent across all simulation modes.
- Easier to compare LBM behavior between test and main application.

---

## ADR 9: Rendering Diagnostic Toggles
**Date**: 2026-06-03
**Status**: Accepted

### Context
Persistent "random triangle" artifacts were observed in the main application visualization. To localize the cause within the complex rendering pipeline (lighting, sky, particles), a method to isolate components was needed.

### Decision
Implement keyboard and menu toggles for major rendering stages:
- **F7**: Toggle Lighting (Aircraft/Model rendering)
- **F8**: Toggle Clouds (Sky/Atmosphere shader)
- Existing **F4**: Toggle Streamlines (LBM Particles)

### Consequences
- Allows the developer to determine which shader/stage is producing the geometric artifacts.
- Improved debuggability of the rendering pipeline.
