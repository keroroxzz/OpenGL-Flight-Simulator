#include "GPUFluidSolver.h"
#include "PhysicalModel.h"
#include <iostream>
#include <algorithm>
#include <cmath>

GPUFluidSolver::GPUFluidSolver(int nx, int ny, int nz) : NX(nx), NY(ny), NZ(nz) {
    size = NX * NY * NZ;
    
    // Default bounds
    gridMin[0] = -22.0f; gridMin[1] = -10.0f; gridMin[2] = -5.0f;
    gridMax[0] = 8.0f; gridMax[1] = 10.0f; gridMax[2] = 5.0f;
    m3dCopyVector3(prevGridMin, gridMin);

    initBuffers();
    initShaders();
}

GPUFluidSolver::~GPUFluidSolver() {
    if (f_in_SSBO) glDeleteBuffers(1, &f_in_SSBO);
    if (f_out_SSBO) glDeleteBuffers(1, &f_out_SSBO);
    if (velocityTexture) glDeleteTextures(1, &velocityTexture);
    if (solidTexture) glDeleteTextures(1, &solidTexture);
    if (particleSSBO) glDeleteBuffers(1, &particleSSBO);
    if (forceSSBO) glDeleteBuffers(1, &forceSSBO);
    if (wakeCandidateSSBO) glDeleteBuffers(1, &wakeCandidateSSBO);
    if (wakeCounterBuffer) glDeleteBuffers(1, &wakeCounterBuffer);
    if (wakeSSBO) glDeleteBuffers(1, &wakeSSBO);
    if (errorBuffer) glDeleteBuffers(1, &errorBuffer);

    delete collisionShader;
    delete streamShader;
    delete reconstructShader;
    delete voxelizeShader;
    delete voxelizeCylinderShader;
    delete particleAdvectShader;
    delete particleRenderShader;
    delete forceComputeShader;
    delete wakeExtractShader;
    delete wakeInjectShader;
    delete stabilityShader;
    delete shiftShader;
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
        float vel_vort[4]; // Renamed for consistency with shaders
    };
    std::vector<Particle> initial_particles(numParticles);
    for (int i = 0; i < numParticles; i++) {
        initial_particles[i].pos_life[3] = 0.0f; // Life = 0 (respawn immediately)
    }
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(Particle), initial_particles.data(), GL_DYNAMIC_DRAW);

    // Force SSBO
    glGenBuffers(1, &forceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, forceSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_COPY);

    // Wake Candidates SSBO
    struct WakeCandidate {
        float pos[4];
        float dir_circ[4];
    };
    glGenBuffers(1, &wakeCandidateSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeCandidateSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(WakeCandidate) * 1024, nullptr, GL_DYNAMIC_COPY);

    glGenBuffers(1, &wakeCounterBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeCounterBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_COPY);

    // Wake SSBO
    struct GPUWake {
        float pos[4];
        float dir_circ[4];
        float params[4];
    };
    glGenBuffers(1, &wakeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUWake) * 1024, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &errorBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, errorBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

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

    forceComputeShader = new Shader(1);
    forceComputeShader->addFromFile("shaders/compute/lbm_force.comp", GL_COMPUTE_SHADER);

    wakeExtractShader = new Shader(1);
    wakeExtractShader->addFromFile("shaders/compute/wake_extract.comp", GL_COMPUTE_SHADER);

    voxelizeCylinderShader = new Shader(1);
    voxelizeCylinderShader->addFromFile("shaders/compute/voxelize_cylinder.comp", GL_COMPUTE_SHADER);

    voxelizeGroundShader = new Shader(1);
    voxelizeGroundShader->addFromFile("shaders/compute/voxelize_ground.comp", GL_COMPUTE_SHADER);

    wakeInjectShader = new Shader(1);
    wakeInjectShader->addFromFile("shaders/compute/wake_inject.comp", GL_COMPUTE_SHADER);

    stabilityShader = new Shader(1);
    stabilityShader->addFromFile("shaders/compute/stability_check.comp", GL_COMPUTE_SHADER);

    shiftShader = new Shader(1);
    shiftShader->addFromFile("shaders/compute/shift_grid.comp", GL_COMPUTE_SHADER);

    vortexPhysicsShader = new Shader(1);
    vortexPhysicsShader->addFromFile("shaders/compute/vortex_physics.comp", GL_COMPUTE_SHADER);
}

