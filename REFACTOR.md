你的需求其實已經不是「一份說明文件」而已，而是：

# 給 AI Agent 的「可直接施工的系統架構規格書」

所以我下面會用：

* production-oriented
* AI-agent-friendly
* 可逐步實作
* 可拆分 task
* 可驗證

的方式來寫。

這份不是論文。

而是：

# 真正能讓 Claude / GPT / Cursor Agent 開始改專案的實戰規格。

---

# OpenGL Flight Simulator

# Real-Time GPU Aerodynamics System

# Implementation Specification v1.0

---

# 0. 專案核心目標

本系統目標：

在現有 OpenGL Flight Simulator 專案中加入：

# 「局部 GPU 流體模擬 + 世界稀疏尾流系統」

以提供：

* 真實風場
* 翼尖渦流
* wake turbulence
* ground effect
* airflow visualization
* aerodynamic feedback

並維持：

```text id="jlwmng"
RTX 4070 + Ryzen 2600
>= 30 FPS
target: 60~144 FPS
```

---

# 1. 系統總體架構

# 1.1 系統分層

```text id="fhv2j6"
┌────────────────────────────┐
│ CPU Flight Physics         │
│ (6DOF rigid body)          │
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│ GPU Local LBM CFD          │
│ (near-field airflow)       │
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│ Wake Extraction Layer      │
│ (vorticity compression)    │
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│ Sparse World Wake System   │
│ (persistent turbulence)    │
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│ GPU Particle Visualization │
│ (streamlines + vortices)   │
└────────────────────────────┘
```

---

# 2. 絕對禁止事項（重要）

AI Agent 必須遵守：

---

# 不要：

## ❌ Full-world CFD

## ❌ CPU fluid simulation

## ❌ FEM/FVM Navier-Stokes solver

## ❌ Pressure Poisson iterative solver

## ❌ CUDA-only implementation

## ❌ Per-frame full mesh remeshing

## ❌ Full aerodynamic force replacement

---

# 必須：

## ✅ GPU-only simulation

## ✅ OpenGL 4.6 compute shader

## ✅ Local moving CFD volume

## ✅ Hybrid flight model

## ✅ Sparse wake representation

## ✅ Temporal force filtering

---

# 3. OpenGL Requirements

# 3.1 必須升級 OpenGL Context

需要：

```cpp
glutInitContextVersion(4, 6);
glutInitContextProfile(GLUT_CORE_PROFILE);
```

最低：

```text id="3wp4cx"
OpenGL 4.3
```

推薦：

```text id="0g1ev4"
OpenGL 4.6
```

---

# 3.2 Required Extensions

必須支援：

```text id="1bqlwl"
GL_ARB_compute_shader
GL_ARB_shader_storage_buffer_object
GL_ARB_shader_image_load_store
GL_ARB_multi_draw_indirect
```

---

# 4. Local CFD System

# 4.1 核心設計

模擬：

# 飛機附近局部風場

而不是：

# 整個世界。

---

# 4.2 Simulation Volume

第一版：

```cpp
const ivec3 CFD_GRID_SIZE = ivec3(96, 96, 96);
```

後續：

```cpp
128³
```

---

# 4.3 Moving Local Frame

CFD Volume：

* 跟隨飛機平移
* 不跟隨旋轉

---

# 4.4 Grid Space

推薦：

```text id="44gb35"
1 voxel = 0.25m ~ 0.5m
```

---

# 4.5 LBM Method

使用：

# D3Q19 MRT-LBM

不要 BGK-only。

---

# 4.6 Required GPU Buffers

---

## Distribution Buffer

```cpp
RGBA16F 3D textures
or
SSBO
```

內容：

```text id="yodzy0"
19 distribution functions
```

---

## Velocity Field

```cpp
RGBA16F 3D texture
```

內容：

```text id="s0rlnu"
xyz = velocity
w = pressure/density
```

---

## Solid Occupancy Grid

```cpp
R8UI 3D texture
```

```text id="oixx9k"
0 = fluid
1 = solid
```

---

