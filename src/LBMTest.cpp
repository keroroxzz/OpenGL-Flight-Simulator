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

enum TestMode {
    KARMAN_VORTEX,
    LID_DRIVEN_CAVITY,
    STEP_FLOW
};

struct TestConfig {
    TestMode mode;
    std::string name;
    int nx, ny, nz;
    M3DVector3f gridMin, gridMax;
    M3DVector3f wind;
    float lidVelocity;
};

// Globals
int windowWidth = 1024;
int windowHeight = 768;
bool running = true;
float sim_time = 0.0;
M3DVector3f windVelocity = {10.0f, 0.0f, 0.0f};

TestConfig currentTest = {
    KARMAN_VORTEX, "Karman Vortex Street",
    128, 64, 64,
    {-16.0f, -8.0f, -8.0f}, {16.0f, 8.0f, 8.0f},
    {10.0f, 0.0f, 0.0f}, 0.0f
};

// Cylinder sits upstream of grid center so the wake has room (~8.7D vs ~5.3D at center)
// to develop several full shedding wavelengths before the outflow sponge layer absorbs it.
const float KARMAN_CYLINDER_X = -8.0f;

M3DMatrix44f projection, model_view;
M3DVector3f cameraPos;
M3DVector3f cameraFocus = { 0, 0, 0 };
M3DVector2f pmouse = {0, 0};
float camDistance = 35.0f;
float camAzimuth = -1.57f;
float camPolar = 0.2f;

GPUFluidSolver* solver = nullptr;
GLuint testVAO; 
bool headless = false;
int headlessSteps = 500;

void UpdateCamera() {
    cameraPos[0] = cameraFocus[0] + camDistance * cos(camPolar) * sin(camAzimuth);
    cameraPos[1] = cameraFocus[1] - camDistance * cos(camPolar) * cos(camAzimuth);
    cameraPos[2] = cameraFocus[2] + camDistance * sin(camPolar);
}

void SetupSolver() {
    if (solver) delete solver;
    solver = new GPUFluidSolver(currentTest.nx, currentTest.ny, currentTest.nz);
    solver->setGridBounds(currentTest.gridMin, currentTest.gridMax);
    solver->resetParticles();
    sim_time = 0.0;
}

void RunSimulationStep() {
    if (!solver) return;
    
    solver->clearSolid();
    
    if (currentTest.mode == KARMAN_VORTEX) {
        M3DVector3f center = { KARMAN_CYLINDER_X, 0.0f, 0.0f };
        solver->voxelizeCylinder(center, 1.2f, 16.0f, 2);
    } else if (currentTest.mode == STEP_FLOW) {
        M3DVector3f stepOrigin = { currentTest.gridMin[0], currentTest.gridMin[1], currentTest.gridMin[2] };
        solver->voxelizeGround(currentTest.gridMin[2] + 4.0f, stepOrigin); 
    }

    M3DMatrix44f identity;
    m3dLoadIdentity44(identity);
    M3DVector3f origin = { 0.0f, 0.0f, 0.0f };

    // Break symmetry by jittering the apparent inflow wind (step() computes the inflow as
    // windVelocity - planeVel, so feeding -perturbation here adds it to the relative wind).
    // Previously this jitter was passed into step()'s unused planePos argument instead, so it
    // never actually reached the flow - shedding relied on numerical noise alone.
    // Decay the nudge away after ~1s so it only seeds the initial instability instead of
    // continuously forcing the wake at the nudge's own frequency (which otherwise locks the
    // shedding to that forcing frequency instead of the cylinder's natural one).
    M3DVector3f perturbedVel = { 0.0f, 0.0f, 0.0f };
    if (currentTest.mode == KARMAN_VORTEX) {
        float amp = 0.5f * exp(-sim_time / 0.4f);
        perturbedVel[1] = -amp * sin(sim_time * 10.0f);
        perturbedVel[2] = -amp * 0.6f * cos(sim_time * 7.0f);
    }

    if (currentTest.mode == LID_DRIVEN_CAVITY) {
        M3DVector3f lidVel = { 20.0f, 0.0f, 0.0f };
        solver->debugUniformVelocity(lidVel);
    }

    for (int i = 0; i < 5; i++) {
        solver->step(0.001f, perturbedVel, identity, origin, sim_time);
        sim_time += 0.001f;
    }
}

void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glGenVertexArrays(1, &testVAO);
    glBindVertexArray(testVAO);
    UpdateCamera();
    SetupSolver();
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

    if (currentTest.mode == KARMAN_VORTEX) {
        glPushMatrix();
        glColor3f(0.4f, 0.4f, 0.4f);
        GLUquadricObj *quad = gluNewQuadric();
        glTranslatef(KARMAN_CYLINDER_X, 0, -8);
        gluCylinder(quad, 1.2, 1.2, 16.0, 32, 1);
        gluDeleteQuadric(quad);
        glPopMatrix();
    }

    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, model_view);
    glBindVertexArray(testVAO);
    solver->drawParticles(mvp);
    glutSwapBuffers();
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

