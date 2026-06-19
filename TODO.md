# TODO — Three Open Problems

Investigated and fixed 2026-06-19. Each section has the symptom, the confirmed root cause(s), evidence gathered, and the fix applied. See `ADR.md` (ADR 9–11) for the permanent record.

---

## 1. Karman vortex street not visually shown by `lbm_test` (forces oscillate, but nothing looks like a street)

**Status**: FIXED

**Confirmed physically shedding** (before fix): `./flight_sim/lbm_test --headless --mode karman --steps 1500` reported `VORTEX SHEDDING DETECTED [PASS]` but with Strouhal number 0.08 (expected 0.15–0.21 for a circular cylinder).

**Root causes found:**

1. **Streakline seeding was broken for this test.** `LBMTest.cpp::RunSimulationStep` called `solver->step(0.001f, dummyPlaneVel, identity, perturbation, sim_time)` — the 4th argument is `step()`'s `planePos` parameter, but a small symmetry-breaking noise vector was passed in, not a real position. Inside `dispatchParticleAdvect`, `spawnPos` was derived from this argument via `spawnPos[0] += -18.0f` — a constant tuned for the main flight sim's grid (`bboxMin.x = -20`). The Karman test's grid is only `[-16, 16]`, so the computed spawn point `x ≈ -18` fell **outside the grid** every time, and 60% of particle respawns were wasted re-rolling a permanently invalid location.
2. **The intended symmetry-breaking perturbation never reached the flow.** The `perturbation` vector was computed but only ever fed into the now-dead `planePos` argument — `step()` doesn't use `planePos` for anything that affects the flow. Shedding was relying on numerical noise alone, which produced a too-low, inconsistent Strouhal number.
3. **Downstream domain was too short for the shedding wavelength.** Cylinder (D=2.4m) was centered at grid x=0 in a `32m` long box, leaving only ≈5.3D of usable downstream length before the outflow sponge — too short to show more than a fraction of one vortex pair even with perfect seeding/physics.

**Fixes applied:**
- `GPUFluidSolver::dispatchParticleAdvect` (`src/GPUFluidSolver.cpp`): `spawnPos`/`spawnRange` are now derived directly from the solver's own `quantizedGridMin/Max` (2m inside the upstream face, centered on y/z) instead of `planePos + hardcoded offset`. Works correctly for any grid size/caller, not just the main sim.
- `LBMTest.cpp::RunSimulationStep`: the perturbation now feeds into `planeVel` (which `step()` does use, via `relWindPhys = windVelocity - planeVel`), actually jittering the inflow wind. Its amplitude decays with `exp(-sim_time/0.4f)` so it seeds the initial instability without continuously forcing/locking the wake to the nudge's own frequency (an earlier non-decaying version produced a forced St of 0.416, too high).
- Cylinder moved upstream to `x = -8` (`KARMAN_CYLINDER_X` in `LBMTest.cpp`, used consistently for voxelization and rendering), giving ≈8.7D of downstream room instead of ≈5.3D.

**Verified:** `lbm_test --headless --mode karman --steps 1500` now reports Strouhal number **0.192**, inside the expected 0.15–0.21 band. The dumped velocity field shows clear alternating-sign `vy` along the wake centerline (vs. a single monotonic transition before). Visual screenshots of the interactive window show a structured, organized wake forming behind the cylinder instead of a diffuse random particle cloud.

---

## 2. Fluid visualization bounding box & voxelization drift away from the aircraft as it moves

**Status**: FIXED

**Root causes found:**

