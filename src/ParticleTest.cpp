#include "baseHeader.h"
#include "Shader.h"
#include <chrono>
#include <unistd.h>
#include <math.h>
#include <vector>

// Globals
int windowWidth = 1024;
int windowHeight = 768;
float sim_time = 0.0;

M3DMatrix44f projection, model_view;
M3DVector3f cameraPos = { 0.0f, -25.0f, 15.0f };
M3DVector3f cameraFocus = { 0, 0, 0 };

GLuint particleSSBO;
GLuint emptyVAO;
GLuint renderProg;
const int numParticles = 50000;
Shader* moveShader = nullptr;

struct Particle {
    float pos_life[4];
    float vel_vort[4];
};

void ChangeSize(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
}

void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glGenVertexArrays(1, &emptyVAO);
    glBindVertexArray(emptyVAO);

    // Init Particles with random positions to see a cloud immediately
    std::vector<Particle> initial(numParticles);
    for(int i=0; i<numParticles; i++) {
        initial[i].pos_life[0] = (float(rand())/RAND_MAX - 0.5f) * 10.0f;
        initial[i].pos_life[1] = (float(rand())/RAND_MAX - 0.5f) * 10.0f;
        initial[i].pos_life[2] = (float(rand())/RAND_MAX - 0.5f) * 10.0f;
        initial[i].pos_life[3] = 1.0f;
    }
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * numParticles, initial.data(), GL_DYNAMIC_DRAW);

    // Use a moving embedded vertex shader to verify time updates
    const char* vsSource = 
        "#version 430 core\n"
        "struct Particle { vec4 pos_life; vec4 vel_vort; };\n"
        "layout(std430, binding = 0) buffer p_buf { Particle particles[]; };\n"
        "uniform mat4 mvp;\n"
        "uniform float iTime;\n"
        "void main() {\n"
        "  vec3 pos = particles[gl_VertexID].pos_life.xyz;\n"
        "  // Add a small debug offset based on time to verify updates\n"
        "  pos.y += sin(iTime + pos.x * 0.5) * 0.5;\n"
        "  gl_Position = mvp * vec4(pos, 1.0);\n"
        "}\n";
    const char* fsSource = 
        "#version 430 core\n"
        "out vec4 color;\n"
        "void main() { color = vec4(0.0, 1.0, 1.0, 1.0); }\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);
    renderProg = glCreateProgram();
    glAttachShader(renderProg, vs);
    glAttachShader(renderProg, fs);
    glLinkProgram(renderProg);
    
    moveShader = new Shader(1);
    moveShader->addFromFile("shaders/compute/debug_circular.comp", GL_COMPUTE_SHADER);
}

void RenderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Debug Triangle
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glBegin(GL_TRIANGLES); glColor3f(0.5, 0, 0); glVertex3f(-0.8, -0.8, 0); glVertex3f(-0.6, -0.8, 0); glVertex3f(-0.7, -0.6, 0); glEnd();

    // 2. Particle Rendering
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)windowWidth/windowHeight, 0.1, 1000.0);
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2], cameraFocus[0], cameraFocus[1], cameraFocus[2], 0, 0, 1);
    glGetFloatv(GL_MODELVIEW_MATRIX, model_view);

    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, model_view);

    glBindVertexArray(emptyVAO);
    glUseProgram(renderProg);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    
    GLint mvpLoc = glGetUniformLocation(renderProg, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
    GLint timeLoc = glGetUniformLocation(renderProg, "iTime");
    glUniform1f(timeLoc, sim_time);
    
    glPointSize(3.0f);
    glDrawArrays(GL_POINTS, 0, numParticles);

    glutSwapBuffers();
}

void idle() {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto curTime = std::chrono::high_resolution_clock::now();
    sim_time = std::chrono::duration<float>(curTime - startTime).count();

    moveShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    moveShader->setUniform("iTime", UNI_FLOAT_1, &sim_time);
    GLuint n = (GLuint)numParticles;
    // Use raw GL to ensure uint consistency
    GLint nLoc = glGetUniformLocation(moveShader->getProgram(), "numParticles");
    glUniform1ui(nLoc, n);
    
    glDispatchCompute(numParticles / 256 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glutPostRedisplay();
}

int main(int argc, char* argv[]) {
    if (chdir("flight_sim") != 0) chdir("../flight_sim");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutCreateWindow("Particle Rendering Integrity Test");
    glewInit();
    SetupRC();
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
