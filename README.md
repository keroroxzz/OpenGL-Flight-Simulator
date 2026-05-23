![.](https://live.staticflickr.com/65535/53265252304_3be65ecef1_c_d.jpg)
# OpenGL Flight Simulator

It's a C++ & OpenGL-based ambitious project to utilize real-time aerodynamic simulation and realistic volumetric cloud simulation. 

![.](https://live.staticflickr.com/65535/53265252304_3be65ecef1_c_d.jpg)

## 🚀 Key Features

1.  **Multiscale Hybrid Aerodynamics**: A high-fidelity physics engine combining Eulerian LBM (Local Grid) and Lagrangian Vortex Particles (Global Wake).
2.  **GPU-Driven Pipeline**: Collision, streaming, and force calculations are fully offloaded to Compute Shaders.
3.  **Real-time Voxelization**: Dynamic 3D model voxelization on the GPU for precise fluid-structure interaction (FSI).
4.  **Realistic Volumetric Clouds**: Immersive weather simulation and daytime lighting.
5.  **Robust Physics**: Integrated NaN-safety mechanisms and automatic fluid field stabilization.

## 🛠 Compilation & Execution

### Dependencies
Install OpenGL and development libraries:
```bash
sudo apt-get install build-essential libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev libglew-dev
```

### Build
```bash
cmake . && make -j4
```

### Run
*   **Main Simulator**: `./flight_sim/flight_sim`
*   **Physics Test (Karman Vortex Street)**: `./flight_sim/lbm_test`
*   **Rendering Integrity Test**: `./flight_sim/particle_test`

## 🕹 Controls

### Flight Control
*   **Thrust**: `W` / `S`
*   **Pitch/Roll**: Mouse Movement
*   **Camera Orbit**: Mouse Drag (Test modes)

### Environment & Debug
*   **Pause/Start**: `F1`
*   **Debug Overlays**: `F2`
*   **Reload Shaders**: `F3`
*   **Move Sun**: `Q` / `E`

## 🧬 Technical Highlights (Recent Refactor)

*   **Hybrid Solver**: Implemented Layer 1 (Local LBM) and Layer 3 (Global Sparse Wake) as per `REFACTOR.md`.
*   **NaN-Safe Particles**: Fixed a critical bug where particles would permanently disappear due to NaN values; added robust `isnan()` / `isinf()` checks and forced respawning in Compute Shaders.
*   **Optimized Resource Management**: Introduced `triSSBO` caching for 3D models and GPU-side vertex transformation to eliminate CPU-GPU bottlenecks.
*   **Vertex Pulling Rendering**: Utilized zero-copy rendering from SSBOs to display millions of tracer particles with dynamic tails.

## 📜 License & Declaration

This project is an experimental side project. All rights of specific source codes (`gltools`, `math3D`) are reserved by Richard S. Wright Jr. 
The F22 texture is by [I. Mikhelevich](https://www.the-blueprints.com/blueprints/modernplanes/lockheed/45834/view/lockheed_martinboeing_f-22_raptor/).
The cloudwork source code is under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.