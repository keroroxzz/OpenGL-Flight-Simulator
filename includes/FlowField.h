#pragma once
#include "baseHeader.h"
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
    FlowField(int maxP = 2000) : maxParticles(maxP), spawnTimer(0) {
        particles.reserve(maxParticles);
    }

    void update(float dt, M3DVector3f planePos, M3DVector3f planeVel, M3DMatrix44f planeWaxis) {
        M3DVector3f forward = {planeWaxis[0], planeWaxis[1], planeWaxis[2]};
        M3DVector3f side = {planeWaxis[4], planeWaxis[5], planeWaxis[6]};
        M3DVector3f up = {planeWaxis[8], planeWaxis[9], planeWaxis[10]};
        
        float speed = m3dGetMagnitude(planeVel);

        // Move particles
        for (auto it = particles.begin(); it != particles.end(); ) {
            // In world space, air is stationary. 
            // Only move by the 'deflection' (it->vel)
            it->pos[0] += it->vel[0] * dt;
            it->pos[1] += it->vel[1] * dt;
            it->pos[2] += it->vel[2] * dt;
            it->life -= dt;

            // Deflection: if near plane, deflect along plane's local down axis
            M3DVector3f rel;
            m3dSubtractVectors3(rel, it->pos, planePos);
            float distSq = m3dGetMagnitudeSquared(rel);
            if (distSq < 225.0f) { // 15 units radius
                float dist = sqrtf(distSq);
                float effect = (1.0f - dist/15.0f) * 15.0f;
                // Deflect in local negative UP direction
                it->vel[0] -= up[0] * effect * dt;
                it->vel[1] -= up[1] * effect * dt;
                it->vel[2] -= up[2] * effect * dt;
            }

            if (it->life <= 0) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }

        // Spawn new particles in a box ahead of the plane
        spawnTimer += dt;
        if (spawnTimer > 0.002f && particles.size() < maxParticles) {
            spawnTimer = 0;
            AirParticle p;
            
            // Spawn distance ahead of plane based on speed
            float offX = 10.0f + speed * 0.1f; 
            float offY = ((rand() % 200) / 100.0f - 1.0f) * 15.0f;
            float offZ = ((rand() % 200) / 100.0f - 1.0f) * 5.0f;
            
            p.pos[0] = planePos[0] + forward[0] * offX + side[0] * offY + up[0] * offZ;
            p.pos[1] = planePos[1] + forward[1] * offX + side[1] * offY + up[1] * offZ;
            p.pos[2] = planePos[2] + forward[2] * offX + side[2] * offY + up[2] * offZ;

            p.vel[0] = 0; p.vel[1] = 0; p.vel[2] = 0; 
            
            p.maxLife = 2.0f; // Longer life so they pass the plane
            p.life = p.maxLife;
            particles.push_back(p);
        }
    }

    void draw(M3DMatrix44f mv, M3DVector3f planeVel) {
        glPushMatrix();
        glLoadMatrixf(mv);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Calculate a generic "back" vector for tails if plane is stationary
        M3DVector3f back;
        float speed = m3dGetMagnitude(planeVel);
        if (speed > 1.0f) {
            m3dCopyVector3(back, planeVel);
            m3dScaleVector3(back, -2.0f / speed); // 2 meter tail
        } else {
            back[0] = -2.0f; back[1] = 0; back[2] = 0;
        }

        glBegin(GL_LINES);
        for (const auto& p : particles) {
            float alpha = p.life / p.maxLife;
            glColor4f(0.7f, 0.8f, 1.0f, alpha * 0.4f);
            glVertex3fv(p.pos);
            
            M3DVector3f tailEnd;
            m3dAddVectors3(tailEnd, p.pos, back);
            glVertex3fv(tailEnd);
        }
        glEnd();
        glDisable(GL_BLEND);
        glPopMatrix();
    }
};
