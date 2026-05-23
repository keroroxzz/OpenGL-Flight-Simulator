#include "GPUFluidSolver.h"
#include "PhysicalModel.h"
#include <iostream>
#include <algorithm>

GPUFluidSolver::GPUFluidSolver(int nx, int ny, int nz) : NX(nx), NY(ny), NZ(nz) {
    size = NX * NY * NZ;
    
    // Default bounds
    gridMin[0] = -22.0f; gridMin[1] = -10.0f; gridMin[2] = -5.0f;
    gridMax[0] = 8.0f; gridMax[1] = 10.0f; gridMax[2] = 5.0f;

    initBuffers();
    initShaders();
}

GPUFluidSolver::~GPUFluidSolver() {
    if (f_in_SSBO) glDeleteBuffers(1, &f_in_SSBO);
    if (f_out_SSBO) glDeleteBuffers(1, &f_out_SSBO);
    if (velocityTexture) glDeleteTextures(1, &velocityTexture);
    if (solidTexture) glDeleteTextures(1, &solidTexture);
    if (particleSSBO) glDeleteBuffers(1, &particleSSBO);

    delete collisionShader;
    delete streamShader;
    delete reconstructShader;
    delete voxelizeShader;
    delete particleAdvectShader;
    delete particleRenderShader;
}

void GPUFluidSolver::initBuffers() {
    // Allocation for 19 distribution functions per cell
    std::vector<float> initial_f(size * 19, 0.0f);
    
    float w[19] = {
        1.0f/3.0f,
        1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
        1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
        1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
        1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
    };

    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < size; j++) {
            initial_f[i * size + j] = w[i];
        }
    }

    glGenBuffers(1, &f_in_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, f_in_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_f.size() * sizeof(float), initial_f.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &f_out_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, f_out_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_f.size() * sizeof(float), initial_f.data(), GL_DYNAMIC_DRAW);

    // Velocity Texture (RGBA16F)
    glGenTextures(1, &velocityTexture);
    glBindTexture(GL_TEXTURE_3D, velocityTexture);
    std::vector<float> initial_vel(size * 4, 0.0f);
    for (int i = 0; i < size; i++) {
        initial_vel[i * 4 + 3] = 1.0f; // Rho = 1.0
    }
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, NX, NY, NZ, 0, GL_RGBA, GL_FLOAT, initial_vel.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Solid Texture (R8UI)
    glGenTextures(1, &solidTexture);
    glBindTexture(GL_TEXTURE_3D, solidTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8UI, NX, NY, NZ, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Particle SSBO
    struct Particle {
        float pos_life[4];
        float vel_maxLife[4];
    };
    std::vector<Particle> initial_particles(numParticles);
    for (int i = 0; i < numParticles; i++) {
        initial_particles[i].pos_life[3] = 0.0f; // Life = 0 (respawn immediately)
    }
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(Particle), initial_particles.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void GPUFluidSolver::initShaders() {
    collisionShader = new Shader(1);
    collisionShader->addFromFile("shaders/compute/lbm_collision.comp", GL_COMPUTE_SHADER);

    streamShader = new Shader(1);
    streamShader->addFromFile("shaders/compute/lbm_stream.comp", GL_COMPUTE_SHADER);

    reconstructShader = new Shader(1);
    reconstructShader->addFromFile("shaders/compute/velocity_reconstruct.comp", GL_COMPUTE_SHADER);
    
    particleAdvectShader = new Shader(1);
    particleAdvectShader->addFromFile("shaders/compute/particle_advect.comp", GL_COMPUTE_SHADER);

    particleRenderShader = new Shader(2);
    particleRenderShader->addFromFile("shaders/vertex/particle.vs", GL_VERTEX_SHADER);
    particleRenderShader->addFromFile("shaders/frag/particle.fs", GL_FRAGMENT_SHADER);

    voxelizeShader = new Shader(1);
    voxelizeShader->addFromFile("shaders/compute/voxelize.comp", GL_COMPUTE_SHADER);
}

void GPUFluidSolver::setGridBounds(M3DVector3f min, M3DVector3f max) {
    m3dCopyVector3(gridMin, min);
    m3dCopyVector3(gridMax, max);
}

void GPUFluidSolver::step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis, M3DVector3f planePos, float simTime) {
    float u_wind = -m3dDotProduct(planeVel, &planeWaxis[0]); 
    float v_wind = -m3dDotProduct(planeVel, &planeWaxis[4]);
    float w_wind = -m3dDotProduct(planeVel, &planeWaxis[8]);
    M3DVector3f localWind = {u_wind, v_wind, w_wind};

    // 1. Collision
    dispatchCollision();
    
    // 2. Stream
    streamShader->use();
    streamShader->setUniform("u_wind", UNI_VEC_3, localWind);
    dispatchStream();
    
    // 3. Reconstruct Velocity/Density
    dispatchReconstruct();
    
    // 4. Particle Advect
    dispatchParticleAdvect(dt, planePos, planeWaxis, simTime);

    // Swap SSBOs
    std::swap(f_in_SSBO, f_out_SSBO);
}

