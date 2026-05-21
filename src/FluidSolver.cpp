#include "FluidSolver.h"
#include <algorithm>

FluidSolver::FluidSolver(int nx, int ny, int nz) : NX(nx), NY(ny), NZ(nz) {
    size = NX * NY * NZ;
    u.resize(size, 0); v.resize(size, 0); w.resize(size, 0);
    u_prev.resize(size, 0); v_prev.resize(size, 0); w_prev.resize(size, 0);
    p.resize(size, 0); div.resize(size, 0);
}

void FluidSolver::step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis) {
    printf("[FluidSolver] Step START dt=%f\n", dt);
    
    // 1. Advection
    printf("[FluidSolver] Advecting U...\n");
    advect(1, u, u_prev, u_prev, v_prev, w_prev, dt);
    printf("[FluidSolver] Advecting V...\n");
    advect(2, v, v_prev, u_prev, v_prev, w_prev, dt);
    printf("[FluidSolver] Advecting W...\n");
    advect(3, w, w_prev, u_prev, v_prev, w_prev, dt);
    
    // 2. Projection (Incompressibility)
    printf("[FluidSolver] Projecting...\n");
    project(u, v, w, p, div);
    
    // 3. Prepare for next frame
    u_prev = u; v_prev = v; w_prev = w;
    printf("[FluidSolver] Step COMPLETE.\n");
}

void FluidSolver::advect(int b, std::vector<float>& d, const std::vector<float>& d0, const std::vector<float>& du, const std::vector<float>& dv, const std::vector<float>& dw, float dt) {
    float dt0 = dt * std::max(NX, std::max(NY, NZ));
    int nanCount = 0;

    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int i = 1; i < NX - 1; i++) {
                int idx = IX(i, j, k);
                float vx = du[idx];
                float vy = dv[idx];
                float vz = dw[idx];

                if (std::isnan(vx) || std::isnan(vy) || std::isnan(vz)) {
                    nanCount++;
                    d[idx] = 0;
                    continue;
                }

                float x = i - dt0 * vx;
                float y = j - dt0 * vy;
                float z = k - dt0 * vz;
                
                // Strict clamping
                if (x < 0.5f) x = 0.5f; if (x > NX - 1.5f) x = NX - 1.5f;
                if (y < 0.5f) y = 0.5f; if (y > NY - 1.5f) y = NY - 1.5f;
                if (z < 0.5f) z = 0.5f; if (z > NZ - 1.5f) z = NZ - 1.5f;
                
                int i0 = (int)x, i1 = i0 + 1;
                int j0 = (int)y, j1 = j0 + 1;
                int k0 = (int)z, k1 = k0 + 1;
                
                // Final safety check
                if (i1 >= NX || j1 >= NY || k1 >= NZ || i0 < 0 || j0 < 0 || k0 < 0) {
                    d[idx] = 0;
                    continue;
                }

                float s1 = x - i0, s0 = 1 - s1;
                float t1 = y - j0, t0 = 1 - t1;
                float u1 = z - k0, u0 = 1 - u1;
                
                d[idx] = 
                    u0 * (t0 * (s0 * d0[IX(i0, j0, k0)] + s1 * d0[IX(i1, j0, k0)]) +
                          t1 * (s0 * d0[IX(i0, j1, k0)] + s1 * d0[IX(i1, j1, k0)])) +
                    u1 * (t0 * (s0 * d0[IX(i0, j0, k1)] + s1 * d0[IX(i1, j0, k1)]) +
                          t1 * (s0 * d0[IX(i0, j1, k1)] + s1 * d0[IX(i1, j1, k1)]));
            }
        }
    }
    if (nanCount > 0) printf("[FluidSolver] Warning: %d NaNs found in velocity field\n", nanCount);
    set_bnd(b, d);
}

void FluidSolver::project(std::vector<float>& du, std::vector<float>& dv, std::vector<float>& dw, std::vector<float>& dp, std::vector<float>& ddiv) {
    float h = 1.0f / NX;
    // printf("[FluidSolver] Calculating divergence...\n");
    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int i = 1; i < NX - 1; i++) {
                ddiv[IX(i, j, k)] = -0.5f * h * (du[IX(i + 1, j, k)] - du[IX(i - 1, j, k)] +
                                               dv[IX(i, j + 1, k)] - dv[IX(i, j - 1, k)] +
                                               dw[IX(i, j, k + 1)] - dw[IX(i, j, k - 1)]);
                dp[IX(i, j, k)] = 0;
            }
        }
    }
    set_bnd(0, ddiv); set_bnd(0, dp);
    
    // printf("[FluidSolver] Solving Poisson equation...\n");
    lin_solve(0, dp, ddiv, 1, 6);
    
    // printf("[FluidSolver] Updating velocities...\n");
    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int i = 1; i < NX - 1; i++) {
                du[IX(i, j, k)] -= 0.5f * (dp[IX(i + 1, j, k)] - dp[IX(i - 1, j, k)]) / h;
                dv[IX(i, j, k)] -= 0.5f * (dp[IX(i, j + 1, k)] - dp[IX(i, j - 1, k)]) / h;
                dw[IX(i, j, k)] -= 0.5f * (dp[IX(i, j, k + 1)] - dp[IX(i, j, k - 1)]) / h;
            }
        }
    }
    set_bnd(1, du); set_bnd(2, dv); set_bnd(3, dw);
}

