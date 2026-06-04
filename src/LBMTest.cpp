#include "baseHeader.h"
#include "GPUFluidSolver.h"
#include "Shader.h"
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
GLfloat planePos[4] = { 0, 0, 0, 1 }; 
M3DVector3f windVelocity = {10.0f, 0.0f, 0.0f};

M3DMatrix44f projection, model_view;

M3DVector3f cameraPos;
M3DVector3f cameraFocus = { 0, 0, 0 };
M3DVector2f pmouse = {0, 0};

// Perfect side-on view
float camDistance = 35.0f;
float camAzimuth = -1.57f; // Look down X axis (from side)
float camPolar = 0.2f;

GPUFluidSolver* solver = nullptr;
GLuint testVAO; 

bool headless = false;
int headlessSteps = 100;

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
    solver->resetParticles(); // Randomized start
}

void DrawBoundingBox() {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glBegin(GL_LINES);
    float mx = -16.0f, my = -8.0f, mz = -8.0f;
    float Mx = 16.0f, My = 8.0f, Mz = 8.0f;
    // Bottom
    glVertex3f(mx, my, mz); glVertex3f(Mx, my, mz);
    glVertex3f(Mx, my, mz); glVertex3f(Mx, My, mz);
    glVertex3f(Mx, My, mz); glVertex3f(mx, My, mz);
    glVertex3f(mx, My, mz); glVertex3f(mx, my, mz);
    // Top
    glVertex3f(mx, my, Mz); glVertex3f(Mx, my, Mz);
    glVertex3f(Mx, my, Mz); glVertex3f(Mx, My, Mz);
    glVertex3f(Mx, My, Mz); glVertex3f(mx, My, Mz);
    glVertex3f(mx, My, Mz); glVertex3f(mx, my, Mz);
    // Pillars
    glVertex3f(mx, my, mz); glVertex3f(mx, my, Mz);
    glVertex3f(Mx, my, mz); glVertex3f(Mx, my, Mz);
    glVertex3f(Mx, My, mz); glVertex3f(Mx, My, Mz);
    glVertex3f(mx, My, mz); glVertex3f(mx, My, Mz);
    glEnd();
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

    // Draw cylinder
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.4f, 0.4f, 0.4f);
    GLUquadricObj *quad = gluNewQuadric();
    glTranslatef(0, 0, -8);
    gluCylinder(quad, 1.2, 1.2, 16.0, 32, 1);
    gluDeleteQuadric(quad);
    glPopMatrix();

    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, model_view);
    
    glBindVertexArray(testVAO);
    M3DVector3f center = { 0, 0, 0 };
    solver->drawParticles(mvp, center);

    glutSwapBuffers();
}

void RunSimulationStep() {
    if (solver) {
        solver->clearSolid();
        M3DVector3f center = { 0.0f, 0.0f, 0.0f };
        solver->voxelizeCylinder(center, 1.2f, 16.0f, 2);

        M3DVector3f dummyPlaneVel = { 0.0f, 0.0f, 0.0f };
        M3DMatrix44f identity;
        m3dLoadIdentity44(identity);

        // Sub-stepping for stability
        for (int i = 0; i < 5; i++) {
            solver->step(0.001f, dummyPlaneVel, identity, center, sim_time);
            sim_time += 0.001f;
        }
    }
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

    static int warmUpFrames = 100; // Warm up LBM before showing particles

    if (running) {
        RunSimulationStep();
        if (warmUpFrames > 0) {
            warmUpFrames--;
            if (warmUpFrames == 0) solver->resetParticles();
        }
    }
    glutPostRedisplay();
}

void mousePressed(int x_, int y_) {
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;
    float dx = x - pmouse[0];
    float dy = y - pmouse[1];
    camAzimuth += dx * 2.0f;
    camPolar += dy * 1.5f;
    if (camPolar > 1.5f) camPolar = 1.5f;
    if (camPolar < 0.1f) camPolar = 0.1f;
    UpdateCamera();
    pmouse[0] = x; pmouse[1] = y;
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        pmouse[0] = 2.0 * (float)x / windowWidth - 1.0;
        pmouse[1] = 2.0 * (float)y / windowHeight - 1.0;
    }
}

void RunHeadless() {
    std::cout << "[LBMTest] Running Headless for " << headlessSteps << " steps..." << std::endl;
    for (int i = 0; i < headlessSteps; i++) {
        RunSimulationStep();
        if (i % 10 == 0) std::cout << "Step " << i << "/" << headlessSteps << std::endl;
    }

    // Dump velocity texture slice
    int nx = 128, ny = 64, nz = 64;
    std::vector<float> velData(nx * ny * nz * 4);
    glBindTexture(GL_TEXTURE_3D, solver->getVelocityTexture());
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, velData.data());

    std::ofstream dump("test_out/velocity_dump.txt");
    dump << "# x y z vx vy vz rho" << std::endl;
    // Dump middle Z slice
    int k = nz / 2;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            int idx = (k * ny * nx + j * nx + i) * 4;
            dump << i << " " << j << " " << k << " " 
                 << velData[idx] << " " << velData[idx+1] << " " << velData[idx+2] << " " << velData[idx+3] << std::endl;
        }
    }
    dump.close();
    std::cout << "[LBMTest] Dumped velocity to test_out/velocity_dump.txt" << std::endl;
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
    
    // In headless mode on some systems, we still need a window but we might be able to use a hidden one
    // or use OSMesa/EGL. For now, let's assume we can create a window or the environment has a virtual display.
    // If it fails, we'll know.
    
    glutInitContextVersion(4, 3); 
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    
    if (headless) {
        // Some GLUT implementations allow creating a window and not showing it, or running without one
        // but it's tricky. Let's try to create a window and then run our loop instead of glutMainLoop.
        glutCreateWindow("LBM Headless");
        glewInit();
        SetupRC();
        RunHeadless();
        return 0;
    }

    glutCreateWindow("LBM Perfect Alignment Test");
    glewInit();
    SetupRC();
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMotionFunc(mousePressed);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