void GPUFluidSolver::setGridBounds(M3DVector3f min, M3DVector3f max, M3DVector3f localWind) {
    M3DVector3f dx = { (gridMax[0] - gridMin[0]) / (float)NX, 
                       (gridMax[1] - gridMin[1]) / (float)NY, 
                       (gridMax[2] - gridMin[2]) / (float)NZ };

    int ox = (int)round((min[0] - prevGridMin[0]) / dx[0]);
    int oy = (int)round((min[1] - prevGridMin[1]) / dx[1]);
    int oz = (int)round((min[2] - prevGridMin[2]) / dx[2]);

    if (ox != 0 || oy != 0 || oz != 0) {
        dispatchShift(ox, oy, oz, localWind);
        prevGridMin[0] += (float)ox * dx[0];
        prevGridMin[1] += (float)oy * dx[1];
        prevGridMin[2] += (float)oz * dx[2];
    }

    m3dCopyVector3(gridMin, min);
    m3dCopyVector3(gridMax, max);
}

void GPUFluidSolver::dispatchShift(int ox, int oy, int oz, M3DVector3f localWind) {
    shiftShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_out_SSBO);
    
    int offset[3] = { ox, oy, oz };
    int dims[3] = { NX, NY, NZ };
    shiftShader->setUniform("offset", UNI_INT_3, offset);
    shiftShader->setUniform("size", UNI_INT_3, dims);
    
    if (localWind) {
        shiftShader->setUniform("u_wind", UNI_VEC_3, localWind);
    } else {
        M3DVector3f zero = {0,0,0};
        shiftShader->setUniform("u_wind", UNI_VEC_3, zero);
    }

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    std::swap(f_in_SSBO, f_out_SSBO);
}

void GPUFluidSolver::step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis, M3DVector3f planePos, float simTime) {
    // Physical parameters
    float dx = 0.25f;       // 0.25m per cell
    float dt_lbm = 0.001f; // 1ms lattice time step
    
    // Scale wind velocity to lattice units (u_lbm = u_phys * dt / dx)
    float u_scale = dt_lbm / dx;
    
    // REFACTOR: Grid is axis-aligned with world, so relative wind should be computed in world coordinates
    extern M3DVector3f windVelocity;
    M3DVector3f relWindPhys;
    m3dSubtractVectors3(relWindPhys, windVelocity, planeVel);

    M3DVector3f localWindLattice = {
        relWindPhys[0] * u_scale,
        relWindPhys[1] * u_scale,
        relWindPhys[2] * u_scale
    };

    // Clamp lattice wind for stability
    for(int i=0; i<3; i++) {
        if(localWindLattice[i] > 0.15f) localWindLattice[i] = 0.15f;
        if(localWindLattice[i] < -0.15f) localWindLattice[i] = -0.15f;
    }

    // 0. Wake Simulation & Inject (Layer 3)
    vortexPhysicsShader->use();
    vortexPhysicsShader->setUniform("u_dt", UNI_FLOAT_1, &dt);
    vortexPhysicsShader->setUniform("u_globalWind", UNI_VEC_3, windVelocity);
    int maxV = 1024;
    vortexPhysicsShader->setUniform("u_maxVortices", UNI_INT_1, &maxV);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, wakeSSBO);
    glDispatchCompute(maxV / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    dispatchWakeInject(planePos);

    // 1. Collision
    dispatchCollision();
    
    // 2. Stream
    streamShader->use();
    streamShader->setUniform("u_wind", UNI_VEC_3, localWindLattice);
    dispatchStream();
    
    // 3. Reconstruct Velocity/Density
    reconstructShader->use();
    float v_inv_scale = dx / dt_lbm; 
    reconstructShader->setUniform("v_scale", UNI_FLOAT_1, &v_inv_scale);
    dispatchReconstruct();
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    // 4. Particle Advect
    dispatchParticleAdvect(dt, planePos, planeWaxis, simTime);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 5. Force Compute
    dispatchForceCompute();
    // ... rest of step ...

    // 6. Wake Extract
    dispatchWakeExtract(dt, planeWaxis, planePos);

    // 7. Stability Check
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, errorBuffer);
    uint32_t zero_err = 0;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero_err), &zero_err);
    stabilityShader->use();
    int dims[3] = { NX, NY, NZ };
    stabilityShader->setUniform("size", UNI_INT_3, dims);
    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    uint32_t has_error = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, errorBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(has_error), &has_error);
    if (has_error) {
        std::cout << "[GPUFluidSolver] Instability detected! Resetting field..." << std::endl;
        
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
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, f_in_SSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, initial_f.size() * sizeof(float), initial_f.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, f_out_SSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, initial_f.size() * sizeof(float), initial_f.data());
        
        resetParticles(); // REFACTOR: Reset particles too to prevent NaN propagation
        filteredForce[0] = filteredForce[1] = filteredForce[2] = 0.0f;
    }

    wakeManager.update(dt);
}

void GPUFluidSolver::resetParticles() {
    struct Particle {
        float pos_life[4];
        float vel_vort[4];
    };
    std::vector<Particle> initial_particles(numParticles);
    for (int i = 0; i < numParticles; i++) {
        initial_particles[i].pos_life[3] = 0.0f; // Life = 0 (respawn)
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, numParticles * sizeof(Particle), initial_particles.data());
}

