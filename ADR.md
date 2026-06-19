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

## ADR 8: Hierarchical Linkage Synchronization for Voxelization
**Date**: 2026-06-14
**Status**: Accepted

### Context
Aircraft control surfaces (ailerons, elevators, etc.) are implemented as a hierarchy of joints. Previously, their world-space matrices (`waxis`) were only updated during the rendering pass. This meant that the voxelization step in the physics loop was using control surface positions that were one frame old, leading to delayed aerodynamic response.

### Decision
Move the call to `updatePositionVelocity` to the beginning of each 0.001s sub-stepping loop in `F22::updatePhysic`. This ensures that all hierarchical world matrices are recalculated based on the latest joint angles and base aircraft position *before* voxelization occurs.

### Consequences
- Voxelization now reflects the "real-time" status of all control surfaces in every sub-step.
- Eliminates the 1-frame latency between pilot input and aerodynamic force changes.
- Ensures physical consistency between the rigid body's visual state and its representation in the fluid grid.

## ADR 9: Convert LBM Force/Torque to Physical Units (Newtons)

**Date**: 2026-06-19
**Status**: Accepted

### Context
`GPUFluidSolver::dispatchForceCompute` read back the momentum-exchange sum from `lbm_force.comp` and only undid the shader's integer atomic-encoding scale (`*1e-6`, inverse of `*1000000.0`). It never applied the standard LBM force conversion (`rho_phys * dx^4 / dt^2`) needed to turn a lattice-unit momentum-exchange sum into Newtons — unlike velocity, which already gets the analogous `v_scale = dx/dt` treatment in `velocity_reconstruct.comp`. Measured with `aircraft_test --headless`, aerodynamic force came out as single-digit Newtons against an F22 weight of ~193,257 N and max thrust of 312,000 N — 4-5 orders of magnitude too small to affect rigid-body dynamics. The aircraft was effectively flying under gravity and thrust alone, which read as "floating in space" rather than real aerodynamics.

Separately, the main simulator's `GPUFluidSolver` was constructed with `(96, 96, 96)` cells over a `32m x 16m x 16m` grid box, giving non-uniform per-axis cell spacing (`dx_x ~= 0.333m` vs `dx_y = dx_z ~= 0.167m`) — invalid for an isotropic D3Q19 lattice, and directly corrupting the force conversion factor above. `lbm_test` and `aircraft_test` already used `(128, 64, 64)` for a uniform `dx = 0.25m`, matching other hardcoded conversion constants elsewhere in the solver (`v_scale = 250`, `reinitEquilibrium`'s `u_scale = 0.001/0.25`).

### Decision
1. Add `forceConv = rho_air(1.225) * dx^4 / dt^2` and `torqueConv = forceConv * dx` (extra `dx` because the lever arm in `lbm_force.comp` is in lattice-cell units, not meters) to `dispatchForceCompute`, applied on top of the existing integer-decode scale.
2. Change the main simulator's grid from `GPUFluidSolver(96, 96, 96)` to `GPUFluidSolver(128, 64, 64)` to restore uniform `dx = 0.25m`, matching `lbm_test`/`aircraft_test`. Updated the two hardcoded `32.0f/96.0f` dx computations (`Plane.cpp`, `GPUFluidSolver::step`) to match; the latter now derives `dx` from the live grid bounds instead of a literal.

### Consequences
- `aircraft_test --headless` now reports steady-state aerodynamic force in the thousands of Newtons (physically plausible for its 10 m/s test airspeed) instead of single digits.
- Aerodynamic force/torque can now meaningfully compete with gravity and thrust in the rigid-body integration, restoring real lift/drag/control-surface authority.
- Real-flight-speed (cruise AoA/speed) validation in the interactive sim is recommended as a follow-up sanity check.

## ADR 10: Fix Double-Translated Bounding Box Debug Visualization

**Date**: 2026-06-19
**Status**: Accepted

### Context
`F22::drawBoundingBox` (the yellow debug box drawn whenever the aircraft is visible) called `gpuFluidSolver->getGridBounds(bMin, bMax)`, which — under the world-space grid convention introduced for moving-grid tracking (ADR 7, `quantizedGridMin/Max`) — already returns absolute world-space bounds including the plane's position. The function then additionally did `glTranslatef(gridOrigin[0], gridOrigin[1], gridOrigin[2])` (`gridOrigin == plane->wpos`) before drawing, adding the plane's position a second time. The box was rendered at `2 x planePos + bboxMin/Max`, drifting at roughly twice the rate the aircraft actually moved from the origin — the dominant, immediately visible cause of the reported "bounding box drifts away as the aircraft moves" symptom.

### Decision
Remove the stale `glTranslatef(gridOrigin...)` call in `drawBoundingBox`; `bMin`/`bMax` are drawn directly since they are already world-space.

### Consequences
- The debug bounding box now stays registered with the aircraft regardless of how far it has flown from the origin.
- Confirmed (via grep) that every other call site already passes `quantizedGridMin/Max` to its shaders, so no other rendering path had the same bug.

## ADR 11: Fix Karman Vortex Street Visualization (Seeding, Forcing, Domain Length)

**Date**: 2026-06-19
**Status**: Accepted

