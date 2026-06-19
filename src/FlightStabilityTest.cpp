// FlightStabilityTest
//
// Headless regression test for the full aerodynamic flight loop: instantiates a real F22,
// gives it a forward cruise velocity at altitude, applies FULL throttle, optionally holds a
// control-surface deflection (elevator / aileron / rudder), and runs F22::updatePhysic
// (LBM aero + rigid-body dynamics) for several simulated seconds.
//
// It verifies the aircraft stays *stable* through the maneuver:
//   - no NaN / Inf ever appears in position, velocity, angular velocity, or orientation
//   - angular velocity stays bounded (catches runaway-spin regressions: the inertia-tensor
//     OOB inversion bug and torque-about-grid-center bug, see ADR 12 / ADR 13)
//   - linear speed stays bounded (no force-scaling blow-up, see ADR 9)
//   - the aircraft makes forward progress
//   - for a commanded maneuver, the flight path actually responds (control authority present)
//
// It also records the flight "trail" (trajectory + attitude per step) to
// test_out/flight_trail_<maneuver>.txt and prints a summary (climb, heading change, lateral
// drift) so the resulting path can be inspected.
//
// Usage:
//   ./flight_sim/flight_stability_test [--steps N] [--verbose]
//                                      [--maneuver <name>] [--deflect <0..1>]
//                                      [--pitch v] [--roll v] [--yaw v]   (explicit, [-1,1])
//   maneuvers: level (default) | pitch-up | pitch-down | roll-left | roll-right
//              | yaw-left | yaw-right | turn-left | turn-right
// Needs a GL context; run on a real/virtual display. Exit 0 = PASS, 1 = FAIL.

#include "baseHeader.h"
#include "Plane.h"
#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <fstream>
#include <vector>

// Globals referenced as `extern` by Plane.cpp / GPUFluidSolver.cpp / WakeSystem.h.
float sim_time = 0.0f;
M3DVector3f windVelocity = { 0.0f, 0.0f, 0.0f }; // still air; TAS comes purely from flying forward
M3DMatrix44f projection;                          // referenced by F22's (unused here) draw methods

static const float MAX_ANGULAR_RATE = 10.0f;   // rad/s; stable flight stays well under a few
static const float MAX_SPEED        = 2000.0f; // m/s
static const float CRUISE_SPEED     = 250.0f;  // m/s initial forward velocity
static const float RESPONSE_EPS     = 2.0f;    // min trajectory response for a commanded maneuver

static bool  isBad(float v)              { return std::isnan(v) || std::isinf(v); }
static bool  vecBad(const M3DVector3f v) { return isBad(v[0]) || isBad(v[1]) || isBad(v[2]); }
static float vecLen(const M3DVector3f v) { return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); }

