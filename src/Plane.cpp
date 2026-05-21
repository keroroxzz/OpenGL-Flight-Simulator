#include "Plane.h"

F22::F22()
{
	body_o = new ObjModel("f22/f22_body.obj");
	rudder_o = new ObjModel("f22/f22_rudder.obj");
	aileronL_o = new ObjModel("f22/f22_aileron.obj");
	aileronR_o = new ObjModel("f22/f22_aileron.obj", 0.0, 0.0, 1.0, -1.0, 1.0);
	flapL_o = new ObjModel("f22/f22_flap.obj");
	flapR_o = new ObjModel("f22/f22_flap.obj",0.0,0.0,1.0,-1.0,1.0);
	elevatorL_o = new ObjModel("f22/f22_elevator.obj");
	elevatorR_o = new ObjModel("f22/f22_elevator.obj", 0.0, 0.0, 1.0, -1.0, 1.0);
	glass_o = new ObjModel("f22/f22_glass.obj");

	plane = new DynamicModel(body_o, 19700.0);
	glass = new DynamicModel(glass_o, 0.0);

	M3DMatrix33f inertia = { 90000.0,0.0,-12512.33, 0.0,300000.0,0.0, -12512.33,0.0, 350000.0 };
	plane->setInertia(inertia);

	aerofoilL = new Aerofoil(nullptr, 1.0, 0.2, 50.0, 0.5, 0.2, 7.05, 0.0);
	aerofoilR = new Aerofoil(nullptr, 1.0, 0.2, 50.0, 0.5, 0.2, 7.05, 0.0);
	aileronL = new Aerofoil(aileronL_o, 0.0, 0.0, 1.5, 0.0, 0.0, 7.05, 0.0);
	aileronR = new Aerofoil(aileronR_o, 0.0, 0.0, 1.5, 0.0, 0.0, 7.05, 0.0);
	flapL = new Aerofoil(flapL_o, 0.0, 0.0, 5.0, 0.0, 0.0, 7.05, 0.0);
	flapR = new Aerofoil(flapR_o, 0.0, 0.0, 5.0, 0.0, 0.0, 7.05, 0.0);
	rudderL = new Aerofoil(rudder_o, 0.0, 0.0, 2.0, 0.0, 0.0, 7.05, 0.0);
	rudderR = new Aerofoil(rudder_o, 0.0, 0.0, 2.0, 0.0, 0.0, 7.05, 0.0);
	elevatorL = new Aerofoil(elevatorL_o, 0.0, 0.0, 20.0, 0.0, 0.0, 7.05, 0.0);
	elevatorR = new Aerofoil(elevatorR_o, 0.0, 0.0, 20.0, 0.0, 0.0, 7.05, 0.0);
	rudderBigL = new Aerofoil(nullptr, 0.0, 0.0, 25.0, 0.0, 0.0, 7.05, 0.0);
	rudderBigR = new Aerofoil(nullptr, 0.0, 0.0, 25.0, 0.0, 0.0, 7.05, 0.0);
	
	thruster = new Thruster(nullptr, 156000 * 2);

	rudderL->setPostion(-0.40, -0.2, 0.0);
	rudderR->setPostion(-0.40, -0.2, 0.0);
	flapL->setPostion(-0.6, 0.23, 0.0);
	flapR->setPostion(-0.6, -0.23, 0.0);
	aileronL->setPostion(-0.3, -0.15, 0.0);
	aileronR->setPostion(-0.3, 0.15, 0.0);

	M3DVector3f glassp = { 5.6, 0.0, 1.0 };
	glass_j = new FixedJoint(plane, glass, glassp, 0.0, 0.0, 0.0);
	
	M3DVector3f afl = { 0.0, 4.0, 0.0 };
	M3DVector3f afr = { 0.0, -4.0, 0.0 };
	aerofoilL_j = new FixedJoint(plane, aerofoilL, afl, 0.0, 0.0, 0.0);
	aerofoilR_j = new FixedJoint(plane, aerofoilR, afr, 0.0, 0.0, 0.0);
	
	M3DVector3f all = { -4.35, 5.2, 0.2 };
	M3DVector3f alr = { -4.35, -5.2, 0.2 };
	aileronL_j = new RevoluteJoint(plane, aileronL, all, -3.0, 0.0, -17.0, 0.0, 1.0, 0.0);
	aileronR_j = new RevoluteJoint(plane, aileronR, alr, 3.0, 0.0, 17.0, 0.0, 1.0, 0.0);

	M3DVector3f fpl = { -4.55, 3.0, 0.3 };
	M3DVector3f fpr = { -4.55, -3.0, 0.3 };
	flapL_j = new RevoluteJoint(plane, flapL, fpl, -3.0, 0.0, -17.0, 0.0, 1.0, 0.0);
	flapR_j = new RevoluteJoint(plane, flapR, fpr, 3.0, 0.0, 17.0, 0.0, 1.0, 0.0);
	
	M3DVector3f rdl = { -5.65, 2.25, 2.05 };
	M3DVector3f rdr = { -5.65, -2.25, 2.05 };
	rudderL_j = new RevoluteJoint(plane, rudderL, rdl, 65.0, 0.0, -22.0, 0.0, 1.0, 0.0);
	rudderR_j = new RevoluteJoint(plane, rudderR, rdr, 115.0, 0.0, -22.0, 0.0, 1.0, 0.0);
	
	M3DVector3f ell = { -7.6, 2.9, 0.3 };
	M3DVector3f elr = { -7.6, -2.9, 0.3 };
	elevatorR_j = new RevoluteJoint(plane, elevatorL, ell, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	elevatorL_j = new RevoluteJoint(plane, elevatorR, elr, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	M3DVector3f rlp = { -4.65, 2.25, 2.05 };
	M3DVector3f rrp = { -4.65, -2.25, 2.05 };
	rudderBigR_j = new FixedJoint(plane, rudderBigR, rrp, 115.0, 0.0, 0.0);
	rudderBigL_j = new FixedJoint(plane, rudderBigL, rlp, 65.0, 0.0, 0.0);

	M3DVector3f tp = { -7, 0.0, 0.0 };
	thrust_j = new RevoluteJoint(plane, thruster, tp, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    flowField = new FlowField(1000);
    fluidSolver = new FluidSolver(24, 16, 16);
    m3dCopyVector3(gridOrigin, plane->wpos);

	plane->updatePositionVelocity(plane);
}

void F22::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
	plane->display(cvmatrix, model_view_loc);
}

void F22::drawFlowField(M3DMatrix44f cvmatrix)
{
    flowField->draw(cvmatrix, plane->wvel);
}

void F22::drawSlice(M3DMatrix44f cvmatrix, Shader* sliceShader)
{
    if (!sliceShader) return;

    // Get Fluid Texture
    GLuint tex = fluidSolver->get3DTexture();

    // Build the transformation matrix from Slice Space to Grid Space
    M3DMatrix44f sliceMat;
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(sliceDist, 0, 0);
    glRotatef(sliceRoll, 1, 0, 0);
    glRotatef(slicePitch, 0, 1, 0);
    glGetFloatv(GL_MODELVIEW_MATRIX, sliceMat);
    glPopMatrix();

    glPushMatrix();
    glLoadMatrixf(cvmatrix);
    
    // Transform to plane local space in the world
    glMultMatrixf(plane->waxis);

    // Apply the SAME transforms as captured in sliceMat for rendering position
    glTranslatef(sliceDist, 0, 0);
    glRotatef(sliceRoll, 1, 0, 0);
    glRotatef(slicePitch, 0, 1, 0);

    sliceShader->use();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, tex);
    sliceShader->setUniform("fluidTex", UNI_TEXTURE, nullptr, 1, GL_FALSE, 0);
    sliceShader->setUniform("sliceMatrix", UNI_MATRIX_4, &sliceMat[0]);
    
    float speed = m3dGetMagnitude(plane->wvel);
    sliceShader->setUniform("speedScale", UNI_FLOAT_1, &speed, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);

    // Draw a large enough quad to cover the grid
    glBegin(GL_QUADS);
    glVertex3f(0, -15, -15);
    glVertex3f(0,  15, -15);
    glVertex3f(0,  15,  15);
    glVertex3f(0, -15,  15);
    glEnd();

    glUseProgram(0);
    glDisable(GL_BLEND);
    glPopMatrix();
}

void F22::rotateSlice(float roll, float pitch)
{
    sliceRoll += roll;
    slicePitch += pitch;
}

void F22::moveSlice(float forward)
{
    sliceDist += forward;
}

void F22::visualize(M3DMatrix44f cvmatrix)
{
	plane->visualize(cvmatrix);

    // Get inverse world matrix for force projection
    M3DMatrix44f invWaxis;
    m3dInvertMatrix44(invWaxis, plane->waxis);

    // Draw CFD forces
    plane->applyCFDAerodynamics(plane, true, cvmatrix, fluidSolver, gridOrigin, invWaxis);
    for(auto child : plane->children) {
         child->getChild()->applyCFDAerodynamics(plane, true, cvmatrix, fluidSolver, gridOrigin, invWaxis);
    }
}

void F22::mouseControl(float x, float y)
{
	elevatorL_j->moveAngle(x * 30.0 + y * 30.0, 0.4);
	elevatorR_j->moveAngle(-x * 30.0 + y * 30.0, 0.4);
	
	aileronL_j->moveAngle(-x * 45.0, 0.5);
	aileronR_j->moveAngle(x * 45.0, 0.5);

	flapL_j->moveAngle(-x * 30.0 + y * 30.0 , 0.3);
	flapR_j->moveAngle(x * 30.0 + y * 30.0, 0.3);

	// rudderR_j->moveAngle(x * 40.0, 0.5);
	// rudderL_j->moveAngle(x * 40.0, 0.5);

	thrust_j->moveAngle(y * 30.0, 0.4);
}

void F22::updatePhysic()
{
    extern float dt;
    static int fluidUpdateCounter = 0;
    
	for (int i = 0; i < 1; i++)
	{
		plane->updatePositionVelocity(plane);

        // Get inverse world matrix for force projection
        M3DMatrix44f invWaxis;
        m3dInvertMatrix44(invWaxis, plane->waxis);

        // Apply CFD aerodynamics and inject into FluidSolver
        plane->applyCFDAerodynamics(plane, false, nullptr, fluidSolver, gridOrigin, invWaxis);
        for(auto child : plane->children) {
             child->getChild()->applyCFDAerodynamics(plane, false, nullptr, fluidSolver, gridOrigin, invWaxis);
        }

        // Move grid with plane
        m3dCopyVector3(gridOrigin, plane->wpos);
        M3DVector3f forward = {plane->waxis[0], plane->waxis[1], plane->waxis[2]};
        m3dScaleVector3(forward, 5.0f);
        m3dSubtractVectors3(gridOrigin, gridOrigin, forward);

        // Apply Thruster force explicitly
        thruster->applyEffect(plane);

		M3DVector3f gravity = { 0.0,0.0,-12000.0 * 9.81 };
		plane->applyForce(gravity);

		plane->updateDynamic();
	}

    fluidUpdateCounter++;
    if (fluidUpdateCounter >= 10) {
        fluidSolver->step(dt * 10.0f, plane->wvel, plane->waxis);
        fluidUpdateCounter = 0;
    }

    flowField->update(dt, plane->wpos, plane->wvel, plane->waxis, fluidSolver, gridOrigin);
}

void F22::thrustControl(float v)
{
	float t = thruster->getMaxThrust()*v;

	throttle = thruster->setThrust(t, 0.3);
}

void F22::setPosition(M3DVector3f pos)
{
	plane->setPostion(pos[0], pos[1], pos[2]);
}

void F22::setDisplacement(M3DVector3f displacement)
{
	M3DVector3f pos;
	getPosition(pos);
	m3dAddVectors3(pos, pos, displacement);
	setPosition(pos);
}

void F22::getPosition(M3DVector3f pos)
{
	m3dCopyVector3(pos, plane->position);
}

void F22::getVelocity(M3DVector3f v)
{
	m3dCopyVector3(v, plane->velocity);
}

void F22::getX(M3DVector3f x)
{
	m3dCopyVector3(x, &plane->axis[0]);
}

void F22::getZ(M3DVector3f z)
{
	m3dCopyVector3(z, &plane->axis[8]);
}