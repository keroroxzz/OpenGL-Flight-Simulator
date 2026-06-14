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

## ADR 6: Prevent Shader State Leakage in Fluid Renderer
**Date**: 2026-06-05
**Status**: Accepted

### Context
In Wind Tunnel mode, unwanted triangles were appearing between particles. This was caused by the `particleRenderShader` remaining active after `GPUFluidSolver::drawParticles`. Subsequent draw calls for UI elements or debugging spheres (which use `GL_TRIANGLES`) were being processed by the particle shader, which interprets vertex IDs as indices into the global particle buffer.

### Decision
Explicitly call `glUseProgram(0)` at the end of all standalone rendering methods in the fluid solver, particularly `drawParticles`.

### Consequences
- Prevents visual artifacts in subsequent rendering passes.
- Ensures robust interoperability between different rendering modules (HUD, visualization, and main scene).

## ADR 5: Implement Aerodynamic Torque via Momentum Exchange
**Date**: 2026-06-14
**Status**: Accepted

### Context
The LBM solver was previously only calculating global force vectors. This allowed for lift and drag simulation but prevented the aircraft from rotating (pitch, roll, yaw) in response to control surface movements (ailerons, elevators).

### Decision
Update the `lbm_force.comp` shader to calculate torque ($r \times f$) for every fluid-solid interaction point relative to the grid center. These moments are accumulated and applied to the rigid body physics engine.

### Consequences
- The aircraft now naturally rotates when control surfaces are deflected.
- Enables full 6-DOF aerodynamic simulation.

## ADR 6: Stabilize High-Speed Flow with Extrapolated Outflow and Sponge Layers
**Date**: 2026-06-14
**Status**: Accepted

### Context
Vortices were "bouncing" off grid boundaries and exploding, causing the simulation to diverge after ~17 seconds of high-speed flow. This was due to reflective equilibrium boundary conditions.

### Decision
1. Implement **Extrapolated Outflow** in `lbm_stream.comp` to pull incoming distributions from internal neighbors, making boundaries "non-reflective."
2. Implement a **Multi-Axis Sponge Layer** to gradually absorb energy and relax distributions toward background equilibrium at the boundaries.
3. Increase base relaxation time ($\tau$) to 0.8 and add a **Density Normalization** term to prevent long-term numerical mass leakage.

### Consequences
- Simulation is now stable for indefinite periods (verified for 30,000+ steps).
- Elimination of non-physical "vortex explosions."

## ADR 7: Unified World-Space Grid Tracking for Moving Grids
**Date**: 2026-06-14
**Status**: Accepted

### Context
The LBM grid follows the aircraft, but inconsistent coordinate origins between sub-steps and frames caused "phantom voxelization" (solid ghosts drifting backwards) and vertical clipping (incorrect ground collision in Wind Tunnel mode).

### Decision
1. Pass **absolute world-space boundaries** to `setGridBounds` to trigger the `shift_grid.comp` logic correctly whenever the grid moves in the world.
2. Maintain a consistent `gridOrigin` throughout the 10x sub-stepping loop to ensure voxelization and particle advection remain registered.
3. Explicitly **disable ground voxelization** in Wind Tunnel mode ($z=0$) to ensure symmetric flow.

### Consequences
- Sharp, well-defined aircraft shape in the fluid field without smearing.
- Full-volume particle distribution in Wind Tunnel mode.
- Consistent physics across different simulation modes.