# 5. GPU Compute Pipeline

# 每 frame：

---

# Step 1 — Aircraft voxelization

輸入：

```text id="jlwmng"
aircraft mesh
transform
```

輸出：

```text id="jlwmng"
solid occupancy grid
```

---

# Step 2 — LBM collision

Compute shader：

```text id="jlwmng"
lbm_collision.comp
```

---

# Step 3 — LBM streaming

Compute shader：

```text id="jlwmng"
lbm_stream.comp
```

---

# Step 4 — Velocity reconstruction

Compute shader：

```text id="jlwmng"
velocity_reconstruct.comp
```

---

# Step 5 — Wake extraction

Compute shader：

```text id="jlwmng"
wake_extract.comp
```

---

# Step 6 — Particle advection

Compute shader：

```text id="jlwmng"
particle_advect.comp
```

---

# 6. LBM Collision Shader

# 6.1 Required Equations

使用：

# MRT collision model

---

# 6.2 Turbulence Model

使用：

# Smagorinsky LES

---

# 6.3 Stability Constraints

AI Agent 必須：

---

## Clamp velocity magnitude

```glsl
velocity = clamp(length(v), 0.0, MAX_VELOCITY);
```

---

## Clamp density

```glsl
density = max(density, MIN_DENSITY);
```

---

## Clamp relaxation time

```glsl
tau = clamp(tau, 0.51, 2.0);
```

---

# 6.4 Mach Number Limit

禁止：

```text id="jlwmng"
Mach > 0.3
```

LBM 第一版：

只做：

# incompressible low-Mach airflow。

---

# 7. Aircraft Boundary Handling

# 7.1 不要 triangle CFD mesh

使用：

# voxel occupancy only

---

# 7.2 Thin Wing Problem

AI Agent 必須：

---

## Minimum wing thickness rule

若：

```text id="jlwmng"
wing thickness < 2 voxels
```

則：

* 自動擴張 occupancy
* 避免 leakage

---

# 7.3 Boundary Method

使用：

# bounce-back boundary

第一版不要 immersed boundary。

---

# 8. Aerodynamic Force System

# 8.1 第一版禁止 full CFD flight

必須：

# Hybrid Flight Model

---

# 8.2 Final Force

```text id="jlwmng"
F_final =
F_classic
+
F_cfd_correction
```

---

# 8.3 CFD Correction Sources

只允許：

* turbulence
* wake disturbance
* ground effect
* stall correction
* vortex interaction

---

# 8.4 Temporal Filtering（重要）

所有 CFD force：

必須 low-pass filter。

---

## Example

```cpp
filteredForce =
lerp(prevForce, currentForce, 0.08f);
```

---

# 8.5 禁止事項

第一版：

## ❌ 不要直接用 CFD 算全部 lift

## ❌ 不要直接取代 flight physics

---

# 9. Wake Extraction System

# 9.1 核心理念

不要保存：

# 世界 velocity field

只保存：

# sparse wake structures

---

# 9.2 Wake Entity Structure

```cpp
struct WakeVortex
{
    vec3 position;

    vec3 direction;

    float circulation;

    float radius;

    float turbulence;

    float age;

    float decayRate;
};
```

---

# 9.3 Wake Source

從：

```text id="jlwmng"
high vorticity regions
```

生成 wake。

---

# 9.4 Wake Lifetime

推薦：

```text id="jlwmng"
5~30 seconds
```

---

# 9.5 Wake Decay

```cpp
wake.circulation *= exp(-decay * dt);
```

---

# 9.6 World Wake Storage

使用：

```text id="jlwmng"
sparse vector/list
```

不要：

```text id="jlwmng"
3D world grid
```

---

# 10. Wake Interaction

# 10.1 Aircraft sampling

每 frame：

飛機查詢：

```text id="jlwmng"
nearby wake entities
```

---

# 10.2 Spatial Acceleration

使用：

# spatial hash grid

不要 brute force。

---

# 10.3 Wake Influence

使用：

# simplified Biot-Savart approximation

