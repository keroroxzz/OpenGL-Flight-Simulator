# OpenGL Flight Simulator

It's a C++ & OpenGL-based ambitious project to utilize real-time aerodynamic simulation (hybrid LBM + Sparse Lagrangian Vortex Particles) and realistic volumetric cloud simulation.

The player can:
Enjoy flying with realistic aerodynamic simulation across varying weather.
Customize design their plane with provided basic items (or import their own 3D models).
Enjoy the beauty of flying between beautiful clouds, daytime.
Enjoy the condition of realistic crashing physics.

Developer & Self-Evolution Guide (GEMINI.md)

This guide defines the Strict Self-Development, Test-Driven Verification, and Automated Debugging Loop for AI Agents (GEMINI). You must strictly follow these instructions to test, locate, and fix physics or rendering issues autonomously without requiring the user to act as your interactive playtester.

## Core Principle: Isolate & Locate (Separation of Concern)

When encountering a bug in wind field dynamics, wake simulation, or particle rendering, never modify both the physics solver and the render pipeline at the same time. You must isolate them to locate the root cause:
Step 1: Isolate the Physics Solver (Math & State Verification)
Method: Run a unit test or offline test harness that advances the LBM/Vortex state with a fixed time-step . Bypass the OpenGL render loop entirely.
Action: Dump the raw physical variables (e.g., density , velocity , forces , moments ) to a CSV/text file or console.
Validation Rules:
Verify Mass Conservation: The sum of micro-distribution functions must equal density , which should remain stable ().
Verify Analytical Drag: Test with a simple static obstacle (e.g., a cube) and verify if the resulting drag coefficient matches known physics ( for a cube).
Assert that no physical values contain NaN or Infinity after any solver step.
Step 2: Isolate the Visualizer (Render Pipeline Verification)
Method: Bypass the real physics solver and feed a mathematically predefined, static "Mock Wind Field" directly into your OpenGL buffers (SSBO or 3D Texture).
Action: Use simple deterministic wind inputs, such as:
A constant uniform wind: 
A perfect mathematical vortex: 
Validation Rules:
If the particles do not move, flow incorrectly, or fail to render, the bug is 100% in the Render Pipeline (e.g., SSBO memory alignment, vertex pulling index errors, shader compilation, or blending states).
If the particles flow perfectly according to the mock mathematics, the render pipeline is fully intact. The bug is 100% in the Physics Solver.

## Mandatory Unit Testing (UT) & Mocking

Write UT First: You must prioritize writing C++ unit tests (e.g., using Google Test or a lightweight test macro) for every new mathematical formula, coordinate conversion, or LBM moment calculation before integrating it into main.cpp.
No-OpenGL Dependency: Decouple your physical solvers (LBM class, Biot-Savart calculations, rigid body kinematics) from OpenGL handles. Ensure the mathematical core can compile and run as a pure console application for rapid test coverage.
Mock GLFW/GLUT: Create a mocked mock-graphics context header to allow running physics tests on headless environments without failing on window creation.

## Development Workflow & Traceability

- **Git Commitment**: You must proactively commit working changes after successfully verifying a sub-task or fix. Ensure commit messages are clear and follow project conventions.
- **Architecture Decision Record (ADR)**: Maintain an `ADR.md` file to trace important architectural changes, mathematical model selections, and logic refactors. Every significant change must be documented in the ADR with its rationale.

## Headless Self-Diagnostics & Image Verification

You must implement and use automated offline execution modes to verify your code changes before declaring a task finished. **Always prioritize writing or updating headless tests to verify your results empirically.**

A. Headless Run Mode
Maintain a command-line test runner (e.g., ./flight_sim --headless-test --steps 100).
Let the simulation advance for exactly  frames without creating a physical window (using surfaceless EGL context or GBM render nodes if available, or a fallback software context).
Log frame times, memory usage, memory bandwidth estimations, and solver convergence data to test_run.log.

B. Automated Screenshot Dumping & Self-Inspection
Implement an image exporter inside your diagnostic loop. After running the headless test for  steps, capture the screen buffer using glReadPixels and write it to an uncompressed image format (e.g., BMP, PPM, or PNG) in test_out/.
Self-Visual Verification: Inspect the dumped images yourself using your vision capability:
Verify if the wind field particles are rendered (check for black screen or empty alpha mask).
Check for visual artifacts (such as NaN-induced neon pixels, clipping planes, or broken particle trails).
Check if HUD text, flight path forces, and volume clouds are rendered within correct coordinate projections.

## Defensive Coding & Automated Statistics Dump

NaN and Infinity Assertions: Wrap every time integration step, matrix multiplication, and LBM collision calculation with strict assertions:

    assert(!std::isnan(u.x) &&!std::isinf(u.x));

Simulation Timeout Guard: To prevent getting stuck in infinite loop bugs (such as diverging conjugate gradient pressure solvers or unstable LBM iterations), implement a strict step-limit (e.g., max 1000 steps) or a thread timeout (e.g., 5 seconds max) during tests.

Automatic Crash/Fail Diagnostic Dump: If an assertion fails, immediately output a detailed diagnostic_dump.txt containing:
Current aircraft position, speed, thrust, and orientation.
Sum of forces () and torque ().

Maximum, minimum, and average fluid velocity inside the local grid.
Detailed compilation and link logs of all failed GLSL shaders.
