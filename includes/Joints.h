#pragma once

#include "baseHeader.h"
#include "PhysicalModel.h"

#ifndef JOINTS_H
#define JOINTS_H

class DynamicModel;

class Joint
{
protected:
	DynamicModel* parent;
	DynamicModel* child;

	M3DVector3f position;
	M3DMatrix44f axis;

	M3DMatrix44f T, IT;

	Joint() {}

public:
	Joint(DynamicModel* parent, DynamicModel* child) :parent(parent), child(child) { child->parent_joint = this; }

	void update();
	void updateEffect(DynamicModel* base);

	virtual void display(M3DMatrix44f cvmatrix, GLint model_view_loc) = 0;
	virtual void visualize(M3DMatrix44f cvmatrix) = 0;
	virtual void updatePositionVelocity(DynamicModel* base) = 0;
};

class FixedJoint : public Joint
{
protected:
	FixedJoint() {}

public:
	FixedJoint(DynamicModel* parent, DynamicModel* child, M3DVector3f pos, float r, float p, float y);

	virtual void display(M3DMatrix44f cvmatrix, GLint model_view_loc);
	virtual void visualize(M3DMatrix44f cvmatrix);
	virtual void updatePositionVelocity(DynamicModel* base);
};

class RevoluteJoint : public FixedJoint
{
private:
	float angle;
	M3DVector3f raxis;
	RevoluteJoint() :raxis{0}, angle(0) {}

public:
	RevoluteJoint(DynamicModel* parent, DynamicModel* child, M3DVector3f pos, float r, float p, float y, float ax, float ay, float az);

	void moveAngle(float target, float ratio);

	virtual void display(M3DMatrix44f cvmatrix, GLint model_view_loc);
	virtual void visualize(M3DMatrix44f cvmatrix);
	virtual void updatePositionVelocity(DynamicModel* base);
};


#endif