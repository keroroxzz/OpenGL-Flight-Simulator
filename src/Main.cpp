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
#include <chrono>

bool showForce = true;
bool isShowingForce = false;
bool isShowingF22 = true;
bool isShowingSlice = false;
bool isWindTunnelMode = false;
M3DVector3f windVelocity = {30.0f, 0.0f, 0.0f};

bool running = false;
float sim_time = 0.0;
GLfloat gravity = -9.81;


M3DVector3f sun = { 0.0, 0.0, 1.0 };
M3DVector3f sun_target = { 0.0, 0.5, 1.0 };
M3DVector2f pmouse = {0.0,0.0};
M3DMatrix33f sunRot, sunRotR;

GLuint texture, map_texture;
Shader* skyShader = nullptr;
Shader* lightingShader = nullptr;
Shader* sliceShader = nullptr;

GLint windowWidth = 1024;               // window size
GLint windowHeight = 512;

GLuint fbo_shadow, shadow_texture, shadow_buffer;

M3DMatrix44f projection, model_view, model_view_proj;
M3DMatrix44f inverse_model_view, inverse_model_view_proj;
M3DMatrix33f inverse_model_view_rot;

M3DMatrix44f plane_coord;
GLfloat planePos[] = {  0.0f, 0.0f, 0.0f, 1.0f };
GLfloat cameraPos[] = { -11.218950, -11.772061, 1.376742 };
GLfloat targetCamPos[] = { -11.218950, -11.772061, 1.376742 };
GLdouble cameraZoom = 0.5;
GLfloat camax = 1.0, camay = 0.7;

ObjModel* map;
F22* f22;
float throttleFactor = 0.0;

chrono::high_resolution_clock::time_point pclock = chrono::high_resolution_clock::now();

void ReloadShaders(bool counting);

M3DVector3f cameraFocus = { 0.0f, 0.0f, 0.0f };

void UpdateCameraPose()
{
    M3DVector3f nv, newZ;
    f22->getPosition(planePos);
    
    // In normal flight, camera follows plane
    if (!isWindTunnelMode) {
        m3dCopyVector3(cameraFocus, planePos);
    }

    // Safety check for NaN plane position
    if (std::isnan(planePos[0]) || std::isnan(planePos[1]) || std::isnan(planePos[2])) {
        M3DVector3f zero = {0,0,0};
        f22->setPosition(zero);
        return;
    }

    f22->getVelocity(nv);
    float vMag = m3dGetMagnitude(nv);
    if (vMag > 0.1f) {
        m3dNormalizeVector(nv);
    } else {
        // Fallback for stationary plane
        nv[0] = 1.0f; nv[1] = 0.0f; nv[2] = 0.0f;
    }
    
    f22->getX(&plane_coord[0]);
    f22->getZ(&newZ[0]);

    // Update local Up vector with damping
    m3dScaleVector3(&plane_coord[8], 0.96f);
    m3dScaleVector3(newZ, 0.04f);
    m3dAddVectors3(&plane_coord[8], &plane_coord[8], newZ);

    if(!running) return;

    // Only update targetCamPos automatically in normal flight mode
    if (!isWindTunnelMode) {
        M3DVector3f camBack, camSpeed;

        // Use current Up and Velocity to position camera
        camSpeed[0] = planePos[0] + plane_coord[8] * 8.0f - nv[0] * 2.0f;
        camSpeed[1] = planePos[1] + plane_coord[9] * 8.0f - nv[1] * 2.0f;
        camSpeed[2] = planePos[2] + plane_coord[10] * 8.0f - nv[2] * 2.0f;

        camBack[0] = planePos[0] + plane_coord[8] * 5.0f - plane_coord[0] * (3.0f + throttleFactor * 2.0f);
        camBack[1] = planePos[1] + plane_coord[9] * 5.0f - plane_coord[1] * (3.0f + throttleFactor * 2.0f);
        camBack[2] = planePos[2] + plane_coord[10] * 5.0f - plane_coord[2] * (3.0f + throttleFactor * 2.0f);

        m3dScaleVector3(camBack, throttleFactor*0.9f);
        m3dScaleVector3(camSpeed, 1.0f - throttleFactor*0.9f);
        
        M3DVector3f newCamPos;
        m3dAddVectors3(newCamPos, camSpeed, camBack);
        
        // Final guard against universe blowup
        if (!std::isnan(newCamPos[0])) {
            m3dCopyVector3(targetCamPos, newCamPos);
        }
    }
}

