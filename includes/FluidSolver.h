#pragma once
#include "baseHeader.h"
#include <vector>

class FluidSolver {
    int NX, NY, NZ;
    int size;
    
    std::vector<float> u, v, w;         // Current velocity
    std::vector<float> u_prev, v_prev, w_prev; // Previous velocity
    std::vector<float> p, div;          // Pressure and divergence
    
    inline int IX(int i, int j, int k) { 
        return i + (j * NX) + (k * NX * NY); 
    }

    GLuint textureID = 0;
    std::vector<float> textureData;
    std::vector<int> solidMask;
    M3DVector3f gridMin, gridMax;

    public:
    FluidSolver(int nx = 32, int ny = 24, int nz = 24);
    ~FluidSolver();

    void setGridBounds(M3DVector3f min, M3DVector3f max);
    void getGridBounds(M3DVector3f minOut, M3DVector3f maxOut) {
        m3dCopyVector3(minOut, gridMin);
        m3dCopyVector3(maxOut, gridMax);
    }
    void step(float dt, M3DVector3f planeVel, M3DMatrix44f planeWaxis);
    void addSource(M3DVector3f worldPos, M3DVector3f force, float dt, M3DVector3f planePos, M3DMatrix44f invWaxis);
    void setSolid(M3DVector3f worldPos, M3DVector3f planePos, M3DMatrix44f invWaxis);
    void setSolidLocal(M3DVector3f localPos);
    void clearSolid();

    void getVelocity(M3DVector3f outVel, M3DVector3f worldPos, M3DVector3f planePos, M3DMatrix44f waxis, M3DMatrix44f invWaxis);
    bool isSolid(M3DVector3f worldPos, M3DVector3f planePos, M3DMatrix44f invWaxis);
    bool isSolidLocal(M3DVector3f localPos);    GLuint get3DTexture(); // Updates and returns the texture ID

    private:
    void advect(int b, std::vector<float>& d, const std::vector<float>& d0, const std::vector<float>& du, const std::vector<float>& dv, const std::vector<float>& dw, float dt);
    void project(std::vector<float>& du, std::vector<float>& dv, std::vector<float>& dw, std::vector<float>& dp, std::vector<float>& ddiv);
    void set_bnd(int b, std::vector<float>& x);
    void lin_solve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c);
};
