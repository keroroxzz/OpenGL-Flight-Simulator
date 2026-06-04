#include "baseHeader.h"
#include "GPUFluidSolver.h"
#include "Shader.h"
#include "ObjModel.h"
#include <chrono>
#include <unistd.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>

// Globals
int windowWidth = 1024;
int windowHeight = 768;
bool running = true;
float sim_time = 0.0;
M3DVector3f windVelocity = {10.0f, 0.0f, 0.0f}; // 30 m/s (~60 kts)

M3DMatrix44f projection, model_view;

M3DVector3f cameraPos;
M3DVector3f cameraFocus = { 0, 0, 0 };
M3DVector2f pmouse = {0, 0};

float camDistance = 35.0f;
float camAzimuth = -1.57f; 
float camPolar = 0.2f;

GPUFluidSolver* solver = nullptr;
ObjModel* f22_body = nullptr;
GLuint testVAO; 

bool headless = false;
int headlessSteps = 5000;

void UpdateCamera() {
    cameraPos[0] = cameraFocus[0] + camDistance * cos(camPolar) * sin(camAzimuth);
    cameraPos[1] = cameraFocus[1] - camDistance * cos(camPolar) * cos(camAzimuth);
    cameraPos[2] = cameraFocus[2] + camDistance * sin(camPolar);
}

void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    
    glGenVertexArrays(1, &testVAO);
    glBindVertexArray(testVAO);

    UpdateCamera();

    solver = new GPUFluidSolver(128, 64, 64);
    
    f22_body = new ObjModel("f22/f22_body.obj", 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    
    M3DVector3f minB, maxB, center;
    f22_body->getBounds(minB, maxB);
    std::cout << "[AircraftTest] Model Bounds: Min[" << minB[0] << ", " << minB[1] << ", " << minB[2] << "] "
              << "Max[" << maxB[0] << ", " << maxB[1] << ", " << maxB[2] << "]" << std::endl;

    m3dAddVectors3(center, minB, maxB);
    m3dScaleVector3(center, 0.5f);

    M3DMatrix44f transform, rot, trans;
    m3dTranslationMatrix44(trans, -center[0], -center[1], -center[2]);
    m3dRotationMatrix44(rot, m3dDegToRad(2.0f), 0, 1, 0); // 2 deg pitch up
    m3dMatrixMultiply44(transform, rot, trans);
    
    solver->clearSolid();
    solver->voxelizePart(f22_body, transform);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    solver->reinitEquilibrium(windVelocity);
    solver->resetParticles();
}

void DrawBoundingBox() {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    float mx = -16.0f, my = -8.0f, mz = -8.0f;
    float Mx = 16.0f, My = 8.0f, Mz = 8.0f;
    glVertex3f(mx, my, mz); glVertex3f(Mx, my, mz);
    glVertex3f(Mx, my, mz); glVertex3f(Mx, My, mz);
    glVertex3f(Mx, My, mz); glVertex3f(mx, My, mz);
    glVertex3f(mx, My, mz); glVertex3f(mx, my, mz);
    glVertex3f(mx, my, Mz); glVertex3f(Mx, my, Mz);
    glVertex3f(Mx, my, Mz); glVertex3f(Mx, My, Mz);
    glVertex3f(Mx, My, Mz); glVertex3f(mx, My, Mz);
    glVertex3f(mx, My, Mz); glVertex3f(mx, my, Mz);
    glVertex3f(mx, my, mz); glVertex3f(mx, my, Mz);
    glVertex3f(Mx, my, mz); glVertex3f(Mx, my, Mz);
    glVertex3f(Mx, My, mz); glVertex3f(Mx, My, Mz);
    glVertex3f(mx, My, mz); glVertex3f(mx, My, Mz);
    glEnd();
}

void RunSimulationStep() {
    if (solver) {
        M3DVector3f planeVel = { 0.0f, 0.0f, 0.0f };
        M3DVector3f planePos = { 0.0f, 0.0f, 0.0f };
        M3DMatrix44f identity;
        m3dLoadIdentity44(identity);

        // Run more steps per frame for faster wake development
        for (int i = 0; i < 20; i++) {
            solver->step(0.001f, planeVel, identity, planePos, sim_time);
            sim_time += 0.001f;
        }
    }
}

void RenderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)windowWidth/windowHeight, 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2], cameraFocus[0], cameraFocus[1], cameraFocus[2], 0, 0, 1);
    glGetFloatv(GL_MODELVIEW_MATRIX, model_view);

    DrawBoundingBox();

    if (f22_body) {
        glPushMatrix();
        M3DVector3f minB, maxB, center;
        f22_body->getBounds(minB, maxB);
        m3dAddVectors3(center, minB, maxB);
        m3dScaleVector3(center, 0.5f);
        glTranslatef(-center[0], -center[1], -center[2]);
        glRotatef(2.0f, 0, 1, 0); // Visual match for AOE
        glColor3f(0.5f, 0.5f, 0.5f);
        f22_body->draw();
        glPopMatrix();
    }

    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, model_view);
    
    glBindVertexArray(testVAO);
    M3DVector3f center = { 0, 0, 0 };
    solver->drawParticles(mvp, center);

    glutSwapBuffers();
}

void idle() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt_frame = std::chrono::duration<float>(currentTime - lastTime).count();
    
    if (dt_frame < 0.01f) {
        usleep(1000);
        return;
    }
    lastTime = currentTime;

    if (running) {
        RunSimulationStep();
    }
    glutPostRedisplay();
}

void RunHeadless() {
    std::cout << "[AircraftTest] Running Headless for " << headlessSteps << " steps..." << std::endl;
    for (int i = 0; i < headlessSteps; i++) {
        M3DVector3f planeVel = { 0.0f, 0.0f, 0.0f };
        M3DVector3f planePos = { 0.0f, 0.0f, 0.0f };
        M3DMatrix44f identity;
        m3dLoadIdentity44(identity);
        solver->step(0.001f, planeVel, identity, planePos, sim_time);
        sim_time += 0.001f;

        if (i % 100 == 0) {
            M3DVector3f* force = solver->getCFDForce();
            std::cout << "Step " << i << "/" << headlessSteps 
                      << " | Force: [" << (*force)[0] << ", " << (*force)[1] << ", " << (*force)[2] << "]" << std::endl;
        }
    }

    int nx = 128, ny = 64, nz = 64;
    std::vector<float> velData(nx * ny * nz * 4);
    glBindTexture(GL_TEXTURE_3D, solver->getVelocityTexture());
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, velData.data());

    std::ofstream dump("test_out/aircraft_velocity.txt");
    dump << "# x y z vx vy vz rho" << std::endl;
    int k = nz / 2;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            int idx = (k * ny * nx + j * nx + i) * 4;
            dump << i << " " << j << " " << k << " " 
                 << velData[idx] << " " << velData[idx+1] << " " << velData[idx+2] << " " << velData[idx+3] << std::endl;
        }
    }
    dump.close();
    std::cout << "[AircraftTest] Dumped velocity to test_out/aircraft_velocity.txt" << std::endl;
}

void mousePressed(int x_, int y_) {
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;
    float dx = x - pmouse[0];
    float dy = y - pmouse[1];
    
    camAzimuth += dx * 2.0f;
    camPolar += dy * 1.5f;
    if (camPolar > 1.5f) camPolar = 1.5f;
    if (camPolar < -1.5f) camPolar = -1.5f;
    
    UpdateCamera();
    pmouse[0] = x; pmouse[1] = y;
}

void mouse(int button, int state, int x, int y) {
    pmouse[0] = 2.0 * (float)x / windowWidth - 1.0;
    pmouse[1] = 2.0 * (float)y / windowHeight - 1.0;
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); 
    if (key == ' ') running = !running;
    if (key == 'r') {
        solver->resetParticles();
    }
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) headless = true;
        if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) headlessSteps = atoi(argv[++i]);
    }

    if (chdir("flight_sim") != 0) chdir("../flight_sim");
    system("mkdir -p test_out");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitContextVersion(4, 3); 
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    
    if (headless) {
        glutCreateWindow("Aircraft Headless");
        glewInit();
        SetupRC();
        RunHeadless();
        return 0;
    }

    glutCreateWindow("F-22 Aerodynamics Test");
    glewInit();
    SetupRC();
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutMotionFunc(mousePressed);
    glutKeyboardFunc(keyboard);
    
    std::cout << "\n--- Aircraft Test Controls ---" << std::endl;
    std::cout << "Left Mouse Drag  : Orbit Camera" << std::endl;
    std::cout << "Space            : Toggle Simulation" << std::endl;
    std::cout << "R                : Reset Particles" << std::endl;
    std::cout << "ESC              : Exit" << std::endl;
    std::cout << "------------------------------\n" << std::endl;

    glutMainLoop();
    return 0;
}
