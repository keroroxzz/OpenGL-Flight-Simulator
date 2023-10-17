#pragma once

#include "baseHeader.h"
#include "ObjModel.h"

#include <vector>

using namespace std;

#ifndef PHY_MOD_H
#define PHY_MOD_H

class Joint;

class DynamicModel
{
	ObjModel *obj;

public:
	//joints
	Joint* parent_joint = nullptr;
	vector<Joint*> children;

	//physic properties
	double mass;
	M3DMatrix44f axis;	//axis of this model in its parent coord
	M3DMatrix33f inertia;
	M3DMatrix33f inv_inertia;

	//world position and velocity
	M3DVector3f wpos, wvel;
	M3DMatrix44f waxis;	//axis of this model in its parent coord

	//properties in its parent coord
	M3DVector3f position;
	M3DVector3f velocity;
	M3DVector3f angular_velocity;

	//model state
	int state = 0;	//0-unavtive 1-active

	DynamicModel(ObjModel* obj, float mass);
	void setInertia(M3DMatrix33f inertia);

	void setPostion(float x, float y, float z) { position[0] = x; position[1] = y; position[2] = z; }
	void rotate(float r, float p, float y);

	void applyForce(M3DVector3f f);
	void applyForce(M3DVector3f p, M3DVector3f f);
	void applyTorque(M3DVector3f t);
	void applyForceModelCoord(M3DVector3f p, M3DVector3f f);
	void updateDynamic();
	void updateChildrenDynamic();

	void updateEffect(DynamicModel* base = nullptr);
	void updateChildrenPV(DynamicModel* base = nullptr);
	void updatePositionVelocity(DynamicModel* base);

	virtual void applyEffect(DynamicModel* base) {};

	void attach(Joint *joint);

	void displayChildren(M3DMatrix44f cvmatrix, GLint model_view_loc);
	void display(M3DMatrix44f cvmatrix, GLint model_view_loc);

	void visualizeChildren(M3DMatrix44f cvmatrix);
	void visualize(M3DMatrix44f cvmatrix);
};

class Aerofoil :public DynamicModel
{
	float area[3], dragCoeff[3], liftFactor;

	Aerofoil();
public:
	Aerofoil(ObjModel* obj, float area_front, float area_side, float area_up, float dx, float dy, float dz, float liftfactor);

	void applyEffect(DynamicModel* base);
};

class Thruster :public DynamicModel
{
	float thrust, max_thrust;

	Thruster();
public:
	Thruster(ObjModel* obj, float max_thrust) :DynamicModel(obj, 0.0), max_thrust(max_thrust) {}

	float setThrust(float target, float ratio);
	float getThrust();
	float getMaxThrust();
	void applyEffect(DynamicModel* base);
};

#endif // !PHY_MOD_H