void KeyPressFunc(unsigned char key, int x, int y) {
    float moveSpeed = 1.0f;
    float sinA = sin(camAzimuth);
    float cosA = cos(camAzimuth);

    switch (key) {
        case 'w':
            cameraFocus[0] += sinA * moveSpeed;
            cameraFocus[1] -= cosA * moveSpeed;
            break;
        case 's':
            cameraFocus[0] -= sinA * moveSpeed;
            cameraFocus[1] += cosA * moveSpeed;
            break;
        case 'a':
            cameraFocus[0] -= cosA * moveSpeed;
            cameraFocus[1] -= sinA * moveSpeed;
            break;
        case 'd':
            cameraFocus[0] += cosA * moveSpeed;
            cameraFocus[1] += sinA * moveSpeed;
            break;
        case 'r': cameraFocus[2] += moveSpeed; break;
        case 'f': cameraFocus[2] -= moveSpeed; break;
        case 'q': exit(0); break;
    }
    UpdateCamera();
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        pmouse[0] = 2.0 * (float)x / windowWidth - 1.0;
        pmouse[1] = 2.0 * (float)y / windowHeight - 1.0;
    }
    if (button == 3) camDistance -= 2.0f;
    if (button == 4) camDistance += 2.0f;
    if (camDistance < 2.0f) camDistance = 2.0f;
    UpdateCamera();
}

void RunHeadless() {
    std::cout << "[LBMTest] Running Scenario: " << currentTest.name << std::endl;
    std::vector<float> liftHistory;
    
    for (int i = 0; i < headlessSteps; i++) {
        RunSimulationStep();
        
        if (currentTest.mode == KARMAN_VORTEX) {
            M3DVector3f* force = solver->getCFDForce();
            liftHistory.push_back((*force)[1]); 
        }
        
        if (i % 50 == 0) std::cout << "Step " << i << "/" << headlessSteps << std::endl;
    }

    int nx = currentTest.nx, ny = currentTest.ny, nz = currentTest.nz;
    std::vector<float> velData(nx * ny * nz * 4);
    glBindTexture(GL_TEXTURE_3D, solver->getVelocityTexture());
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, velData.data());

    std::cout << "\n--- Validation Results ---" << std::endl;
    if (currentTest.mode == KARMAN_VORTEX) {
        int crossings = 0;
        float avgLift = 0; for(float l : liftHistory) avgLift += l; 
        if (!liftHistory.empty()) avgLift /= liftHistory.size();
        for(size_t i=1; i<liftHistory.size(); i++) {
            if ((liftHistory[i-1] - avgLift) * (liftHistory[i] - avgLift) < 0) crossings++;
        }
        float totalTime = headlessSteps * 0.005f;
        float freq = (crossings / 2.0f) / (totalTime > 0 ? totalTime : 1.0f); 
        float strouhal = freq * 2.4f / 10.0f; 
        std::cout << "Karman Index (Strouhal Number): " << strouhal << " (Expected ~0.15-0.21)" << std::endl;
        if (strouhal > 0.01f) std::cout << ">> STATUS: VORTEX SHEDDING DETECTED [PASS]" << std::endl;
        else std::cout << ">> STATUS: STEADY FLOW [FAIL]" << std::endl;
    } else if (currentTest.mode == LID_DRIVEN_CAVITY) {
        float minV = 1e10; int cx=0, cy=0;
        int k = nz / 2;
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                int idx = (k * ny * nx + j * nx + i) * 4;
                float speed = sqrt(velData[idx]*velData[idx] + velData[idx+1]*velData[idx+1]);
                if (speed < minV && i > 5 && i < nx-5 && j > 5 && j < ny-5) {
                    minV = speed; cx = i; cy = j;
                }
            }
        }
        std::cout << "Cavity Index (Vortex Center): [" << (float)cx/nx << ", " << (float)cy/ny << "] (Expected ~[0.5, 0.7] for Re=1000)" << std::endl;
    } else if (currentTest.mode == STEP_FLOW) {
        int k = nz / 2; int j = ny / 2;
        int reattachmentX = nx/4;
        for (int i = nx/4 + 2; i < nx; i++) {
            int idx = (k * ny * nx + j * nx + i) * 4;
            if (velData[idx] > 0.1f) { reattachmentX = i; break; }
        }
        float length = (float)(reattachmentX - nx/4) * (32.0f / nx);
        std::cout << "Step Index (Reattachment Length): " << length << "m (Expected > 0)" << std::endl;
    }

    std::string filename = "test_out/" + currentTest.name + "_dump.txt";
    std::ofstream dump(filename);
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
    std::cout << "[LBMTest] Result dump saved to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) headless = true;
        if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) headlessSteps = atoi(argv[++i]);
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            std::string m = argv[++i];
            if (m == "karman") currentTest.mode = KARMAN_VORTEX;
            else if (m == "cavity") {
                currentTest.mode = LID_DRIVEN_CAVITY;
                currentTest.name = "Lid Driven Cavity";
                currentTest.nx = currentTest.ny = currentTest.nz = 64;
                currentTest.gridMin[0] = currentTest.gridMin[1] = currentTest.gridMin[2] = -8.0f;
                currentTest.gridMax[0] = currentTest.gridMax[1] = currentTest.gridMax[2] = 8.0f;
            }
            else if (m == "step") {
                currentTest.mode = STEP_FLOW;
                currentTest.name = "Backward Facing Step";
            }
        }
    }

    if (chdir("flight_sim") != 0) chdir("../flight_sim");
    system("mkdir -p test_out");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

    if (headless) {
        glutCreateWindow("LBM Headless");
        glewInit();
        SetupSolver();
        RunHeadless();
        return 0;
    }

    glutCreateWindow("LBM Validation Suite");
    glewInit();
    SetupRC();
    glutDisplayFunc(RenderScene);
    glutIdleFunc([](){
        if (running) RunSimulationStep();
        glutPostRedisplay();
    });
    glutKeyboardFunc(KeyPressFunc);
    glutMotionFunc(mousePressed);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