### Context
`lbm_test --mode karman` detected vortex shedding via oscillating cylinder force (Strouhal 0.08, below the expected 0.15-0.21 physical range) but the particle visualization never showed a recognizable alternating vortex street. Three compounding bugs were found:
1. `LBMTest.cpp::RunSimulationStep` passed a small perturbation vector into `step()`'s `planePos` parameter. `dispatchParticleAdvect` derived its upstream particle-seeding position (`spawnPos`) from that same parameter via a `-18.0f` offset tuned for the main sim's `[-20, 12]` grid; the Karman test's `[-16, 16]` grid put the computed spawn point outside its own bounds, so the intended upstream "smoke streak" never existed — particles only ever populated the domain via the uniform-random fallback.
2. The perturbation vector itself was never connected to anything that affects the flow (`planePos` is otherwise unused by `step()`), so vortex shedding relied on numerical noise alone, producing a weak and inconsistent shedding frequency.
3. The cylinder sat dead-center in a 32m grid, leaving only ~5.3 diameters of usable downstream length before the outflow sponge layer — too short to contain more than a fraction of one shedding wavelength (~30m at the measured Strouhal number), so even perfect seeding/physics couldn't visually read as a repeating "street."

### Decision
1. `GPUFluidSolver::dispatchParticleAdvect` now derives `spawnPos`/`spawnRange` directly from the solver's own `quantizedGridMin/Max` (2m inside the upstream face, centered on y/z) instead of `planePos + hardcoded offset`, so it is correct for any grid configuration.
2. `LBMTest.cpp`'s perturbation now feeds into `planeVel` (which `step()` does use, via `relWindPhys = windVelocity - planeVel`), actually jittering the inflow boundary condition. Its amplitude decays with `exp(-sim_time/0.4f)` so it seeds the initial symmetry break without continuously forcing/locking the wake to the perturbation's own frequency.
3. The Karman test cylinder moved upstream to `x = -8` within its existing grid (`KARMAN_CYLINDER_X`), giving ~8.7 diameters of downstream room instead of ~5.3.

### Consequences
- `lbm_test --headless --mode karman` now reports Strouhal number 0.192, inside the expected 0.15-0.21 band (a non-decaying version of the perturbation fix alone over-forced lock-in to 0.416, confirming the decay was necessary).
- The dumped velocity field shows clear alternating-sign transverse velocity along the wake centerline, instead of a single monotonic transition.
- Interactive visualization shows a structured, organized wake forming behind the cylinder instead of a diffuse randomly-seeded particle cloud.

## ADR 12: Fix Out-of-Bounds Inertia Tensor Inversion (Wild Aircraft Rotation)

**Date**: 2026-06-19
**Status**: Accepted

### Context
After ADR 9 gave aerodynamic torque a correct physical magnitude (previously ~1e5 too small), the aircraft began rotating violently in the flight sim. Root cause was a latent indexing bug in `DynamicModel::setInertia` (`PhysicalModel.cpp`): `inv_inertia` is a 3x3 matrix (9 floats), but the third row was copied to `&inv_inertia[9]` (one row past the end) instead of `&inv_inertia[6]`. This had two effects: (1) the real third row `[6],[7],[8]` was never written and kept the constructor's default `1/166.67` diagonal, and (2) the copy wrote out of bounds into the adjacent `wpos` member. `applyTorque` reads `inv_inertia[6],[7],[8]` for every torque component, so the inverted yaw inertia term `inv_inertia[8]` was `0.006` instead of the correct `~2.9e-6` — roughly 2100x too large. While torque was negligible (pre-ADR-9) this never mattered; once torque was real-scale, any yaw moment produced runaway spin.

### Decision
Copy the third inverse-inertia row to `&inv_inertia[6]` (the correct in-bounds location).

### Consequences
- Verified with a standalone math check: for a representative 700 N*m yaw torque, yaw angular acceleration drops from 4.2 rad/s^2 (buggy) to 2.0e-3 rad/s^2 (fixed) - the ~2100x correction matching the inverted-inertia error.
- The aircraft no longer spins up uncontrollably; aerodynamic moments now produce physically reasonable rotational response.

## ADR 13: Take Aerodynamic Torque About the Center of Mass, Not the Grid Center

**Date**: 2026-06-19
**Status**: Accepted

### Context
`lbm_force.comp` computed the momentum-exchange torque with a lever arm measured from the LBM grid center (`vec3(size)*0.5`). The rigid-body integrator (`applyTorque`) applies moments about the aircraft's center of rotation (`position`/`wpos`) using the inertia tensor defined about that point. These two reference points coincide in `aircraft_test` (planePos = origin, model centered, CoM lands exactly at the grid center) but differ by ~4m in the flight sim, where the grid box (`[-20, 12]` in x) puts the aircraft origin at lattice (80,32,32) while the grid center is (64,32,32). Taking torque about a point offset from the CoM injects a spurious `r_offset x F_net` moment; with the now-physical net aero force, that is a persistently-biased torque of order 1e3-1e5 N*m that compounds the rotation instability.

### Decision
Pass the center of rotation (the aircraft CoM == `planePos`, converted to lattice-cell coordinates via `(planePos - quantizedGridMin)/dx`) into `lbm_force.comp` as a `com` uniform, and measure the torque lever arm `r = pos - com` from it.

### Consequences
- Torque is now reference-consistent with the rigid-body integrator regardless of where the aircraft sits within its grid box.
- `aircraft_test` torque is unchanged (CoM already coincided with the grid center there), confirming the change is a no-op for the symmetric case and only removes the spurious term in the offset (flight-sim) case.