void DrawWindArrow(M3DMatrix44f mv)
{
    float speed = m3dGetMagnitude(windVelocity);
    if (speed < 0.1f) return;

    glPushMatrix();
    glLoadMatrixf(mv);
    
    // Position the arrow slightly in front and to the side of the focus point
    glTranslatef(cameraFocus[0] + plane_coord[0]*10.0f, 
                 cameraFocus[1] + plane_coord[1]*10.0f + 10.0f, 
                 cameraFocus[2] + plane_coord[2]*10.0f);

    glDisable(GL_LIGHTING);
    glLineWidth(4.0f);
    glColor3f(1.0f, 1.0f, 0.0f); // Bright Yellow

    M3DVector3f dir;
    m3dCopyVector3(dir, windVelocity);
    m3dScaleVector3(dir, 1.0f / speed);

    // Arrow length scales with speed
    float length = 2.0f + speed * 0.1f;

    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(dir[0] * length, dir[1] * length, dir[2] * length);
    glEnd();

    // Draw an arrow head
    glPushMatrix();
    glTranslatef(dir[0] * length, dir[1] * length, dir[2] * length);
    
    // Simple cone-like head using spheres/cubes or just more lines
    glutSolidSphere(0.3f, 8, 8);
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void RenderShadowDepth(M3DMatrix44f mvp, M3DMatrix44f mv)
{
    
    // render shadhow buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_shadow);
	glBindTexture(GL_TEXTURE_2D, map_texture);
    glViewport(0,0,2048,2048);

    // get shadow buffer
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 50.0f);

    M3DMatrix44f projection_shadow;
    glGetFloatv(GL_PROJECTION_MATRIX, projection_shadow);

    M3DVector3f up;
    M3DVector3f zaxis = {0.0,0.0,1.0};

    m3dCrossProduct(up, sun_target, zaxis);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cameraFocus[0]+sun_target[0]*5.0, cameraFocus[1]+sun_target[1]*5.0, cameraFocus[2]+sun_target[2]*5.0, cameraFocus[0], cameraFocus[1], cameraFocus[2], 0.0, 0.0, 1.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);

    m3dMatrixMultiply44(mvp, projection_shadow, mv);

    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    f22->display(mv);

    if (isShowingForce){
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
    
    UpdateCameraPose();
    M3DMatrix44f model_view_proj_shadow, model_view_shadow;
    RenderShadowDepth(model_view_proj_shadow, model_view_shadow);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    throttleFactor = throttleFactor*0.99 + f22->getThrottle()*0.01;
    float zoom = (throttleFactor * 0.25 + 0.35)*cameraZoom;

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
    gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2], cameraFocus[0], cameraFocus[1], cameraFocus[2], plane_coord[8], plane_coord[9], plane_coord[10]);
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
        
        if (isWindTunnelMode) {
            // Light comes from camera position (Spotlight feel)
            M3DVector3f camDir;
            m3dSubtractVectors3(camDir, cameraPos, planePos);
            m3dNormalizeVector(camDir);
            lightingShader->setUniform("SunDirection", UNI_VEC_3, camDir);
        } else {
            lightingShader->setUniform("SunDirection", UNI_VEC_3, sun);
        }

        GLint model_view_loc = lightingShader->uniformLocation("ModelViewMatrix");

        glActiveTexture(GL_TEXTURE0+0);
        glBindTexture(GL_TEXTURE_2D, texture);
        lightingShader->setUniform("sampler0", UNI_TEXTURE, 0, 1, 0U, 0);

        glActiveTexture(GL_TEXTURE0+1);
        glBindTexture(GL_TEXTURE_2D, shadow_texture);
        lightingShader->setUniform("sampler1", UNI_TEXTURE, 0, 1, 0U, 1);

        f22->display(model_view, model_view_loc);
    }

    glUseProgram(0);

    // Draw immediate-mode visual effects outside the shader
    if (isShowingF22) {
        f22->drawFlowField(model_view);
    }

    if (isShowingSlice) {
        f22->drawSlice(model_view, sliceShader);
    }

    if (isWindTunnelMode) {
        DrawWindArrow(model_view);
    }

    if (isShowingForce){
        glPushMatrix();
        f22->visualize(model_view);
        glPopMatrix();
    }

    if (!isWindTunnelMode) {
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
        glBindTexture(GL_TEXTURE_2D, map_texture);
        skyShader->setUniform("sampler0", UNI_TEXTURE, 0);

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
    }

    glUseProgram(0);

    // Flush drawing commands
    glutSwapBuffers();

    printf("shader time: %f\n", (chrono::high_resolution_clock::now()-pclock).count()/1000000.0);
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
    
	//check the status of the frame buffer
	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		printf("Could not validate framebuffer");
	}

	printf("Generate texture %d and fbo %d.\n", *texture, *fboname);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ProcessMenu(int value)
{
    M3DVector3f zero = { 0,0,0 };
    switch (value)
    {
    case 1: 
        running = !running; 
        printf("[Menu] Simulation %s\n", running ? "STARTED" : "PAUSED");
        break;
    case 2: 
        f22->setPosition(zero);
        f22->thrustControl(0);
        printf("[Menu] Aircraft RESET to origin.\n");
        break;
    case 3: 
        isWindTunnelMode = !isWindTunnelMode;
        if (isWindTunnelMode) {
             f22->setPosition(zero);
             running = true; 
        }
        printf("[Menu] Wind Tunnel Mode: %s\n", isWindTunnelMode ? "ENABLED (Plane static)" : "DISABLED (Free flight)");
        break;

    case 10: 
        isShowingForce = !isShowingForce; 
        printf("[Menu] Force Vectors: %s\n", isShowingForce ? "ON" : "OFF");
        break;
    case 11: 
        isShowingF22 = !isShowingF22; 
        printf("[Menu] Streamlines: %s\n", isShowingF22 ? "ON" : "OFF");
        break;
    case 12: 
        isShowingSlice = !isShowingSlice; 
        printf("[Menu] CFD Slice: %s\n", isShowingSlice ? "ON" : "OFF");
        break;
    case 13: 
        ReloadShaders(false); 
        printf("[Menu] Shaders reloaded.\n");
        break;

    case 20: windVelocity[0] += 10.0f; break;
    case 21: windVelocity[0] -= 10.0f; break;
    case 22: windVelocity[1] += 5.0f; break;
    case 23: windVelocity[1] -= 5.0f; break;
    case 24: windVelocity[2] += 5.0f; break;
    case 25: windVelocity[2] -= 5.0f; break;
    case 26: 
        windVelocity[0] = 30.0f;
        windVelocity[1] = 0.0f;
        windVelocity[2] = 0.0f;
        printf("[Menu] Wind reset to default.\n");
        break;
    case 100: exit(0); break;
    }
    if (value >= 20 && value <= 25) {
        printf("[Wind] Current Velocity: [%.1f, %.1f, %.1f] m/s\n", windVelocity[0], windVelocity[1], windVelocity[2]);
    }
    glutPostRedisplay();
}

