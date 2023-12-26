/*
    FlightSimulator by Brian
    Version : Alpha
    Date : 2021/1/5
*/

#define _CRT_SECURE_NO_WARNINGS
#define FREEGLUT_STATIC

#include <pthread.h>
#include "baseHeader.h"
#include "Shader.h"
#include "ObjModel.h"
#include "Plane.h"
#include "TextureGenerator.h"
#include <chrono>

bool showForce = true;
bool isShowingForce = false;
bool isShowingF22 = true;

bool running = false;
float sim_time = 0.0;
GLfloat gravity = -9.81;

M3DVector3f sun = {0.0, 0.0, 1.0};
M3DVector3f sun_target = {0.0, 0.5, 1.0};
M3DVector2f pmouse = {0.0, 0.0};
M3DMatrix33f sunRot, sunRotR;

GLuint texture, map_texture;
Shader *skyShader, *lightingShader;

GLint windowWidth = 1024; // window size
GLint windowHeight = 512;

GLuint fbo_shadow, shadow_texture, shadow_buffer;

M3DMatrix44f projection, model_view, model_view_proj;
M3DMatrix44f inverse_model_view, inverse_model_view_proj;
M3DMatrix33f inverse_model_view_rot;

M3DMatrix44f plane_coord;
GLfloat planePos[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat cameraPos[] = {-11.218950, -11.772061, 1.376742};
GLfloat targetCamPos[] = {-11.218950, -11.772061, 1.376742};
GLdouble cameraZoom = 0.5;
GLfloat camax = 1.0, camay = 0.7;

ObjModel *map;
F22 *f22;
float throttleFactor = 0.0;

TextureGenerator *texgen;

chrono::high_resolution_clock::time_point pclock = chrono::high_resolution_clock::now();

void UpdateCameraPose()
{
    M3DVector3f nv, newZ;
    f22->getPosition(planePos);
    f22->getVelocity(nv);
    m3dNormalizeVector(nv);
    f22->getX(&plane_coord[0]);
    f22->getZ(&newZ[0]);

    m3dScaleVector3(&plane_coord[8], 0.96);
    m3dScaleVector3(newZ, 0.04);
    m3dAddVectors3(&plane_coord[8], &plane_coord[8], newZ);

    if (!running)
        return;

    M3DVector3f camBack, camSpeed;

    camSpeed[0] = planePos[0] + plane_coord[8] * 8.0 - nv[0] * 2.0;
    camSpeed[1] = planePos[1] + plane_coord[9] * 8.0 - nv[1] * 2.0;
    camSpeed[2] = planePos[2] + plane_coord[10] * 8.0 - nv[2] * 2.0;

    camBack[0] = planePos[0] + plane_coord[8] * 5.0 - plane_coord[0] * (3.0 + throttleFactor * 2.0);
    camBack[1] = planePos[1] + plane_coord[9] * 5.0 - plane_coord[1] * (3.0 + throttleFactor * 2.0);
    camBack[2] = planePos[2] + plane_coord[10] * 5.0 - plane_coord[2] * (3.0 + throttleFactor * 2.0);

    m3dScaleVector3(camBack, throttleFactor * 0.9);
    m3dScaleVector3(camSpeed, 1.0 - throttleFactor * 0.9);
    m3dAddVectors3(targetCamPos, camSpeed, camBack);
}

void RenderShadowDepth(M3DMatrix44f mvp, M3DMatrix44f mv)
{

    // render shadhow buffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_shadow);
    glBindTexture(GL_TEXTURE_2D, map_texture);
    glViewport(0, 0, 2048, 2048);

    // get shadow buffer
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 50.0f);

    M3DMatrix44f projection_shadow;
    glGetFloatv(GL_PROJECTION_MATRIX, projection_shadow);

    M3DVector3f up;
    M3DVector3f zaxis = {0.0, 0.0, 1.0};

    m3dCrossProduct(up, sun_target, zaxis);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(planePos[0] + sun_target[0] * 5.0, planePos[1] + sun_target[1] * 5.0, planePos[2] + sun_target[2] * 5.0, planePos[0], planePos[1], planePos[2], 0.0, 0.0, 1.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);

    m3dMatrixMultiply44(mvp, projection_shadow, mv);

    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    f22->display(mv);

    if (isShowingForce)
    {
        glPushMatrix();
        f22->visualize(mv);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(10000.0, 0.0, -700.0);
    map->draw(2, GL_TRIANGLES);
    glPopMatrix();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

// Called to draw scene
void RenderScene(void)
{
    // delete texgen;
    // texgen = new TextureGenerator(256, 256, 256);
    // texgen->reloadShaders();
    texgen->generateTexture();

    UpdateCameraPose();
    M3DMatrix44f model_view_proj_shadow, model_view_shadow;
    RenderShadowDepth(model_view_proj_shadow, model_view_shadow);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    throttleFactor = throttleFactor * 0.99 + f22->getThrottle() * 0.01;
    float zoom = (throttleFactor * 0.25 + 0.35) * cameraZoom;

    if (windowWidth > windowHeight)
    {
        GLdouble ar = (GLdouble)windowWidth / (GLdouble)windowHeight;
        glFrustum(-ar * zoom, ar * zoom, -zoom, zoom, 1.0, 15000.0f);
    }
    else
    {
        GLdouble ar = (GLdouble)windowHeight / (GLdouble)windowWidth;
        glFrustum(-zoom, zoom, -ar * zoom, ar * zoom, 1.0, 15000.0f);
    }

    glGetFloatv(GL_PROJECTION_MATRIX, projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2], planePos[0], planePos[1], planePos[2], plane_coord[8], plane_coord[9], plane_coord[10]);
    glGetFloatv(GL_MODELVIEW_MATRIX, model_view);

    m3dMatrixMultiply44(model_view_proj, projection, model_view);

    m3dInvertMatrix44(inverse_model_view, model_view);
    m3dInvertMatrix44(inverse_model_view_proj, model_view_proj);

    m3dCopyVector3(&inverse_model_view_rot[0], &inverse_model_view[0]);
    m3dCopyVector3(&inverse_model_view_rot[3], &inverse_model_view[4]);
    m3dCopyVector3(&inverse_model_view_rot[6], &inverse_model_view[8]);

    glViewport(0, 0, windowWidth, windowHeight);

    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    if (isShowingF22)
    {
        lightingShader->use();
        lightingShader->setUniform("InvVP", UNI_MATRIX_4, &inverse_model_view_proj[0]);
        lightingShader->setUniform("InvNVP", UNI_MATRIX_3, &inverse_model_view_rot[0]);
        lightingShader->setUniform("ProjectionMatrix", UNI_MATRIX_4, &projection[0]);
        lightingShader->setUniform("ShadowMVP", UNI_MATRIX_4, &model_view_proj_shadow[0]);
        // lightingShader->setUniform("ShadowMV", UNI_MATRIX_4, &model_view_shadow[0]);
        lightingShader->setUniform("windowWidth", UNI_INT_1, &windowWidth);
        lightingShader->setUniform("windowHeight", UNI_INT_1, &windowHeight);
        lightingShader->setUniform("camera", UNI_VEC_4, cameraPos);
        lightingShader->setUniform("SunDirection", UNI_VEC_3, sun);

        GLint model_view_loc = lightingShader->uniformLocation("ModelViewMatrix");

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture);
        lightingShader->setUniform("sampler0", UNI_TEXTURE, 0, 1, 0U, 0);

        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, shadow_texture);
        lightingShader->setUniform("sampler1", UNI_TEXTURE, 0, 1, 0U, 1);

        f22->display(model_view, model_view_loc);
    }

    glUseProgram(0);

    if (isShowingForce)
    {
        glPushMatrix();
        f22->visualize(model_view);
        glPopMatrix();
    }

    skyShader->use();
    skyShader->setUniform("windowWidth", UNI_INT_1, &windowWidth);
    skyShader->setUniform("windowHeight", UNI_INT_1, &windowHeight);
    skyShader->setUniform("InvVP", UNI_MATRIX_4, &inverse_model_view_proj[0]);
    skyShader->setUniform("InvNVP", UNI_MATRIX_3, &inverse_model_view_rot[0]);
    skyShader->setUniform("ProjectionMatrix", UNI_MATRIX_4, &projection[0]);
    skyShader->setUniform("camera", UNI_VEC_4, cameraPos);
    skyShader->setUniform("iTime", UNI_FLOAT_1, &sim_time);
    skyShader->setUniform("SunDirection", UNI_VEC_3, sun);
    GLint model_view_loc = skyShader->uniformLocation("ModelViewMatrix");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    skyShader->setUniform("sampler0", UNI_TEXTURE, 0);

    texgen->bindTexture(GL_TEXTURE0+1);
    skyShader->setUniform("test", UNI_TEXTURE, 0, 1, 0U, 1);

    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(10000.0, 0.0, -700.0);
    map->draw(2, GL_TRIANGLES, model_view_loc);
    glPopMatrix();

    // Draw dome
    glColor3f(0.68f, 0.75f, 0.85f);
    glPushMatrix();
    glTranslatef(cameraPos[0], cameraPos[1], cameraPos[2]);

    glGetFloatv(GL_MODELVIEW_MATRIX, model_view);
    glUniformMatrix4fv(model_view_loc, 1, GL_FALSE, &model_view[0]);
    glutSolidSphere(14900.0f, 100, 100);
    glPopMatrix();

    glUseProgram(0);

    // Flush drawing commands
    glutSwapBuffers();

    // printf("shader time: %f\n", (chrono::high_resolution_clock::now() - pclock).count() / 1000000.0);
    pclock = chrono::high_resolution_clock::now();
}

