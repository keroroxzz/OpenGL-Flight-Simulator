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
    fluidSolver = new FluidSolver(32, 24, 24);
    gpuFluidSolver = new GPUFluidSolver(96, 96, 96);
    m3dCopyVector3(gridOrigin, plane->wpos);

	plane->updatePositionVelocity(plane);
}

void F22::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
	plane->display(cvmatrix, model_view_loc);
}

void F22::drawFlowField(M3DMatrix44f cvmatrix)
{
    extern M3DMatrix44f projection;
    M3DMatrix44f mvp;
    m3dMatrixMultiply44(mvp, projection, cvmatrix);
    gpuFluidSolver->drawParticles(mvp);
}

void F22::drawBoundingBox(M3DMatrix44f cvmatrix)
{
    M3DVector3f bMin, bMax;
    gpuFluidSolver->getGridBounds(bMin, bMax);
    
    glPushMatrix();
    glLoadMatrixf(cvmatrix);
    // Grid bounds are relative to gridOrigin (which is plane->wpos at the start of updatePhysic)
    glTranslatef(gridOrigin[0], gridOrigin[1], gridOrigin[2]);
    
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glBegin(GL_LINES);
    // Draw edges of the box defined by bMin and bMax
    // Bottom (z = bMin[2])
    glVertex3f(bMin[0], bMin[1], bMin[2]); glVertex3f(bMax[0], bMin[1], bMin[2]);
    glVertex3f(bMax[0], bMin[1], bMin[2]); glVertex3f(bMax[0], bMax[1], bMin[2]);
    glVertex3f(bMax[0], bMax[1], bMin[2]); glVertex3f(bMin[0], bMax[1], bMin[2]);
    glVertex3f(bMin[0], bMax[1], bMin[2]); glVertex3f(bMin[0], bMin[1], bMin[2]);
    // Top (z = bMax[2])
    glVertex3f(bMin[0], bMin[1], bMax[2]); glVertex3f(bMax[0], bMin[1], bMax[2]);
    glVertex3f(bMax[0], bMin[1], bMax[2]); glVertex3f(bMax[0], bMax[1], bMax[2]);
    glVertex3f(bMax[0], bMax[1], bMax[2]); glVertex3f(bMin[0], bMax[1], bMax[2]);
    glVertex3f(bMin[0], bMax[1], bMax[2]); glVertex3f(bMin[0], bMin[1], bMax[2]);
    // Pillars (verticals)
    glVertex3f(bMin[0], bMin[1], bMin[2]); glVertex3f(bMin[0], bMin[1], bMax[2]);
    glVertex3f(bMax[0], bMin[1], bMin[2]); glVertex3f(bMax[0], bMin[1], bMax[2]);
    glVertex3f(bMax[0], bMax[1], bMin[2]); glVertex3f(bMax[0], bMax[1], bMax[2]);
    glVertex3f(bMin[0], bMax[1], bMin[2]); glVertex3f(bMin[0], bMax[1], bMax[2]);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void F22::visualize(M3DMatrix44f cvmatrix)
{
	plane->visualize(cvmatrix);
    M3DMatrix44f invWaxis;
    m3dInvertMatrix44(invWaxis, plane->waxis);

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
	thrust_j->moveAngle(y * 30.0, 0.4);
}

void F22::reinitEquilibrium(M3DVector3f wind) {
    M3DVector3f relWind;
    m3dSubtractVectors3(relWind, wind, plane->wvel);
    gpuFluidSolver->reinitEquilibrium(relWind);
}

void F22::updatePhysic(bool windTunnel, M3DVector3f wind)
{
    extern float dt;
    extern float sim_time;
    
    // Fixed size box for LBM grid stability (32m x 16m x 16m)
    M3DVector3f bboxMin = { -20.0f, -8.0f, -8.0f };
    M3DVector3f bboxMax = { 12.0f, 8.0f, 8.0f };
    
    float groundZ = 0.0f;
    M3DVector3f worldBBoxMin, worldBBoxMax;

    // Calculate local wind in lattice units for proper grid shift initialization
    M3DVector3f relWindPhys;
    extern M3DVector3f windVelocity;
    m3dSubtractVectors3(relWindPhys, windVelocity, plane->wvel);

    M3DVector3f localWindLattice = { 0.0f, 0.0f, 0.0f };
    float dx_lbm = 32.0f / 96.0f; // matches grid size
    float u_scale = dt / dx_lbm;
    localWindLattice[0] = relWindPhys[0] * u_scale;
    localWindLattice[1] = relWindPhys[1] * u_scale;
    localWindLattice[2] = relWindPhys[2] * u_scale;
    for (int i = 0; i < 3; i++) {
        if (localWindLattice[i] > 0.15f) localWindLattice[i] = 0.15f;
        if (localWindLattice[i] < -0.15f) localWindLattice[i] = -0.15f;
    }

	for (int i = 0; i < 10; i++)
	{
        // Update world-space grid bounds based on current plane position
        m3dAddVectors3(worldBBoxMin, plane->wpos, bboxMin);
        m3dAddVectors3(worldBBoxMax, plane->wpos, bboxMax);
        gpuFluidSolver->setGridBounds(worldBBoxMin, worldBBoxMax, localWindLattice);
        m3dCopyVector3(gridOrigin, plane->wpos);

        gpuFluidSolver->clearSolid();
        if (!windTunnel && worldBBoxMin[2] <= groundZ + 0.1f) {
            gpuFluidSolver->voxelizeGround(groundZ, plane->wpos);
        }

        // Voxelize relative to current grid origin (plane wpos)
        M3DMatrix44f relTransform;
        auto voxelizeRel = [&](ObjModel* model, M3DMatrix44f waxis) {
            m3dCopyMatrix44(relTransform, waxis);
            relTransform[12] -= plane->wpos[0];
            relTransform[13] -= plane->wpos[1];
            relTransform[14] -= plane->wpos[2];
            gpuFluidSolver->voxelizePart(model, relTransform);
        };

        voxelizeRel(body_o, plane->waxis);
        voxelizeRel(aileronL_o, aileronL->waxis);
        voxelizeRel(aileronR_o, aileronR->waxis);
        voxelizeRel(flapL_o, flapL->waxis);
        voxelizeRel(flapR_o, flapR->waxis);
        voxelizeRel(rudder_o, rudderL->waxis);
        voxelizeRel(rudder_o, rudderR->waxis);
        voxelizeRel(elevatorL_o, elevatorL->waxis);
        voxelizeRel(elevatorR_o, elevatorR->waxis);

		plane->updatePositionVelocity(plane);
        M3DMatrix44f invWaxis;
        m3dInvertMatrix44(invWaxis, plane->waxis);
        
        if (!windTunnel) {
            thruster->applyEffect(plane);
            M3DVector3f gravityVec = { 0.0, 0.0, -12000.0 * 9.81 };
            plane->applyForce(gravityVec);
            M3DVector3f* cfdF = gpuFluidSolver->getCFDForce();
            M3DVector3f* cfdT = gpuFluidSolver->getCFDTorque();
            
            // NaN Safety check
            if (std::isnan((*cfdF)[0]) || std::isnan((*cfdF)[1]) || std::isnan((*cfdF)[2]) ||
                std::isnan((*cfdT)[0]) || std::isnan((*cfdT)[1]) || std::isnan((*cfdT)[2])) {
                printf("[Plane] CFD Force/Torque is NaN! Resetting solver particles...\n");
                gpuFluidSolver->resetParticles();
            } else {
                plane->applyForce(*cfdF);
                plane->applyTorque(*cfdT);
            }
            plane->updateDynamic();
        } else {
            plane->velocity[0] = plane->velocity[1] = plane->velocity[2] = 0.0f;
            plane->angular_velocity[0] = plane->angular_velocity[1] = plane->angular_velocity[2] = 0.0f;
        }

        gpuFluidSolver->step(dt, plane->wvel, plane->waxis, plane->wpos, sim_time);
        sim_time += dt;
	}
}

void F22::thrustControl(float v)
{
	float t = thruster->getMaxThrust()*v;
	throttle = thruster->setThrust(t, 0.3);
}

void F22::setPosition(M3DVector3f pos) { plane->setPostion(pos[0], pos[1], pos[2]); }
void F22::setDisplacement(M3DVector3f d) { M3DVector3f pos; getPosition(pos); m3dAddVectors3(pos, pos, d); setPosition(pos); }

void F22::updateLBMTest(M3DVector3f wind)
{
    extern float dt;
    static int fluidUpdateCounter = 0;
    static int totalIterations = 0;
    extern float sim_time;

    gpuFluidSolver->clearSolid();
    M3DVector3f bboxMin = { -15.0f, -10.0f, -10.0f };
    M3DVector3f bboxMax = { 15.0f, 10.0f, 10.0f };
    gpuFluidSolver->setGridBounds(bboxMin, bboxMax);

    // BREAK SYMMETRY - Slight noise prevents perfect numerical symmetry that can delay vortex shedding
    M3DVector3f noisyWind;
    m3dCopyVector3(noisyWind, wind);
    noisyWind[1] += sin(sim_time * 5.0f) * 0.5f;
    noisyWind[2] += cos(sim_time * 7.0f) * 0.5f;

    M3DVector3f center = { 0.0f, 0.0f, 0.0f };
    gpuFluidSolver->voxelizeCylinder(center, 1.2f, 16.0f, 2);

    M3DVector3f dummyVel = { -noisyWind[0], -noisyWind[1], -noisyWind[2] };
    M3DMatrix44f identity;
    m3dLoadIdentity44(identity);

    fluidUpdateCounter++;
    if (fluidUpdateCounter >= 2) {
        gpuFluidSolver->step(0.001f, dummyVel, identity, center, sim_time);
        fluidUpdateCounter = 0;
        totalIterations++;
        M3DVector3f* force = gpuFluidSolver->getCFDForce();
        if (std::isnan((*force)[0])) {
            printf("[LBM Test] NaN detected! Resetting...\n");
            gpuFluidSolver->resetParticles();
        }
    }
}

void F22::getPosition(M3DVector3f pos) { m3dCopyVector3(pos, plane->position); }
void F22::getVelocity(M3DVector3f v) { m3dCopyVector3(v, plane->velocity); }
void F22::getX(M3DVector3f x) { m3dCopyVector3(x, &plane->axis[0]); }
void F22::getZ(M3DVector3f z) { m3dCopyVector3(z, &plane->axis[8]); }