//initialization
void SetupRC()
{
    GLint i;

    fprintf(stdout, "Simple Flight Simulator by Brian\n");

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Misc. state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    GLbyte* tga;
    GLint width, height, component;
    GLenum format;

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glActiveTexture(GL_TEXTURE0);
    //Load Image
    tga = gltLoadTGA("f22/f22.tga", &width, &height, &component, &format);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, component, width, height, 0, format, GL_UNSIGNED_BYTE, (void*)tga);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    free(tga);

    
    //Load map texture
    glActiveTexture(GL_TEXTURE0);
    tga = gltLoadTGA("map/map.tga", &width, &height, &component, &format);
    glGenTextures(1, &map_texture);
    glBindTexture(GL_TEXTURE_2D, map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, component, width, height, 0, format, GL_UNSIGNED_BYTE, (void*)tga);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    free(tga);

	prepareFBO(GL_TEXTURE0, &fbo_shadow, &shadow_buffer, &shadow_texture, 2048, 2048, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

    glEnable(GL_TEXTURE_2D);

    // Create Menus
    int nSimMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Play/Pause (F1)", 1);
    glutAddMenuEntry("Reset Aircraft", 2);
    glutAddMenuEntry("Toggle Wind Tunnel Mode (F6)", 3);

    int nVisMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Toggle Force Vectors (F2)", 10);
    glutAddMenuEntry("Toggle Streamlines (F4)", 11);
    glutAddMenuEntry("Toggle CFD Slice (F5)", 12);
    glutAddMenuEntry("Reload Shaders (F3)", 13);

    int nWindMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Increase Speed (+10)", 20);
    glutAddMenuEntry("Decrease Speed (-10)", 21);
    glutAddMenuEntry("Rotate Wind Right", 22);
    glutAddMenuEntry("Rotate Wind Left", 23);
    glutAddMenuEntry("Rotate Wind Up", 24);
    glutAddMenuEntry("Rotate Wind Down", 25);
    glutAddMenuEntry("Reset to 30m/s Forward", 26);

    glutCreateMenu(ProcessMenu);
    glutAddSubMenu("Simulation", nSimMenu);
    glutAddSubMenu("Visualization", nVisMenu);
    glutAddSubMenu("Wind Tunnel Settings", nWindMenu);
    glutAddMenuEntry("Exit (Q)", 100);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void KeyPressFunc(unsigned char key, int x, int y)
{
    M3DMatrix33f sunEx, newSun;

    switch (key)
    {
    case 'w':
        if (isWindTunnelMode) {
             cameraFocus[0] += plane_coord[0] * 0.5f;
             cameraFocus[1] += plane_coord[1] * 0.5f;
             cameraFocus[2] += plane_coord[2] * 0.5f;
             targetCamPos[0] += plane_coord[0] * 0.5f;
             targetCamPos[1] += plane_coord[1] * 0.5f;
             targetCamPos[2] += plane_coord[2] * 0.5f;
        } else f22->thrustControl(1.0);
        break;
    case 's':
        if (isWindTunnelMode) {
             cameraFocus[0] -= plane_coord[0] * 0.5f;
             cameraFocus[1] -= plane_coord[1] * 0.5f;
             cameraFocus[2] -= plane_coord[2] * 0.5f;
             targetCamPos[0] -= plane_coord[0] * 0.5f;
             targetCamPos[1] -= plane_coord[1] * 0.5f;
             targetCamPos[2] -= plane_coord[2] * 0.5f;
        } else f22->thrustControl(0.0);
        break;
    case 'a':
        if (isWindTunnelMode) {
             cameraFocus[0] -= plane_coord[4] * 0.5f;
             cameraFocus[1] -= plane_coord[5] * 0.5f;
             cameraFocus[2] -= plane_coord[6] * 0.5f;
             targetCamPos[0] -= plane_coord[4] * 0.5f;
             targetCamPos[1] -= plane_coord[5] * 0.5f;
             targetCamPos[2] -= plane_coord[6] * 0.5f;
        } break;
    case 'd':
        if (isWindTunnelMode) {
             cameraFocus[0] += plane_coord[4] * 0.5f;
             cameraFocus[1] += plane_coord[5] * 0.5f;
             cameraFocus[2] += plane_coord[6] * 0.5f;
             targetCamPos[0] += plane_coord[4] * 0.5f;
             targetCamPos[1] += plane_coord[5] * 0.5f;
             targetCamPos[2] += plane_coord[6] * 0.5f;
        } break;
    case 'r':
        if (isWindTunnelMode) {
             cameraFocus[0] += plane_coord[8] * 0.5f;
             cameraFocus[1] += plane_coord[9] * 0.5f;
             cameraFocus[2] += plane_coord[10] * 0.5f;
             targetCamPos[0] += plane_coord[8] * 0.5f;
             targetCamPos[1] += plane_coord[9] * 0.5f;
             targetCamPos[2] += plane_coord[10] * 0.5f;
        } break;
    case 'f':
        if (isWindTunnelMode) {
             cameraFocus[0] -= plane_coord[8] * 0.5f;
             cameraFocus[1] -= plane_coord[9] * 0.5f;
             cameraFocus[2] -= plane_coord[10] * 0.5f;
             targetCamPos[0] -= plane_coord[8] * 0.5f;
             targetCamPos[1] -= plane_coord[9] * 0.5f;
             targetCamPos[2] -= plane_coord[10] * 0.5f;
        } break;

    // Slice Controls
    case 'i': f22->moveSlice(0.5f); break;
    case 'k': f22->moveSlice(-0.5f); break;
    case 'j': f22->rotateSlice(0.0f, 5.0f); break;
    case 'l': f22->rotateSlice(0.0f, -5.0f); break;
    case 'u': f22->rotateSlice(5.0f, 0.0f); break;
    case 'o': f22->rotateSlice(-5.0f, 0.0f); break;

    // Wind Controls
    case '[': windVelocity[0] -= 5.0f; break;
    case ']': windVelocity[0] += 5.0f; break;
    case ';': windVelocity[1] -= 5.0f; break;
    case '\'': windVelocity[1] += 5.0f; break;
    case '.': windVelocity[2] -= 5.0f; break;
    case '/': windVelocity[2] += 5.0f; break;

    case 'q': exit(0); break;
    }

    // Refresh the Window
    glutPostRedisplay();
}

void ReloadShaders(bool counting)
{
    delete skyShader;
    delete lightingShader;
    delete sliceShader;

    skyShader = new Shader(2);
    skyShader->addFromFile("shaders/vertex/cloudworks.vs", GL_VERTEX_SHADER);
    skyShader->addFromFile("shaders/frag/cloudworks.fs", GL_FRAGMENT_SHADER);

    lightingShader = new Shader(2);
    lightingShader->addFromFile("shaders/vertex/lighting.vs", GL_VERTEX_SHADER);
    lightingShader->addFromFile("shaders/frag/lighting.fs", GL_FRAGMENT_SHADER);

    sliceShader = new Shader(2);
    sliceShader->addFromFile("shaders/vertex/slice.vs", GL_VERTEX_SHADER);
    sliceShader->addFromFile("shaders/frag/slice.fs", GL_FRAGMENT_SHADER);
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
        printf("[Key] Simulation %s\n", running ? "STARTED" : "PAUSED");
        break;
    case GLUT_KEY_F2:
        isShowingForce = !isShowingForce;
        printf("[Key] Force Vectors: %s\n", isShowingForce ? "ON" : "OFF");
        break;
    case GLUT_KEY_F3:
        ReloadShaders(false);
        printf("[Key] Shaders reloaded.\n");
        break;
    case GLUT_KEY_F4:
        isShowingF22 = !isShowingF22;
        printf("[Key] Streamlines: %s\n", isShowingF22 ? "ON" : "OFF");
        break;
    case GLUT_KEY_F5:
        isShowingSlice = !isShowingSlice;
        printf("[Key] CFD Slice: %s\n", isShowingSlice ? "ON" : "OFF");
        break;
    case GLUT_KEY_F6:
        isWindTunnelMode = !isWindTunnelMode;
        if (isWindTunnelMode) {
            M3DVector3f zero = {0,0,0};
            f22->setPosition(zero);
            running = true;
        }
        printf("[Key] Wind Tunnel Mode: %s\n", isWindTunnelMode ? "ENABLED" : "DISABLED");
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
            f22->updatePhysic(isWindTunnelMode, windVelocity);
    glPopMatrix();

    cameraPos[0] = cameraPos[0]*0.9 + targetCamPos[0]*0.1;
    cameraPos[1] = cameraPos[1]*0.9 + targetCamPos[1]*0.1;
    cameraPos[2] = cameraPos[2]*0.9 + targetCamPos[2]*0.1;

    sim_time += 0.001;
    usleep(10);
}

void mousePressed(int x_, int y_)
{
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;

    M3DVector3f p = {0.0};
    p[0]=-(x-pmouse[0])*10.0;
    p[1]=(y-pmouse[1])*10.0;

    M3DVector3f d;
    m3dRotateVector(d, p, inverse_model_view_rot);
    
    M3DVector3f plan_to_camera;
    m3dSubtractVectors3(plan_to_camera, targetCamPos, cameraFocus);
    float radius=m3dGetMagnitude(plan_to_camera);

    m3dAddVectors3(plan_to_camera, plan_to_camera, d);
    m3dNormalizeVector(plan_to_camera);
    m3dScaleVector3(plan_to_camera,radius);
    m3dAddVectors3(targetCamPos, cameraFocus, plan_to_camera);

    pmouse[0]=x;
    pmouse[1]=y;
}

void mouseHover(int x_, int y_)
{
    float x = 2.0 * (float)x_ / windowWidth - 1.0;
    float y = 2.0 * (float)y_ / windowHeight - 1.0;
    pmouse[0]=x;
    pmouse[1]=y;

    f22->mouseControl(x, y);
}

void mouse(int button, int state, int x, int y)
{
    if (button == 4) 
        cameraZoom = cameraZoom*0.9 + 0.3;
    
    else if (button == 3) 
        cameraZoom = cameraZoom*0.9 + 0.05;
}

int main(int argc, char* argv[])
{
    m3dRotationMatrix33(sunRot, 0.015, 0.85, -2.0, 0.45);
    m3dRotationMatrix33(sunRotR, -0.015, 0.85, -2.0, 0.45);

    targetCamPos[0] = -11.218950;
    targetCamPos[1] = -11.772061;
    targetCamPos[2] = 1.376742;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    
    // Request OpenGL 4.6 Compatibility Profile
    glutInitContextVersion(4, 6);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

    glutCreateWindow("Fight Simulator - alpha");

    glewInit();

    ReloadShaders(false);

    glutReshapeFunc(ChangeSize);
    glutKeyboardFunc(KeyPressFunc);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    glutIdleFunc(idle);
    glutMotionFunc(mousePressed);
    glutPassiveMotionFunc(mouseHover);
    glutMouseFunc(mouse);

    f22 = new F22();
    map = new ObjModel("map/map.obj",0.0, 0.0, 26000.0, 26000.0, 26000.0);
    printf("init f22...\n");
    f22->updatePhysic();

    printf("setup...\n");
    SetupRC();
    UpdateCameraPose();

    printf("mainloop...\n");
    glutMainLoop();

    return 0;
}