void prepareFBO(GLenum textureNum, GLuint *fboname, GLuint *dbname, GLuint *texture, int width, int height, GLint internalformat, GLenum format, GLenum type)
{
    // fbo
    glGenFramebuffersEXT(1, fboname);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *fboname);

    // texture
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // depth buffer
    glGenRenderbuffersEXT(1, dbname);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, *dbname);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, *dbname);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *texture, 0);

    // GLenum DrawBuffers[1] = {GL_DEPTH_ATTACHMENT};
    // glDrawBuffers(1, DrawBuffers);

    // check the status of the frame buffer
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        printf("Could not validate framebuffer");
    }

    printf("Generate texture %d and fbo %d.\n", *texture, *fboname);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// initialization
void SetupRC()
{
    GLint i;

    fprintf(stdout, "Simple Flight Simulator by Brian");
    GLint w, h, c;
    // Make sure required functionality is available!
    if (!GL_VERSION_2_0 && (!GL_ARB_fragment_shader ||
                            !GL_ARB_shader_objects ||
                            !GL_ARB_shading_language_100))
    {
        fprintf(stderr, "GLSL extensions not available!\n");
        return;
    }

    // Make sure we have multitexture and cube maps!
    if (!GL_VERSION_1_3 && (!GL_ARB_multitexture ||
                            !GL_ARB_texture_cube_map))
    {
        fprintf(stderr, "Neither OpenGL 1.3 nor necessary"
                        " extensions are available!\n");
        return;
    }

    fprintf(stdout, "Controls:\n");
    fprintf(stdout, "\tRight-click for menu\n\n");
    fprintf(stdout, "\tR/L arrows\t+/- rotate lights for others shaders\n\n");
    fprintf(stdout, "\tx/X\t\tMove +/- in x direction\n");
    fprintf(stdout, "\ty/Y\t\tMove +/- in y direction\n");
    fprintf(stdout, "\tz/Z\t\tMove +/- in z direction\n\n");
    fprintf(stdout, "\tq\t\tExit demo\n\n");

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Misc. state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    GLbyte *tga;
    GLint width, height, component;
    GLenum format;

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glActiveTexture(GL_TEXTURE0);
    // Load Image
    tga = gltLoadTGA("f22/f22.tga", &width, &height, &component, &format);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, component, width, height, 0, format, GL_UNSIGNED_BYTE, (void *)tga);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    free(tga);

    // Load map texture
    glActiveTexture(GL_TEXTURE0);
    tga = gltLoadTGA("map/map.tga", &width, &height, &component, &format);
    glGenTextures(1, &map_texture);
    glBindTexture(GL_TEXTURE_2D, map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, component, width, height, 0, format, GL_UNSIGNED_BYTE, (void *)tga);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    free(tga);

    prepareFBO(GL_TEXTURE0, &fbo_shadow, &shadow_buffer, &shadow_texture, 2048, 2048, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

    glEnable(GL_TEXTURE_2D);
}

