#pragma once
#include "baseHeader.h"
#include "Shader.h"
#include "WakeSystem.h"
#include <vector>

class GPUFluidSolver {
    int NX, NY, NZ;
    int size;

    GLuint f_in_SSBO = 0;
    GLuint f_out_SSBO = 0;
    GLuint velocityTexture = 0;
    GLuint solidTexture = 0;

    Shader* collisionShader = nullptr;
    Shader* streamShader = nullptr;
    Shader* reconstructShader = nullptr;
    Shader* voxelizeShader = nullptr;
    Shader* particleAdvectShader = nullptr;
    Shader* particleRenderShader = nullptr;
    Shader* forceComputeShader = nullptr;
    Shader* wakeExtractShader = nullptr;

    GLuint particleSSBO = 0;
    int numParticles = 100000;

    GLuint forceSSBO = 0;
    GLuint wakeCandidateSSBO = 0;
    GLuint wakeCounterBuffer = 0;

    WakeManager wakeManager;

    M3DVector3f gridMin, gridMax;
    M3DVector3f cfdForce = {0,0,0};
    M3DVector3f filteredForce = {0,0,0};

public:
    GPUFluidSolver(int nx = 96, int ny = 96, int nz = 96);
    ~GPUFluidSolver();

    void setGridBounds(M3DVector3f min, M3DVector3f max);
    void getGridBounds(M3DVector3f minOut, M3DVector3f maxOut) {
        m3dCopyVector3(minOut, gridMin);
        m3dCopyVector3(maxOut, gridMax);
    }

    void step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis, M3DVector3f planePos, float simTime);
    
    void clearSolid();
    void voxelizePart(class ObjModel* model, M3DMatrix44f worldTransform);
    void voxelizeAircraft(class F22* aircraft);

    void drawParticles(M3DMatrix44f mvp);

    GLuint getVelocityTexture() { return velocityTexture; }
    M3DVector3f* getCFDForce() { return &filteredForce; }

    void dispatchCollision();
    void dispatchStream();
    void dispatchReconstruct();
    void dispatchParticleAdvect(float dt, M3DVector3f planePos, M3DMatrix44f planeWaxis, float simTime);
    void dispatchForceCompute();
    void dispatchWakeExtract(float dt, M3DMatrix44f planeWaxis);

private:
    void initBuffers();
    void initShaders();
};
