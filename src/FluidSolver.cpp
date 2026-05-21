#include "FluidSolver.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

FluidSolver::FluidSolver(int nx, int ny, int nz) : NX(nx), NY(ny), NZ(nz) {
    size = NX * NY * NZ;
    u.resize(size, 0); v.resize(size, 0); w.resize(size, 0);
    u_prev.resize(size, 0); v_prev.resize(size, 0); w_prev.resize(size, 0);
    p.resize(size, 0); div.resize(size, 0);
}

void FluidSolver::step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis) {
    float speed = m3dGetMagnitude(planeVel);
    
    // Constantly inject background wind at the front (high X)
    for (int k = 0; k < NZ; k++) {
        for (int j = 0; j < NY; j++) {
            int idx = IX(NX-1, j, k); 
            u_prev[idx] = -speed; 
            v_prev[idx] = 0;
            w_prev[idx] = 0;
        }
    }

    advect(1, u, u_prev, u_prev, v_prev, w_prev, dt);
    advect(2, v, v_prev, u_prev, v_prev, w_prev, dt);
    advect(3, w, w_prev, u_prev, v_prev, w_prev, dt);
    
    project(u, v, w, p, div);
    
    u_prev = u; v_prev = v; w_prev = w;
}

void FluidSolver::advect(int b, std::vector<float>& d, const std::vector<float>& d0, const std::vector<float>& du, const std::vector<float>& dv, const std::vector<float>& dw, float dt) {
    float dt0 = dt * std::max(NX, std::max(NY, NZ)) * 0.1f;
    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int i = 1; i < NX - 1; i++) {
                int idx = IX(i, j, k);
                float vx = du[idx];
                float vy = dv[idx];
                float vz = dw[idx];

                if (std::isnan(vx) || std::isnan(vy) || std::isnan(vz)) {
                    d[idx] = 0;
                    continue;
                }

                float x = i - dt0 * vx;
                float y = j - dt0 * vy;
                float z = k - dt0 * vz;
                
                if (x < 0.5f) x = 0.5f; if (x > NX - 1.5f) x = NX - 1.5f;
                if (y < 0.5f) y = 0.5f; if (y > NY - 1.5f) y = NY - 1.5f;
                if (z < 0.5f) z = 0.5f; if (z > NZ - 1.5f) z = NZ - 1.5f;
                
                int i0 = (int)x, i1 = i0 + 1;
                int j0 = (int)y, j1 = j0 + 1;
                int k0 = (int)z, k1 = k0 + 1;
                
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
    set_bnd(b, d);
}

void FluidSolver::project(std::vector<float>& du, std::vector<float>& dv, std::vector<float>& dw, std::vector<float>& dp, std::vector<float>& ddiv) {
    float h = 1.0f / NX;
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
    lin_solve(0, dp, ddiv, 1, 6);
    
    for (int k = 1; k < NZ - 1; k++) {
        for (int j = 1; j < NY - 1; j++) {
            for (int i = 1; i < NX - 1; i++) {
                du[IX(i, j, k)] -= 0.5f * (dp[IX(i + 1, j, k)] - dp[IX(i - 1, j, k)]) / h;
                dv[IX(i, j + 1, k)] -= 0.5f * (dp[IX(i, j + 1, k)] - dp[IX(i, j - 1, k)]) / h;
                dw[IX(i, j, k + 1)] -= 0.5f * (dp[IX(i, j, k + 1)] - dp[IX(i, j, k - 1)]) / h;
            }
        }
    }
    set_bnd(1, du); set_bnd(2, dv); set_bnd(3, dw);
}

void FluidSolver::lin_solve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c) {
    for (int n = 0; n < 4; n++) {
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
}

void FluidSolver::addSource(M3DVector3f worldPos, M3DVector3f force, float dt, M3DVector3f gridOrigin, M3DMatrix44f invWaxis) {
    M3DVector3f localPos;
    m3dSubtractVectors3(localPos, worldPos, gridOrigin);
    M3DVector3f localForce;
    if (invWaxis) {
        m3dRotateVector(localForce, force, invWaxis);
    } else {
        m3dCopyVector3(localForce, force);
    }
    int i = (int)((localPos[0] + 10.0f) * (NX / 30.0f));
    int j = (int)((localPos[1] + 15.0f) * (NY / 30.0f));
    int k = (int)((localPos[2] + 15.0f) * (NZ / 30.0f));
    if (i >= 1 && i < NX-1 && j >= 1 && j < NY-1 && k >= 1 && k < NZ-1) {
        int idx = IX(i, j, k);
        u_prev[idx] -= localForce[0] * dt * 0.1f;
        v_prev[idx] -= localForce[1] * dt * 0.1f;
        w_prev[idx] -= localForce[2] * dt * 0.1f;
    }
}

void FluidSolver::getVelocity(M3DVector3f outVel, M3DVector3f worldPos, M3DVector3f gridOrigin, M3DMatrix44f waxis) {
    outVel[0] = outVel[1] = outVel[2] = 0.0f;
    M3DVector3f localPos;
    m3dSubtractVectors3(localPos, worldPos, gridOrigin);
    int i = (int)((localPos[0] + 10.0f) * (NX / 30.0f));
    int j = (int)((localPos[1] + 15.0f) * (NY / 30.0f));
    int k = (int)((localPos[2] + 15.0f) * (NZ / 30.0f));
    if (i >= 0 && i < NX && j >= 0 && j < NY && k >= 0 && k < NZ) {
        int idx = IX(i, j, k);
        M3DVector3f localVel = { u[idx], v[idx], w[idx] };
        if (waxis) {
            outVel[0] = waxis[0]*localVel[0] + waxis[4]*localVel[1] + waxis[8]*localVel[2];
            outVel[1] = waxis[1]*localVel[0] + waxis[5]*localVel[1] + waxis[9]*localVel[2];
            outVel[2] = waxis[2]*localVel[0] + waxis[6]*localVel[1] + waxis[10]*localVel[2];
        } else {
            m3dCopyVector3(outVel, localVel);
        }
    }
}