void KeyPressFunc(unsigned char key, int x, int y)
{
    M3DMatrix33f sunEx, newSun;

    switch (key)
    {
    case 'w':
        f22->thrustControl(1.0);
        break;
    case 's':
        f22->thrustControl(0.0);
        break;

    case 'q':
        m3dCopyVector3(sunEx, sun_target);
        m3dMatrixMultiply33(newSun, sunRot, sunEx);
        m3dCopyVector3(sun_target, newSun);
        m3dNormalizeVector(sun_target);
        break;

    case 'e':
        m3dCopyVector3(sunEx, sun_target);
        m3dMatrixMultiply33(newSun, sunRotR, sunEx);
        m3dCopyVector3(sun_target, newSun);
        m3dNormalizeVector(sun_target);
        break;

    case '1':
        sun_target[0] = 0.0;
        sun_target[1] = 0.0;
        sun_target[2] = 1.0;
        break;
    }

    // Refresh the Window
    glutPostRedisplay();
}

void ReloadShaders(bool counting = false)
{
    delete skyShader;
    delete lightingShader;

    skyShader = new Shader(2);
    skyShader->addFromFile("shaders/vertex/cloudworks.vs", GL_VERTEX_SHADER);
    skyShader->addFromFile("shaders/frag/cloudworks.fs", GL_FRAGMENT_SHADER);

    lightingShader = new Shader(2);
    lightingShader->addFromFile("shaders/vertex/lighting.vs", GL_VERTEX_SHADER);
    lightingShader->addFromFile("shaders/frag/lighting.fs", GL_FRAGMENT_SHADER);

    texgen->reloadShaders();
    texgen->generateTexture();
}

