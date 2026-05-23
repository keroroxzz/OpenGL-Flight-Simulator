#include "baseHeader.h"
#include "GPUFluidSolver.h"
#include "Shader.h"
#include <chrono>
#include <unistd.h>
#include <math.h>

// Globals for simple test
int windowWidth = 1024;
int windowHeight = 768;
bool running = true;
float sim_time = 0.0;
GLfloat planePos[4] = { 0, 0, 0, 1 }; 
M3DVector3f windVelocity = {0.0f, 0.0f, 0.0f}; // REFACTOR: Reduced speed for stability

M3DMatrix44f projection, model_view;
M3DVector3f cameraPos;
M3DVector3f cameraFocus = { 0, 0, 0 };
M3DVector2f pmouse = {0, 0};

// Camera state in polar coordinates for stability
float camDistance = 28.0f;
float camAzimuth = 0.0f; // Rotation around Z axis
float camPolar = 1.2f;   // Angle from XY plane

GPUFluidSolver* solver = nullptr;

void UpdateCamera() {
    cameraPos[0] = cameraFocus[0] + camDistance * cos(camPolar) * sin(camAzimuth);
    cameraPos[1] = cameraFocus[1] - camDistance * cos(camPolar) * cos(camAzimuth);
    cameraPos[2] = cameraFocus[2] + camDistance * sin(camPolar);
}

void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending
    
    UpdateCamera();

    solver = new GPUFluidSolver(128, 64, 64);
    
    M3DVector3f bboxMin = { -15.0f, -8.0f, -8.0f };
    M3DVector3f bboxMax = { 15.0f, 8.0f, 8.0f };
    solver->setGridBounds(bboxMin, bboxMax);
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

    // Draw cylinder obstacle
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);
    GLUquadricObj *quad = gluNewQuadric();
    glTranslatef(0, 0, -8);
    gluCylinder(quad, 1.2, 1.2, 16.0, 32, 1);
    gluDeleteQuadric(quad);
    glEnable(GL_LIGHTING);
    glPopMatrix();

    // Visualize flow field
    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, model_view);
    solver->drawParticles(mvp);

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

    const float dt_sub = 0.002f; // Smaller sub-step for fluid stability

    if (running) {
        solver->clearSolid();
        M3DVector3f center = { 0.0f, 0.0f, 0.0f };
        solver->voxelizeCylinder(center, 1.2f, 16.0f, 2);

        M3DVector3f dummyPlaneVel = { 0.0f, 0.0f, 0.0f };
        M3DMatrix44f identity;
        m3dLoadIdentity44(identity);

        // Perform multiple sub-steps per visual frame to allow fluid to catch up
        for (int i = 0; i < 5; i++) {
            solver->step(dt_sub, dummyPlaneVel, identity, center, sim_time);
            sim_time += dt_sub;
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

    // Limit polar angle to prevent flipping and gimbal lock
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

int main(int argc, char* argv[]) {
    // REFACTOR: Ensure we are in the correct directory to load shaders
    if (chdir("flight_sim") != 0) {
        chdir("../flight_sim");
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitContextVersion(4, 6);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutCreateWindow("LBM Pure Physics Test: Karman Vortex Street");
    glewInit();
    SetupRC();
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMotionFunc(mousePressed);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