1. **`F22::drawBoundingBox` (`src/Plane.cpp`) double-applied the plane's world position.** `gpuFluidSolver->getGridBounds(bMin, bMax)` already returns absolute world-space bounds (under the world-space grid convention in `GPUFluidSolver::setGridBounds`), but `drawBoundingBox` then did `glTranslatef(gridOrigin[0], gridOrigin[1], gridOrigin[2])` (`gridOrigin == plane->wpos`) before drawing — adding the plane position a *second* time. The rendered box sat at `2×planePos + bboxMin/Max`, drifting at roughly twice the rate the aircraft actually moves. Drawn every frame whenever the aircraft is visible — this was the dominant, immediately visible "drift."
2. **Slower float-precision drift in grid tracking (ADR 7)** was already being addressed by an in-progress change in `GPUFluidSolver` (`quantizedGridMin/Max`, integer `gridOffsetX/Y/Z` tracked from a fixed `initialGridMin` instead of incrementally accumulated floats). Reviewed this logic in full: it derives `quantizedGridMin` fresh from `initialGridMin + integerOffset*dx` on every call rather than accumulating, so it cannot compound floating-point error. All voxelization/advection/render call sites already consumed `quantizedGridMin/Max` correctly.

**Fixes applied:**
- Removed the stale `glTranslatef(gridOrigin...)` in `F22::drawBoundingBox` — `bMin`/`bMax` are already world-space.
- Confirmed (via `grep`) every shader uniform call site uses `quantizedGridMin/Max`, not raw `gridMin/Max`; no other call sites needed updating.

---

## 3. Aircraft aerodynamics feel "floaty" / unrealistic, not like real flight

**Status**: FIXED