void SpecialKeys(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT:
        camax -= 0.05f;
        break;
    case GLUT_KEY_RIGHT:
        camax += 0.05f;
        break;
    case GLUT_KEY_UP:
        camay += 0.05f;
        break;
    case GLUT_KEY_DOWN:
        camay -= 0.05f;
        break;
    case GLUT_KEY_F1:
        running = !running;
        break;
    case GLUT_KEY_F2:
        isShowingForce = !isShowingForce;
        break;
    case GLUT_KEY_F3:
        ReloadShaders();
        
        break;
    case GLUT_KEY_F4:
        isShowingF22 = !isShowingF22;
        break;
    case GLUT_KEY_PAGE_UP:
        M3DVector3f d;
        d[2] = 10.0;
        f22->setDisplacement(d);
        break;
    case GLUT_KEY_PAGE_DOWN:
        d[2] = -10.0;
        f22->setDisplacement(d);
        break;
    default:
        break;
    }

    // Refresh the Window
    glutPostRedisplay();
}

void ChangeSize(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
}

void idle()
{
    glutPostRedisplay();

    m3dAddVectors3(sun, sun_target, sun);
    m3dScaleVector3(sun, 0.5);
    m3dNormalizeVector(sun);

    glPushMatrix();
    if (running)
        for (int i = 0; i < 100; i++)
            f22->updatePhysic();
    glPopMatrix();

    cameraPos[0] = cameraPos[0] * 0.9 + targetCamPos[0] * 0.1;
    cameraPos[1] = cameraPos[1] * 0.9 + targetCamPos[1] * 0.1;
    cameraPos[2] = cameraPos[2] * 0.9 + targetCamPos[2] * 0.1;

    sim_time += 0.001;
    usleep(10);
}

void mousePressed(int x_, int y_)
{
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;

    M3DVector3f p = {0.0};
    p[0] = -(x - pmouse[0]) * 10.0;
    p[1] = (y - pmouse[1]) * 10.0;

    M3DVector3f d;
    m3dRotateVector(d, p, inverse_model_view_rot);

    M3DVector3f plan_to_camera;
    m3dSubtractVectors3(plan_to_camera, targetCamPos, planePos);
    float radius = m3dGetMagnitude(plan_to_camera);

    m3dAddVectors3(plan_to_camera, plan_to_camera, d);
    m3dNormalizeVector(plan_to_camera);
    m3dScaleVector3(plan_to_camera, radius);
    m3dAddVectors3(targetCamPos, planePos, plan_to_camera);

    pmouse[0] = x;
    pmouse[1] = y;
}

void mouseHover(int x_, int y_)
{
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;
    pmouse[0] = x;
    pmouse[1] = y;

    // x = (abs(x) < 0.1) ? 0.0 : x;
    // y = (abs(y) < 0.1) ? 0.0 : y;

    f22->mouseControl(x, y);
}

void mouse(int button, int state, int x, int y)
{
    if (button == 4)
        cameraZoom = cameraZoom * 0.9 + 0.3;

    else if (button == 3)
        cameraZoom = cameraZoom * 0.9 + 0.05;
}

int main(int argc, char *argv[])
{
    m3dRotationMatrix33(sunRot, 0.015, 0.85, -2.0, 0.45);
    m3dRotationMatrix33(sunRotR, -0.015, 0.85, -2.0, 0.45);

    targetCamPos[0] = -11.218950;
    targetCamPos[1] = -11.772061;
    targetCamPos[2] = 1.376742;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Fight Simulator - alpha");

    skyShader = new Shader(2);
    skyShader->addFromFile("shaders/vertex/cloudworks.vs", GL_VERTEX_SHADER);
    skyShader->addFromFile("shaders/frag/cloudworks.fs", GL_FRAGMENT_SHADER);

    lightingShader = new Shader(2);
    lightingShader->addFromFile("shaders/vertex/lighting.vs", GL_VERTEX_SHADER);
    lightingShader->addFromFile("shaders/frag/lighting.fs", GL_FRAGMENT_SHADER);

    glutReshapeFunc(ChangeSize);
    glutKeyboardFunc(KeyPressFunc);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMotionFunc(mousePressed);
    glutPassiveMotionFunc(mouseHover);
    glutMouseFunc(mouse);

    f22 = new F22();
    map = new ObjModel("map/map.obj", 0.0, 0.0, 26000.0, 26000.0, 26000.0);
    printf("init f22...\n");
    f22->updatePhysic();

    texgen = new TextureGenerator(64, 64, 64, GL_R16F, GL_MIRRORED_REPEAT);
    texgen->generateTexture();

    printf("setup...\n");
    SetupRC();
    UpdateCameraPose();

    printf("mainloop...\n");
    glutMainLoop();

    return 0;
}
