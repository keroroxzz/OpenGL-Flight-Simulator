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

	plane->updatePositionVelocity(plane);
}

void F22::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
	plane->display(cvmatrix, model_view_loc);
}

void F22::visualize(M3DMatrix44f cvmatrix)
{
	plane->visualize(cvmatrix);
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
	for (int i = 0; i < 1; i++)
	{

		glPushMatrix();
		glLoadIdentity();
		plane->updatePositionVelocity(plane);
		glPopMatrix();

		glPushMatrix();
		plane->updateEffect(plane);
		glPopMatrix();

		M3DVector3f gravity = { 0.0,0.0,-12000.0 * 9.81 };
		plane->applyForce(gravity);

		plane->updateDynamic();
	}
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