**Root cause found:** `GPUFluidSolver::dispatchForceCompute` never converted the LBM force/torque from **lattice units** to **physical Newtons**. It only undid the integer atomic-encoding scale (`* 1e-6`, inverse of `lbm_force.comp`'s `* 1000000.0`) — there was no `rho_phys * dx⁴ / dt²` LBM force conversion. Velocity gets the analogous treatment correctly (`v_scale = dx/dt ≈ 250` in `velocity_reconstruct.comp`); force had no equivalent.

**Confirmed empirically (before fix):** `aircraft_test --headless --steps 2000` (F22, mass 19700 kg, max thrust 312,000 N — realistic values) logged aerodynamic force `~0.7-8` throughout, vs. aircraft weight `19700 × 9.81 ≈ 193,257 N` — 4–5 orders of magnitude too small to matter. Gravity and thrust alone drove the motion.

**Secondary issue found:** the main sim's grid (`Plane.cpp`) used `GPUFluidSolver(96, 96, 96)` over a `32m × 16m × 16m` box, giving **non-uniform** cell spacing (`dx_x ≈ 0.333m` vs `dx_y = dx_z ≈ 0.167m`) — invalid for an isotropic D3Q19 lattice, and directly feeding into the force conversion factor. `lbm_test`/`aircraft_test` already correctly used `(128, 64, 64)` for a uniform `dx = 0.25m`, matching the `v_scale = 250` and `reinitEquilibrium`'s `u_scale = 0.001/0.25` hardcoded elsewhere — strong evidence `96` in `Plane.cpp` was simply inconsistent with the rest of the system.

**Fixes applied:**
- `Plane.cpp`: `GPUFluidSolver(96, 96, 96)` → `GPUFluidSolver(128, 64, 64)` for uniform `dx = 0.25m`, matching `lbm_test`/`aircraft_test`. The two hardcoded `32.0f/96.0f` dx computations (`Plane.cpp`, `GPUFluidSolver::step`) updated to match (the latter now derives `dx` from the live grid bounds instead of a hardcoded literal).
- `GPUFluidSolver::dispatchForceCompute`: added `forceConv = rho_air(1.225) * dx⁴ / dt²` and `torqueConv = forceConv * dx` (extra `dx` because `lbm_force.comp`'s lever arm is in lattice-cell units), applied on top of the existing integer-decode scale.

**Verified:** `aircraft_test --headless --steps 2000` now logs steady-state force around `[~4700-39000, ~10, ~1000-1300]` N — physically plausible order of magnitude (thousands of Newtons) for the test's low airspeed (10 m/s, well below cruise), versus single-digit Newtons before. Real-flight-speed validation (interactive sim, cruise AoA/speed) still recommended as a follow-up sanity check.

---

## 4. (Follow-up) Aircraft "gets wild and rotates really fast" in the flight sim after the force fix

**Status**: FIXED (see ADR 12, ADR 13)

Reported after issue #3's fix gave torque a real magnitude for the first time. Two compounding bugs, both latent until torque became non-negligible:

1. **Out-of-bounds inertia-tensor inversion** (`DynamicModel::setInertia`, `PhysicalModel.cpp`): the third row of the 3x3 `inv_inertia` was written to `&inv_inertia[9]` (past the end) instead of `&inv_inertia[6]`. The real third row stayed at the constructor default (`1/166.67` on the diagonal), so the yaw term `inv_inertia[8]` was `0.006` vs. the correct `~2.9e-6` — ~2100x too large — and the stray write clobbered the adjacent `wpos`. Any yaw torque produced runaway spin. **Fixed** by writing to `[6]`. Verified with a standalone math check: yaw angular acceleration for a 700 N*m moment drops from 4.2 rad/s^2 to 2.0e-3 rad/s^2.
2. **Torque taken about the grid center instead of the CoM** (`lbm_force.comp`): the lever arm was measured from `vec3(size)*0.5`, but the rigid-body integrator rotates about the aircraft CoM (`planePos`). These coincide in `aircraft_test` (so it looked fine) but differ by ~4m in the flight sim, injecting a spurious `r_offset x F_net` moment. **Fixed** by passing the CoM (in lattice coords) as a `com` uniform and measuring `r = pos - com`.

---

## 5. (Follow-up) "Fluid field still not going with the aircraft correctly"

**Status**: VERIFIED FIXED (bounding box + grid-tracking confirmed headlessly).

Investigated the grid-tracking/particle path again after issue #2's bounding-box fix:
- Tracer particles are stored in world space and advected by `v + gridVel` (grid-frame fluid velocity + plane velocity), which correctly keeps still-air particles fixed in world space while the grid translates with the plane; out-of-bounds particles respawn at the (moved) grid's upstream face, so the active particle region tracks the aircraft by construction.
- The solid grid follows the plane via integer-cell shifts (`setGridBounds`/`shift_grid.comp`), and all voxelization/advection/render call sites consume `quantizedGridMin/Max`.

**Verified empirically**, not just by audit: `flight_stability_test` now asserts the fluid grid stays pinned to the aircraft. The solid is re-voxelized fresh each substep at the plane's world transform, so the voxelization/particle visualization follows the plane iff `quantizedGridMin - planePos` stays constant. Over a ~990 m flight (level and all maneuvers) the measured drift is **~0.2 m — below the 0.25 m cell size**, i.e. pure quantization rounding with no unbounded drift. Combined with the bounding-box fix (item #2 / ADR 10), the visualization tracks the aircraft. The earlier "not following" was the plane tumbling away from its own correctly-following grid (issue #4). The test fails (`fluid grid drift`) if grid tracking ever regresses.

---

## 6. Automated full-flight stability regression test

**Status**: ADDED

`src/FlightStabilityTest.cpp` (target `flight_stability_test`) instantiates a real `F22`, places it at altitude flying forward at 250 m/s, applies full throttle, and runs `F22::updatePhysic` (LBM aero + rigid-body dynamics) for ~5 simulated seconds. It fails (nonzero exit) on any NaN/Inf, angular rate ≥ 10 rad/s (runaway rotation), speed ≥ 2000 m/s (force blow-up), or lack of forward progress — guarding the fixes in items #3/#4 (ADR 9, 12, 13).

Verified it both passes on the fixed code (max angular rate ~0.32 rad/s, ~1226 m forward in 5 s) **and** fails on the regression: temporarily reintroducing the inertia-inversion bug drove max angular rate to ~987 rad/s → `[FAIL] (runaway rotation)`. Run: `cd flight_sim && ./flight_stability_test --verbose`.
