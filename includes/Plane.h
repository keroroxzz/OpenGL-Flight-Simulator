#pragma once

#include "ObjModel.h"
#include "PhysicalModel.h"
#include "Joints.h"
#include "FlowField.h"
#include "FluidSolver.h"
#include "GPUFluidSolver.h"
#include "Shader.h"

class F22
{
	float throttle = 0.0;
	ObjModel* body_o, *aileronL_o, *aileronR_o, *flapL_o, *flapR_o, *rudder_o, *elevatorR_o, *elevatorL_o, *glass_o;

	DynamicModel* plane, *glass;
	Aerofoil* aerofoilL, * aerofoilR, * aileronL, * aileronR, * flapL, * flapR, * rudderL, * rudderR, * elevatorR, * elevatorL, * rudderBigL, * rudderBigR;

	FixedJoint* aerofoilL_j, * aerofoilR_j, *glass_j, *rudderBigL_j, *rudderBigR_j;
	RevoluteJoint *aileronL_j, * aileronR_j, * flapL_j, * flapR_j, * rudderL_j, * rudderR_j, * elevatorR_j, * elevatorL_j, * thrust_j;

	Thruster *thruster;
    FlowField *flowField;
    FluidSolver *fluidSolver;
    GPUFluidSolver *gpuFluidSolver;
    M3DVector3f gridOrigin;

public:
	F22();

	void updatePhysic(bool windTunnel = false, M3DVector3f wind = nullptr);
    void updateLBMTest(M3DVector3f wind);
    void reinitEquilibrium(M3DVector3f wind);

	void display(M3DMatrix44f cvmatrix, GLint model_view_loc=-1);
	void visualize(M3DMatrix44f cvmatrix);
    void drawFlowField(M3DMatrix44f cvmatrix);
    void drawSolidGrid(M3DMatrix44f cvmatrix);
    void drawBoundingBox(M3DMatrix44f cvmatrix);

	void mouseControl(float x, float y);
	// Normalized control inputs in [-1, 1]: pitch (elevators), roll (ailerons), yaw (rudders).
	// Reuses the same elevator/aileron/flap/thrust-vector mapping as mouseControl and adds rudder.
	void setControl(float pitch, float roll, float yaw);
	void thrustControl(float v);

	void setPosition(M3DVector3f pos);
	void setVelocity(M3DVector3f v);
	void setDisplacement(M3DVector3f displacement);

	void getPosition(M3DVector3f pos);
	void getVelocity(M3DVector3f v);
	void getAngularVelocity(M3DVector3f w);
	void getX(M3DVector3f x);
	void getZ(M3DVector3f z);
	float getThrottle() {return throttle;}
};