void FluidSolver::lin_solve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c) {
    for (int n = 0; n < 4; n++) { // Reduced iterations for real-time
        for (int k = 1; k < NZ - 1; k++) {
            for (int j = 1; j < NY - 1; j++) {
                for (int i = 1; i < NX - 1; i++) {
                    x[IX(i, j, k)] = (x0[IX(i, j, k)] + a * (x[IX(i + 1, j, k)] + x[IX(i - 1, j, k)] +
                                                           x[IX(i, j + 1, k)] + x[IX(i, j - 1, k)] +
                                                           x[IX(i, j, k + 1)] + x[IX(i, j, k - 1)])) / c;
                }
            }
        }
        set_bnd(b, x);
    }
}

void FluidSolver::set_bnd(int b, std::vector<float>& x) {
    for (int j = 1; j < NY - 1; j++) {
        for (int i = 1; i < NX - 1; i++) {
            x[IX(i, j, 0)] = b == 3 ? -x[IX(i, j, 1)] : x[IX(i, j, 1)];
            x[IX(i, j, NZ - 1)] = b == 3 ? -x[IX(i, j, NZ - 2)] : x[IX(i, j, NZ - 2)];
        }
    }
    for (int k = 1; k < NZ - 1; k++) {
        for (int i = 1; i < NX - 1; i++) {
            x[IX(i, 0, k)] = b == 2 ? -x[IX(i, 1, k)] : x[IX(i, 1, k)];
            x[IX(i, NY - 1, k)] = b == 2 ? -x[IX(i, NY - 2, k)] : x[IX(i, NY - 2, k)];
        }
    }
    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            x[IX(0, j, k)] = b == 1 ? -x[IX(1, j, k)] : x[IX(1, j, k)];
            x[IX(NX - 1, j, k)] = b == 1 ? -x[IX(NX - 2, j, k)] : x[IX(NX - 2, j, k)];
        }
    }

    // Edge cases (corners)
    x[IX(0, 0, 0)] = 0.333f * (x[IX(1, 0, 0)] + x[IX(0, 1, 0)] + x[IX(0, 0, 1)]);
    x[IX(NX-1, NY-1, NZ-1)] = 0.333f * (x[IX(NX-2, NY-1, NZ-1)] + x[IX(NX-1, NY-2, NZ-1)] + x[IX(NX-1, NY-1, NZ-2)]);
    // ... Simplified corners, but enough to prevent random memory reads
}

void FluidSolver::addSource(M3DVector3f pos, M3DVector3f force, float dt, M3DVector3f gridOrigin, M3DMatrix44f invWaxis) {
    // Transform world position to grid index
    M3DVector3f localPos;
    m3dSubtractVectors3(localPos, pos, gridOrigin);
    // Grid spans approx -10 to 20 in X and -15 to 15 in Y/Z
    int i = (int)((localPos[0] + 10.0f) * (NX / 30.0f));
    int j = (int)((localPos[1] + 15.0f) * (NY / 30.0f));
    int k = (int)((localPos[2] + 15.0f) * (NZ / 30.0f));
    
    if (i >= 1 && i < NX-1 && j >= 1 && j < NY-1 && k >= 1 && k < NZ-1) {
        int idx = IX(i, j, k);
        u_prev[idx] += force[0] * dt;
        v_prev[idx] += force[1] * dt;
        w_prev[idx] += force[2] * dt;
    }
}

void FluidSolver::getVelocity(M3DVector3f outVel, M3DVector3f worldPos, M3DVector3f gridOrigin, M3DMatrix44f invWaxis) {
    outVel[0] = outVel[1] = outVel[2] = 0.0f;
    M3DVector3f localPos;
    m3dSubtractVectors3(localPos, worldPos, gridOrigin);
    
    // Scale local position to grid coordinates
    float fi = (localPos[0] + 10.0f) * (NX / 30.0f);
    float fj = (localPos[1] + 15.0f) * (NY / 30.0f);
    float fk = (localPos[2] + 15.0f) * (NZ / 30.0f);

    int i = (int)fi;
    int j = (int)fj;
    int k = (int)fk;
    
    if (i >= 0 && i < NX && j >= 0 && j < NY && k >= 0 && k < NZ) {
        int idx = IX(i, j, k);
        outVel[0] = u[idx];
        outVel[1] = v[idx];
        outVel[2] = w[idx];
    }
}