void GPUFluidSolver::dispatchCollision() {
    collisionShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_out_SSBO);
    glBindImageTexture(2, velocityTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
    glBindImageTexture(3, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
    
    float tau = 0.51f; // Slightly higher for better stability
    float Smag = 0.12f; // Adjusted Smagorinsky
    collisionShader->setUniform("tau", UNI_FLOAT_1, &tau);
    collisionShader->setUniform("SmagorinskyConstant", UNI_FLOAT_1, &Smag);
    
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

extern GLfloat planePos[4];
void GPUFluidSolver::dispatchParticleAdvect(float dt, M3DVector3f planePos_arg, M3DMatrix44f planeWaxis, float simTime) {
    particleAdvectShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    
    // Bind velocity texture as sampler3D for linear filtering
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocityTexture);
    particleAdvectShader->setUniform("velocity_tex", UNI_TEXTURE, nullptr, 1, GL_FALSE, 0);

    glBindImageTexture(2, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

    M3DVector3f worldGridMin, worldGridMax;
    m3dAddVectors3(worldGridMin, planePos_arg, gridMin);
    m3dAddVectors3(worldGridMax, planePos_arg, gridMax);

    particleAdvectShader->setUniform("dt", UNI_FLOAT_1, &dt);
    particleAdvectShader->setUniform("gridMin", UNI_VEC_3, worldGridMin);
    particleAdvectShader->setUniform("gridMax", UNI_VEC_3, worldGridMax);
    
    // Automatic spawn position based on windward side
    M3DVector3f spawnPos;
    m3dCopyVector3(spawnPos, planePos_arg);
    
    spawnPos[0] += gridMin[0] + 0.5f; // Closer to the inflow wall

    particleAdvectShader->setUniform("spawnPos", UNI_VEC_3, spawnPos);
    M3DVector3f spawnRange = {0.5f, 16.0f, 16.0f}; // Wider range for better coverage
    particleAdvectShader->setUniform("spawnRange", UNI_VEC_3, spawnRange);
    
    particleAdvectShader->setUniform("iTime", UNI_FLOAT_1, &simTime);

    glDispatchCompute(numParticles / 256 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPUFluidSolver::dispatchForceCompute() {
    forceComputeShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, f_in_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_out_SSBO);
    glBindImageTexture(3, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, forceSSBO);

    // Clear force buffer (integer)
    int zero[4] = {0,0,0,0};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero), zero);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back force (integer)
    int forceData[4];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, forceSSBO);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(forceData), forceData);
    
    // Scale and Filter
    float scale_val = 1.0f / 1000.0f; // Inverse of fixed point scale
    float alpha = 0.08f;
    cfdForce[0] = forceData[0] * scale_val; 
    cfdForce[1] = forceData[1] * scale_val;
    cfdForce[2] = forceData[2] * scale_val;

    filteredForce[0] = filteredForce[0] * (1.0f - alpha) + cfdForce[0] * alpha;
    filteredForce[1] = filteredForce[1] * (1.0f - alpha) + cfdForce[1] * alpha;
    filteredForce[2] = filteredForce[2] * (1.0f - alpha) + cfdForce[2] * alpha;
}

void GPUFluidSolver::dispatchWakeExtract(float dt, M3DMatrix44f planeWaxis, M3DVector3f planePos) {
    wakeExtractShader->use();
    glBindImageTexture(1, velocityTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
    glBindImageTexture(2, solidTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, wakeCandidateSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, wakeCounterBuffer);

    uint32_t zero_cnt = 0;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero_cnt), &zero_cnt);

    M3DVector3f worldGridMin, worldGridMax;
    m3dAddVectors3(worldGridMin, planePos, gridMin);
    m3dAddVectors3(worldGridMax, planePos, gridMax);

    wakeExtractShader->setUniform("gridMin", UNI_VEC_3, worldGridMin);
    wakeExtractShader->setUniform("gridMax", UNI_VEC_3, worldGridMax);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back candidates and add to manager
    uint32_t count;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeCounterBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(count), &count);
    if (count > 1024) count = 1024;

    struct WakeCandidate {
        float pos[4];
        float dir_circ[4];
    };
    std::vector<WakeCandidate> candidates(count);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeCandidateSSBO);
    if (count > 0) {
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(WakeCandidate), candidates.data());
    }

    for (uint32_t i = 0; i < count; i++) {
        WakeVortex v;
        m3dCopyVector3(v.position, candidates[i].pos);
        m3dCopyVector3(v.direction, candidates[i].dir_circ);
        v.circulation = candidates[i].dir_circ[3];
        v.radius = 0.8f; // REFACTOR: Standard core size
        v.age = 1.0f;   // Life starts at 1.0 and goes to 0.0
        v.decayRate = 0.15f;
        v.turbulence = 0.0f;
        wakeManager.addVortex(v);
    }
}

