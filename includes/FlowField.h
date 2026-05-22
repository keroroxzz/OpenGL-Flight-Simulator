#pragma once
#include "FluidSolver.h"
#include <vector>

struct AirParticle {
    M3DVector3f pos;
    M3DVector3f vel;
    float life;
    float maxLife;
};

class FlowField {
    std::vector<AirParticle> particles;
    int maxParticles;
    float spawnTimer;

public:
    FlowField(int maxP = 5000) : maxParticles(maxP), spawnTimer(0) {
        particles.reserve(maxParticles);
    }

    void update(float dt, M3DVector3f planePos, M3DVector3f planeVel, M3DMatrix44f planeWaxis, M3DMatrix44f invWaxis, FluidSolver* solver) {
        M3DVector3f forward = {planeWaxis[0], planeWaxis[1], planeWaxis[2]};
        M3DVector3f side = {planeWaxis[4], planeWaxis[5], planeWaxis[6]};
        M3DVector3f up = {planeWaxis[8], planeWaxis[9], planeWaxis[10]};
        
        float speed = m3dGetMagnitude(planeVel);

        // Move particles
        for (auto it = particles.begin(); it != particles.end(); ) {
            // Check for collision with solid geometry
            if (solver->isSolid(it->pos, planePos, invWaxis)) {
                // Particle is inside! Kill it or deflect. 
                // For now, let's just kill it to show the shadow.
                it = particles.erase(it);
                continue;
            }

            // Sample velocity from fluid grid
            M3DVector3f worldFluid;
            solver->getVelocity(worldFluid, it->pos, planePos, planeWaxis, invWaxis);
            
            m3dCopyVector3(it->vel, worldFluid);

            it->pos[0] += (worldFluid[0]) * dt;
            it->pos[1] += (worldFluid[1]) * dt;
            it->pos[2] += (worldFluid[2]) * dt;
            it->life -= dt;

            if (it->life <= 0) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }

        // Spawn new particles (high frequency)
        spawnTimer += dt;
        if (spawnTimer > 0.001f && particles.size() < maxParticles) {
            spawnTimer = 0;
            for(int i=0; i<5; ++i) {
                AirParticle p;
                // Spawn near the front of the grid
                float offX = 8.0f; 
                float offY = ((rand() % 400) / 100.0f - 2.0f) * 8.0f; 
                float offZ = ((rand() % 200) / 100.0f - 1.0f) * 4.0f;
                
                p.pos[0] = planePos[0] + forward[0] * offX + side[0] * offY + up[0] * offZ;
                p.pos[1] = planePos[1] + forward[1] * offX + side[1] * offY + up[1] * offZ;
                p.pos[2] = planePos[2] + forward[2] * offX + side[2] * offY + up[2] * offZ;

                p.vel[0] = 0; p.vel[1] = 0; p.vel[2] = 0; 
                p.maxLife = 1.2f;
                p.life = p.maxLife;
                particles.push_back(p);
            }
        }
    }

    void draw(M3DMatrix44f mv, M3DVector3f planeVel) {
        glPushMatrix();
        glLoadMatrixf(mv);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glBegin(GL_LINES);
        for (const auto& p : particles) {
            float alpha = p.life / p.maxLife;
            
            float vMag = m3dGetMagnitude(p.vel);
            float pVelMag = m3dGetMagnitude(planeVel);
            float t = vMag / (pVelMag + 5.0f);
            if (t > 1.0f) t = 1.0f;

            if (t < 0.25f) glColor4f(0, 4*t, 1, alpha * 0.6f);
            else if (t < 0.5f) glColor4f(0, 1, 1 - 4*(t-0.25f), alpha * 0.6f);
            else if (t < 0.75f) glColor4f(4*(t-0.5f), 1, 0, alpha * 0.6f);
            else glColor4f(1, 1 - 4*(t-0.75f), 0, alpha * 0.6f);

            glVertex3fv(p.pos);
            
            M3DVector3f tailEnd;
            tailEnd[0] = p.pos[0] - p.vel[0] * 0.05f;
            tailEnd[1] = p.pos[1] - p.vel[1] * 0.05f;
            tailEnd[2] = p.pos[2] - p.vel[2] * 0.05f;
            glVertex3fv(tailEnd);
        }
        glEnd();
        glDisable(GL_BLEND);
        glPopMatrix();
    }
};