\mathbf{u}(\mathbf{x}) = \frac{\Gamma}{4\pi} \frac{\mathbf{dl} \times \mathbf{r}}{|\mathbf{r}|^3}

---

# 10.4 Inject Into Local CFD

world wake：

不直接控制飛機。

而是：

# 擾動 local airflow。

---

# 11. Particle Visualization

# 11.1 Particle Count

第一版：

```text id="jlwmng"
100k ~ 300k particles
```

後續：

```text id="jlwmng"
1 million
```

---

# 11.2 Storage

使用：

# SSBO

---

# 11.3 Rendering

使用：

# programmable vertex pulling

---

# 11.4 Particle Update

Compute shader：

```text id="jlwmng"
particle_advect.comp
```

---

# 11.5 Particle Integration

使用：

# RK2 integration

不要 Euler integration。

---

# 12. Performance Constraints

# 12.1 GPU Budget

CFD 全系統：

```text id="jlwmng"
target <= 6ms
hard limit <= 10ms
```

---

# 12.2 CPU Budget

```text id="jlwmng"
<= 2ms
```

---

# 12.3 VRAM Budget

```text id="jlwmng"
<= 1GB
```

---

# 13. Debug Visualization

AI Agent 必須加入：

---

# Required Debug Modes

---

## Velocity magnitude

```text id="jlwmng"
heatmap
```

---

## Pressure field

```text id="jlwmng"
density visualization
```

---

## Vorticity visualization

```text id="jlwmng"
curl(u)
```

---

## Solid occupancy grid

---

## Wake entities

---

## Force vectors

---

# 14. Numerical Stability Rules

# AI Agent 必須：

---

## Use fixed timestep

```cpp
const float dt = 1.0f / 120.0f;
```

---

## Never variable timestep CFD

---

## CFL-like safety constraint

限制：

```text id="jlwmng"
velocity * dt / dx < 0.2
```

---

## Auto reset unstable regions

若：

```text id="jlwmng"
NaN detected
density explosion
```

則：

重設 local cells。

---

# 15. Recommended Development Order

# Phase 1

## Minimal GPU LBM

* static box
* smoke flow
* no aircraft

---

# Phase 2

## Moving volume

* moving CFD box
* stable advection

---

# Phase 3

## Aircraft voxelization

* solid boundaries
* wake generation

---

# Phase 4

## Particle visualization

* streamlines
* vorticity rendering

---

# Phase 5

## Wake persistence

* sparse wake entities
* wake interaction

---

# Phase 6

## Aerodynamic correction

* turbulence forces
* wake buffeting
* ground effect

---

# 16. 第一版成功標準（重要）

第一版成功條件：

---

# 必須成功：

## ✅ 穩定 60 FPS

## ✅ 翼尖渦流可視化

## ✅ wake persistence

## ✅ wake turbulence interaction

## ✅ no simulation explosion

## ✅ stable GPU memory usage

---

# 暫時不要求：

## ❌ supersonic flow

## ❌ compressible CFD

## ❌ shock waves

## ❌ full aerodynamic realism

## ❌ transonic physics

## ❌ perfect stall model

---

# 17. 最終核心原則（重要）

AI Agent 必須遵守：

---

# 本系統的目標不是：

# scientific CFD solver

---

# 而是：

# perceptually believable real-time aerodynamics

---

# 一切設計決策：

優先順序如下：

```text id="jlwmng"
stability
>
frame rate
>
visual believability
>
physical realism
```

---

# 18. 最終推薦技術棧

| 系統                   | 技術                     |
| -------------------- | ---------------------- |
| Graphics API         | OpenGL 4.6             |
| GPU Compute          | GLSL Compute Shader    |
| CFD                  | D3Q19 MRT-LBM          |
| Turbulence           | Smagorinsky LES        |
| Geometry             | voxel occupancy        |
| Wake                 | sparse vortex entities |
| Rendering            | SSBO + vertex pulling  |
| Physics              | hybrid flight model    |
| Spatial Query        | hash grid              |
| Particle Integration | RK2                    |
| Memory               | FP16 mixed precision   |