void GPUFluidSolver::dispatchWakeInject(M3DVector3f planePos) {
    const std::list<WakeVortex>& vortices = wakeManager.getVortices();
    if (vortices.empty()) return;

    struct GPUWake {
        float pos[4];
        float dir_circ[4];
        float params[4];
    };
    std::vector<GPUWake> gpuVortices;
    for (const auto& v : vortices) {
        GPUWake gv;
        m3dCopyVector3(gv.pos, v.position); gv.pos[3] = 1.0f;
        m3dCopyVector3(gv.dir_circ, v.direction); gv.dir_circ[3] = v.circulation;
        gv.params[0] = v.radius;
        gv.params[1] = v.turbulence;
        gv.params[2] = v.age;
        gv.params[3] = v.decayRate;
        gpuVortices.push_back(gv);
        if (gpuVortices.size() >= 1024) break;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, wakeSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuVortices.size() * sizeof(GPUWake), gpuVortices.data());

    wakeInjectShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, wakeSSBO);
    glBindImageTexture(1, velocityTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    
    unsigned int count = (unsigned int)gpuVortices.size();
    wakeInjectShader->setUniform("numVortices", UNI_INT_1, &count);

    M3DVector3f worldGridMin, worldGridMax;
    m3dAddVectors3(worldGridMin, planePos, gridMin);
    m3dAddVectors3(worldGridMax, planePos, gridMax);

    wakeInjectShader->setUniform("gridMin", UNI_VEC_3, worldGridMin);
    wakeInjectShader->setUniform("gridMax", UNI_VEC_3, worldGridMax);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void GPUFluidSolver::drawParticles(M3DMatrix44f mvp) {
    if (!particleRenderShader) return;

    particleRenderShader->use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    particleRenderShader->setUniform("modelViewProj", UNI_MATRIX_4, &mvp[0]);
    float tail = 0.08f; // 80ms tail
    particleRenderShader->setUniform("tailLength", UNI_FLOAT_1, &tail);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // REFACTOR: Use additive blending for flow visualization
    glDepthMask(GL_FALSE);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, numParticles * 2);
    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void GPUFluidSolver::clearSolid() {
    GLuint zero = 0;
    glBindTexture(GL_TEXTURE_3D, solidTexture);
    glClearTexImage(solidTexture, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &zero);
}

void GPUFluidSolver::voxelizePart(ObjModel* model, M3DMatrix44f worldTransform) {
    if (!model) return;

    GLuint triSSBO = model->getTriSSBO();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triSSBO);

    voxelizeShader->use();
    glBindImageTexture(1, solidTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
    voxelizeShader->setUniform("gridMin", UNI_VEC_3, gridMin);
    voxelizeShader->setUniform("gridMax", UNI_VEC_3, gridMax);
    unsigned int nTri = (unsigned int)(model->getNumIndices() / 3);
    voxelizeShader->setUniform("numTriangles", UNI_INT_1, &nTri);
    voxelizeShader->setUniform("modelMatrix", UNI_MATRIX_4, &worldTransform[0]);

    glDispatchCompute(nTri / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void GPUFluidSolver::voxelizeAircraft(F22* aircraft) {
    // Handled in F22::updatePhysic
}

void GPUFluidSolver::voxelizeCylinder(M3DVector3f center, float radius, float height, int axis) {
    voxelizeCylinderShader->use();
    glBindImageTexture(1, solidTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
    voxelizeCylinderShader->setUniform("gridMin", UNI_VEC_3, gridMin);
    voxelizeCylinderShader->setUniform("gridMax", UNI_VEC_3, gridMax);
    voxelizeCylinderShader->setUniform("center", UNI_VEC_3, center);
    voxelizeCylinderShader->setUniform("radius", UNI_FLOAT_1, &radius);
    voxelizeCylinderShader->setUniform("height", UNI_FLOAT_1, &height);
    voxelizeCylinderShader->setUniform("axis", UNI_INT_1, &axis);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void GPUFluidSolver::voxelizeGround(float groundZ) {
    voxelizeGroundShader->use();
    glBindImageTexture(1, solidTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
    voxelizeGroundShader->setUniform("gridMin", UNI_VEC_3, gridMin);
    voxelizeGroundShader->setUniform("gridMax", UNI_VEC_3, gridMax);
    voxelizeGroundShader->setUniform("groundZ", UNI_FLOAT_1, &groundZ);

    glDispatchCompute(NX / 8, NY / 8, NZ / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
