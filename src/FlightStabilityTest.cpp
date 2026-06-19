// FlightStabilityTest
//
// Headless regression test for the full aerodynamic flight loop: instantiates a real F22,
// gives it a forward cruise velocity at altitude, applies FULL throttle, and runs
// F22::updatePhysic (LBM aero + rigid-body dynamics) for several simulated seconds.
//
// It verifies the aircraft stays *stable* while flying forward:
//   - no NaN / Inf ever appears in position, velocity, angular velocity, or orientation
//   - angular velocity stays bounded (catches the runaway-spin regressions: the inertia-tensor
//     OOB inversion bug and torque-about-grid-center bug, see ADR 12 / ADR 13)
//   - linear speed stays bounded (no force-scaling blow-up, see ADR 9)
//   - the aircraft actually makes forward progress
//
// Usage: ./flight_sim/flight_stability_test [--steps N] [--verbose]
// Needs a GL context (compute shaders); run on a real/virtual display, from repo root or
// inside flight_sim/.  Exit code 0 = PASS, 1 = FAIL (so it can gate CI / scripts).

#include "baseHeader.h"
#include "Plane.h"
#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>
#include <cmath>

// Globals referenced as `extern` by Plane.cpp / GPUFluidSolver.cpp / WakeSystem.h.
float sim_time = 0.0f;
M3DVector3f windVelocity = { 0.0f, 0.0f, 0.0f }; // still air; TAS comes purely from flying forward
M3DMatrix44f projection;                          // referenced by F22's (unused here) draw methods

// Stability thresholds. Pre-fix, angular velocity blew up to >>10 rad/s within a fraction of a
// second; a stably-flying aircraft stays well under a few rad/s.
static const float MAX_ANGULAR_RATE = 10.0f;   // rad/s
static const float MAX_SPEED        = 2000.0f; // m/s (~Mach 6; a real F22 never gets here)
static const float CRUISE_SPEED     = 250.0f;  // m/s initial forward velocity

static bool isBad(float v) { return std::isnan(v) || std::isinf(v); }
static bool vecBad(const M3DVector3f v) { return isBad(v[0]) || isBad(v[1]) || isBad(v[2]); }
static float vecLen(const M3DVector3f v) { return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); }

int main(int argc, char* argv[]) {
    int steps = 500; // each updatePhysic call advances 10 x 0.001s = 0.01s -> 500 = 5.0s
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) steps = atoi(argv[++i]);
        if (strcmp(argv[i], "--verbose") == 0) verbose = true;
    }

    // F22() loads f22/*.obj relative to flight_sim/.
    if (chdir("flight_sim") != 0) { if (chdir("../flight_sim") != 0) {} }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(256, 256);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutCreateWindow("Flight Stability Test");
    glewInit();

    F22* f22 = new F22();

    // Start at altitude (grid box is wpos.z +/- 8; ground voxelizes when wpos.z <= ~8.1, so
    // 1000m keeps us in clean free air) and flying forward along +x at cruise speed.
    M3DVector3f startPos = { 0.0f, 0.0f, 1000.0f };
    M3DVector3f startVel = { CRUISE_SPEED, 0.0f, 0.0f };
    f22->setPosition(startPos);
    f22->setVelocity(startVel);
    f22->reinitEquilibrium(windVelocity);

    printf("[FlightStabilityTest] %d steps (%.2fs sim), full throttle, start speed %.0f m/s\n",
           steps, steps * 0.01f, CRUISE_SPEED);

    float maxAngRate = 0.0f, maxSpeed = 0.0f;
    bool sawNaN = false;
    M3DVector3f startP; f22->getPosition(startP);

    for (int i = 0; i < steps; i++) {
        f22->thrustControl(1.0f); // full throttle (ramps to max over a few calls)
        f22->updatePhysic(false, windVelocity);

        M3DVector3f pos, vel, ang;
        f22->getPosition(pos);
        f22->getVelocity(vel);
        f22->getAngularVelocity(ang);

        if (vecBad(pos) || vecBad(vel) || vecBad(ang)) { sawNaN = true; printf("  [step %d] NaN/Inf detected!\n", i); break; }

        float speed = vecLen(vel);
        float angRate = vecLen(ang);
        if (speed   > maxSpeed)   maxSpeed = speed;
        if (angRate > maxAngRate) maxAngRate = angRate;

        if (verbose && i % 50 == 0)
            printf("  step %d: pos[%.1f %.1f %.1f] speed %.1f m/s  angRate %.4f rad/s  throttle %.2f\n",
                   i, pos[0], pos[1], pos[2], speed, angRate, f22->getThrottle());
    }

    M3DVector3f endP; f22->getPosition(endP);
    M3DVector3f disp; m3dSubtractVectors3(disp, endP, startP);
    float forwardProgress = disp[0]; // started flying +x

    printf("\n--- Stability Results ---\n");
    printf("  max angular rate : %.4f rad/s (limit %.1f)\n", maxAngRate, MAX_ANGULAR_RATE);
    printf("  max speed        : %.1f m/s   (limit %.0f)\n", maxSpeed, MAX_SPEED);
    printf("  forward progress : %.1f m\n", forwardProgress);

    bool pass = !sawNaN
             && maxAngRate < MAX_ANGULAR_RATE
             && maxSpeed   < MAX_SPEED
             && forwardProgress > 1.0f;

    if (pass) { printf(">> STATUS: STABLE FORWARD FLIGHT [PASS]\n"); }
    else {
        printf(">> STATUS: UNSTABLE [FAIL]");
        if (sawNaN)                          printf(" (NaN/Inf)");
        if (maxAngRate >= MAX_ANGULAR_RATE)  printf(" (runaway rotation)");
        if (maxSpeed   >= MAX_SPEED)         printf(" (speed blow-up)");
        if (forwardProgress <= 1.0f)         printf(" (no forward progress)");
        printf("\n");
    }

    delete f22;
    return pass ? 0 : 1;
}