int main(int argc, char* argv[]) {
    int steps = 500; // each updatePhysic call advances 10 x 0.001s = 0.01s -> 500 = 5.0s
    bool verbose = false;
    std::string maneuver = "level";
    float deflect = 0.6f;                 // normalized magnitude for maneuver presets
    float pitch = 0, roll = 0, yaw = 0;   // explicit overrides
    bool explicitCtl = false;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--steps") == 0    && i+1 < argc) steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--verbose") == 0)                verbose = true;
        else if (strcmp(argv[i], "--maneuver") == 0 && i+1 < argc) maneuver = argv[++i];
        else if (strcmp(argv[i], "--deflect") == 0  && i+1 < argc) deflect = atof(argv[++i]);
        else if (strcmp(argv[i], "--pitch") == 0    && i+1 < argc) { pitch = atof(argv[++i]); explicitCtl = true; }
        else if (strcmp(argv[i], "--roll") == 0     && i+1 < argc) { roll  = atof(argv[++i]); explicitCtl = true; }
        else if (strcmp(argv[i], "--yaw") == 0      && i+1 < argc) { yaw   = atof(argv[++i]); explicitCtl = true; }
    }

    // Map a named maneuver to control inputs (unless explicit --pitch/--roll/--yaw were given).
    // turn-* use coordinated roll + yaw. Sign convention is reported via the trail, not assumed.
    if (!explicitCtl) {
        if      (maneuver == "pitch-up")   pitch =  deflect;
        else if (maneuver == "pitch-down") pitch = -deflect;
        else if (maneuver == "roll-right") roll  =  deflect;
        else if (maneuver == "roll-left")  roll  = -deflect;
        else if (maneuver == "yaw-right")  yaw   =  deflect;
        else if (maneuver == "yaw-left")   yaw   = -deflect;
        else if (maneuver == "turn-right") { roll =  deflect; yaw =  deflect; }
        else if (maneuver == "turn-left")  { roll = -deflect; yaw = -deflect; }
        else maneuver = "level"; // anything else (incl. "level") -> no input
    } else {
        maneuver = "custom";
    }
    bool commanded = (fabsf(pitch) + fabsf(roll) + fabsf(yaw)) > 1e-4f;

    if (chdir("flight_sim") != 0) { if (chdir("../flight_sim") != 0) {} }
    system("mkdir -p test_out");

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

    printf("[FlightStabilityTest] maneuver=%s  pitch=%.2f roll=%.2f yaw=%.2f  %d steps (%.2fs), full throttle\n",
           maneuver.c_str(), pitch, roll, yaw, steps, steps * 0.01f);

    // Trail dump: time, position, forward axis (X), up axis (Z).
    std::string trailName = "test_out/flight_trail_" + maneuver + ".txt";
    std::ofstream trail(trailName);
    trail << "# t  px py pz  fx fy fz  ux uy uz  speed angRate\n";

    float maxAngRate = 0.0f, maxSpeed = 0.0f;
    bool sawNaN = false;
    M3DVector3f startP, startFwd; f22->getPosition(startP); f22->getX(startFwd);
    float startHeading = atan2f(startFwd[1], startFwd[0]);

    for (int i = 0; i < steps; i++) {
        f22->thrustControl(1.0f);            // full throttle (ramps to max over a few calls)
        f22->setControl(pitch, roll, yaw);   // hold the commanded control deflection
        f22->updatePhysic(false, windVelocity);

        M3DVector3f pos, vel, ang, fwd, up;
        f22->getPosition(pos); f22->getVelocity(vel); f22->getAngularVelocity(ang);
        f22->getX(fwd); f22->getZ(up);

        if (vecBad(pos) || vecBad(vel) || vecBad(ang) || vecBad(fwd) || vecBad(up)) {
            sawNaN = true; printf("  [step %d] NaN/Inf detected!\n", i); break;
        }

        float speed = vecLen(vel), angRate = vecLen(ang);
        if (speed > maxSpeed)     maxSpeed = speed;
        if (angRate > maxAngRate) maxAngRate = angRate;

        trail << sim_time << "  " << pos[0] << " " << pos[1] << " " << pos[2]
              << "  " << fwd[0] << " " << fwd[1] << " " << fwd[2]
              << "  " << up[0] << " " << up[1] << " " << up[2]
              << "  " << speed << " " << angRate << "\n";

        if (verbose && i % 50 == 0)
            printf("  step %d: pos[%.1f %.1f %.1f] speed %.1f  angRate %.4f  fwd[%.2f %.2f %.2f]\n",
                   i, pos[0], pos[1], pos[2], speed, angRate, fwd[0], fwd[1], fwd[2]);
    }
    trail.close();

    M3DVector3f endP, endFwd; f22->getPosition(endP); f22->getX(endFwd);
    M3DVector3f disp; m3dSubtractVectors3(disp, endP, startP);
    float endHeading = atan2f(endFwd[1], endFwd[0]);
    float headingChangeDeg = (endHeading - startHeading) * 180.0f / 3.14159265f;
    while (headingChangeDeg > 180.0f)  headingChangeDeg -= 360.0f;
    while (headingChangeDeg < -180.0f) headingChangeDeg += 360.0f;

    float forwardProgress = disp[0];          // started flying +x
    float lateralDrift    = disp[1];          // +y left / -y right (turning)
    float altitudeChange  = disp[2];          // +z climb / -z descend (pulling up/down)
    float pitchAttitude   = endFwd[2];        // nose-up component of forward vector

    printf("\n--- Stability + Trail (%s) ---\n", maneuver.c_str());
    printf("  max angular rate : %.4f rad/s (limit %.1f)\n", maxAngRate, MAX_ANGULAR_RATE);
    printf("  max speed        : %.1f m/s   (limit %.0f)\n", maxSpeed, MAX_SPEED);
    printf("  forward progress : %.1f m\n", forwardProgress);
    printf("  altitude change  : %+.1f m   (climb+/descend-)\n", altitudeChange);
    printf("  lateral drift    : %+.1f m   (left+/right-)\n", lateralDrift);
    printf("  heading change   : %+.1f deg\n", headingChangeDeg);
    printf("  final pitch attd : %+.3f  (forward.z, nose-up+)\n", pitchAttitude);
    printf("  trail dumped to  : flight_sim/%s\n", trailName.c_str());

    // Stability must always hold.
    bool stable = !sawNaN
               && maxAngRate < MAX_ANGULAR_RATE
               && maxSpeed   < MAX_SPEED
               && forwardProgress > 1.0f;

    // For a commanded maneuver, the flight path must measurably respond in the relevant channel.
    bool responded = true;
    if (stable && commanded) {
        bool pitchCmd = fabsf(pitch) > 1e-4f;
        bool turnCmd  = (fabsf(roll) + fabsf(yaw)) > 1e-4f;
        float pitchResp = fabsf(altitudeChange) + fabsf(pitchAttitude) * forwardProgress;
        float turnResp  = fabsf(lateralDrift) + fabsf(headingChangeDeg);
        if (pitchCmd && pitchResp < RESPONSE_EPS) responded = false;
        if (turnCmd  && turnResp  < RESPONSE_EPS) responded = false;
    }

    bool pass = stable && responded;
    if (pass) {
        printf(">> STATUS: %s [PASS]\n", commanded ? "STABLE MANEUVER" : "STABLE FORWARD FLIGHT");
    } else {
        printf(">> STATUS: [FAIL]");
        if (sawNaN)                         printf(" (NaN/Inf)");
        if (maxAngRate >= MAX_ANGULAR_RATE) printf(" (runaway rotation)");
        if (maxSpeed   >= MAX_SPEED)        printf(" (speed blow-up)");
        if (forwardProgress <= 1.0f)        printf(" (no forward progress)");
        if (!responded)                     printf(" (no response to control)");
        printf("\n");
    }

    delete f22;
    return pass ? 0 : 1;
}