void GPUFluidSolver::dispatchCollision() {
    collisionShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_out_SSBO);
    glBindImageTexture(2, velocityTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
    glBindImageTexture(3, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
    
    float tau = 0.55f;
    collisionShader->setUniform("tau", UNI_FLOAT_1, &tau);
    
    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUFluidSolver::dispatchStream() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_out_SSBO);
    glBindImageTexture(3, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUFluidSolver::dispatchReconstruct() {
    reconstructShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindImageTexture(1, velocityTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

void GPUFluidSolver::dispatchParticleAdvect(float dt, M3DVector3f planePos, M3DMatrix44f planeWaxis, float simTime) {
    particleAdvectShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    glBindImageTexture(1, velocityTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
    glBindImageTexture(2, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

    particleAdvectShader->setUniform("dt", UNI_FLOAT_1, &dt);
    particleAdvectShader->setUniform("gridMin", UNI_VEC_3, gridMin);
    particleAdvectShader->setUniform("gridMax", UNI_VEC_3, gridMax);
    
    M3DVector3f spawnPos;
    M3DVector3f forward = {planeWaxis[0], planeWaxis[1], planeWaxis[2]};
    m3dScaleVector3(forward, 15.0f);
    m3dAddVectors3(spawnPos, planePos, forward);

    particleAdvectShader->setUniform("spawnPos", UNI_VEC_3, spawnPos);
    M3DVector3f spawnRange = {2.0f, 15.0f, 8.0f};
    particleAdvectShader->setUniform("spawnRange", UNI_VEC_3, spawnRange);
    
    particleAdvectShader->setUniform("iTime", UNI_FLOAT_1, &simTime);

    glDispatchCompute(numParticles / 256 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUFluidSolver::drawParticles(M3DMatrix44f mvp) {
    particleRenderShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    particleRenderShader->setUniform("modelViewProj", UNI_MATRIX_4, &mvp[0]);
    float tail = 0.05f;
    particleRenderShader->setUniform("tailLength", UNI_FLOAT_1, &tail);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_LINES, 0, numParticles * 2);
    glDisable(GL_BLEND);
}

void GPUFluidSolver::clearSolid() {
    GLuint zero = 0;
    glBindTexture(GL_TEXTURE_3D, solidTexture);
    glClearTexImage(solidTexture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
}

void GPUFluidSolver::voxelizePart(ObjModel* model, M3DMatrix44f worldTransform) {
    if (!model) return;

    int nInd = model->getNumIndices();
    GLushort* pInd = model->getIndices();
    GLfloat* pVerts = model->getVertices();

    struct GPUTriangle {
        float p0[4], p1[4], p2[4];
    };
    std::vector<GPUTriangle> triangles(nInd / 3);
    for (int i = 0; i < nInd / 3; i++) {
        int i0 = pInd[i * 3 + 0];
        int i1 = pInd[i * 3 + 1];
        int i2 = pInd[i * 3 + 2];
        
        M3DVector3f v0 = { pVerts[i0 * 3 + 0], pVerts[i0 * 3 + 1], pVerts[i0 * 3 + 2] };
        M3DVector3f v1 = { pVerts[i1 * 3 + 0], pVerts[i1 * 3 + 1], pVerts[i1 * 3 + 2] };
        M3DVector3f v2 = { pVerts[i2 * 3 + 0], pVerts[i2 * 3 + 1], pVerts[i2 * 3 + 2] };

        M3DVector3f tv0, tv1, tv2;
        m3dTransformVector3(tv0, v0, worldTransform);
        m3dTransformVector3(tv1, v1, worldTransform);
        m3dTransformVector3(tv2, v2, worldTransform);

        for(int j=0; j<3; ++j) {
            triangles[i].p0[j] = tv0[j];
            triangles[i].p1[j] = tv1[j];
            triangles[i].p2[j] = tv2[j];
        }
        triangles[i].p0[3] = triangles[i].p1[3] = triangles[i].p2[3] = 1.0f;
    }

    GLuint triSSBO;
    glGenBuffers(1, &triSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(GPUTriangle), triangles.data(), GL_STREAM_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triSSBO);

    voxelizeShader->use();
    glBindImageTexture(1, solidTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
    voxelizeShader->setUniform("gridMin", UNI_VEC_3, gridMin);
    voxelizeShader->setUniform("gridMax", UNI_VEC_3, gridMax);
    unsigned int nTri = (unsigned int)triangles.size();
    voxelizeShader->setUniform("numTriangles", UNI_INT_1, &nTri);

    glDispatchCompute(nTri / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glDeleteBuffers(1, &triSSBO);
}

void GPUFluidSolver::voxelizeAircraft(F22* aircraft) {
    // Requires access to F22 internals, better done in F22::updatePhysic
}
