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

    float sliceRoll = 0.0f, slicePitch = 90.0f, sliceDist = 5.0f;

public:
	F22();

	void updatePhysic(bool windTunnel = false, M3DVector3f wind = nullptr);

	void display(M3DMatrix44f cvmatrix, GLint model_view_loc=-1);
	void visualize(M3DMatrix44f cvmatrix);
    void drawFlowField(M3DMatrix44f cvmatrix);
    void drawSlice(M3DMatrix44f cvmatrix, Shader* sliceShader);

    void rotateSlice(float roll, float pitch);
    void moveSlice(float forward);

	void mouseControl(float x, float y);
	void thrustControl(float v);

	void setPosition(M3DVector3f pos);
	void setDisplacement(M3DVector3f displacement);

	void getPosition(M3DVector3f pos);
	void getVelocity(M3DVector3f v);
	void getX(M3DVector3f x);
	void getZ(M3DVector3f z);
	float getThrottle() {return throttle;}
};

