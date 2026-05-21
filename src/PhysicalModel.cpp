#include "PhysicalModel.h"
#include "Joints.h"
#include <cmath>

#define VEC_SIZE 4.0
float dt = 0.0002;
// extern bool showForce;

std::ostream& operator<<(std::ostream& o, M3DMatrix44d m)
{
    std::cout << m[0] << "  \t" << m[4] << "  \t" << m[8] << "  \t" << m[12] << std::endl;
    std::cout << m[1] << "  \t" << m[5] << "  \t" << m[9] << "  \t" << m[13] << std::endl;
    std::cout << m[2] << "  \t" << m[6] << "  \t" << m[10] << "  \t" << m[14] << std::endl;
    std::cout << m[3] << "  \t" << m[7] << "  \t" << m[11] << "  \t" << m[15] << std::endl << std::endl;

    return o;
}

std::ostream& operator<<(std::ostream& o, M3DMatrix44f m)
{
    std::cout << m[0] << "  \t" << m[4] << "  \t" << m[8] << "  \t" << m[12] << std::endl;
    std::cout << m[1] << "  \t" << m[5] << "  \t" << m[9] << "  \t" << m[13] << std::endl;
    std::cout << m[2] << "  \t" << m[6] << "  \t" << m[10] << "  \t" << m[14] << std::endl;
    std::cout << m[3] << "  \t" << m[7] << "  \t" << m[11] << "  \t" << m[15] << std::endl << std::endl;

    return o;
}

DynamicModel::DynamicModel(ObjModel* obj, float mass) :obj(obj), mass(mass), wpos{ 0 }, wvel{ 0 }
{
    M3DMatrix44f axis_ = { 1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0 };
    M3DMatrix33f inertia_ = { 166.67,0.0,0.0, 0.0,166.67,0.0, 0.0,0.0,166.67 };
    M3DMatrix33f inv_inertia_ = { 1.0 / 166.67,0.0,0.0, 0.0,1.0 / 166.67,0.0, 0.0,0.0,1.0 / 166.67 };

    M3DVector3f position_ = { 0.0,0.0,0.0};
    M3DVector3f velocity_ = { 0.0,0.0,0.0 };
    M3DVector3f angular_velocity_ = { 0.0,0.0,0.0 };
    
    memcpy(axis, axis_, 16 * sizeof(float));
    memcpy(inertia, inertia_, 9 * sizeof(float));
    memcpy(inv_inertia, inv_inertia_, 9 * sizeof(float));

    memcpy(position, position_, 3 * sizeof(float));
    memcpy(velocity, velocity_, 3 * sizeof(float));
    memcpy(angular_velocity, angular_velocity_, 3 * sizeof(float));
}

void DynamicModel::rotate(float r, float p, float y)
{
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotated(r, 1.0, 0.0, 0.0);
    glRotated(p, 0.0, 1.0, 0.0);
    glRotated(y, 0.0, 0.0, 1.0);
    glMultMatrixf(axis);
    glGetFloatv(GL_MODELVIEW_MATRIX, axis);
    glPopMatrix();

    //normalization
    m3dNormalizeVector(&axis[0]);
    m3dCrossProduct(&axis[8], &axis[0], &axis[4]);
    m3dCrossProduct(&axis[4], &axis[8], &axis[0]);
    m3dNormalizeVector(&axis[4]);
    m3dNormalizeVector(&axis[8]);
}

void DynamicModel::setInertia(M3DMatrix33f inertia)
{
    M3DMatrix44f temp, temp2;

    m3dCopyMatrix33(this->inertia, inertia);

    m3dLoadIdentity44(temp2);
    m3dCopyVector3(&temp2[0], &inertia[0]);
    m3dCopyVector3(&temp2[4], &inertia[3]);
    m3dCopyVector3(&temp2[8], &inertia[6]);

    m3dInvertMatrix44(temp, temp2);

    m3dCopyVector3(&inv_inertia[0], &temp[0]);
    m3dCopyVector3(&inv_inertia[3], &temp[4]);
    m3dCopyVector3(&inv_inertia[9], &temp[8]);
}

void DynamicModel::applyForceModelCoord(M3DVector3f p, M3DVector3f f)
{
    M3DVector3f moment, momentum, delta_ang_vel, delta_ang_vel_wc;

    //calculate the moment
    m3dCrossProduct(moment, p, f);

    //delta angular velocity in model coord
    delta_ang_vel[0] = inv_inertia[0] * moment[0] + inv_inertia[3] * moment[1] + inv_inertia[6] * moment[2];
    delta_ang_vel[1] = inv_inertia[1] * moment[0] + inv_inertia[4] * moment[1] + inv_inertia[7] * moment[2];
    delta_ang_vel[2] = inv_inertia[2] * moment[0] + inv_inertia[5] * moment[1] + inv_inertia[8] * moment[2];
    
    //transfer it to world coord
    delta_ang_vel_wc[0] = delta_ang_vel[0] * axis[0] + delta_ang_vel[1] * axis[4] + delta_ang_vel[2] * axis[8];
    delta_ang_vel_wc[1] = delta_ang_vel[0] * axis[1] + delta_ang_vel[1] * axis[5] + delta_ang_vel[2] * axis[9];
    delta_ang_vel_wc[2] = delta_ang_vel[0] * axis[2] + delta_ang_vel[1] * axis[6] + delta_ang_vel[2] * axis[10];
    m3dScaleVector3(delta_ang_vel_wc, dt);

    momentum[0] = f[0] * axis[0] + f[1] * axis[4] + f[2] * axis[8];
    momentum[1] = f[0] * axis[1] + f[1] * axis[5] + f[2] * axis[9];
    momentum[2] = f[0] * axis[2] + f[1] * axis[6] + f[2] * axis[10];
    m3dScaleVector3(momentum, dt / mass);

    m3dAddVectors3(angular_velocity, angular_velocity, delta_ang_vel_wc);
    m3dAddVectors3(velocity, velocity, momentum);
}

void DynamicModel::applyForce(M3DVector3f f)
{
    M3DVector3f  momentum;

    m3dCopyVector3(momentum, f);
    m3dScaleVector3(momentum, dt / mass);

    m3dAddVectors3(velocity, velocity, momentum);
}

void DynamicModel::applyForce(M3DVector3f p, M3DVector3f f)
{
    M3DVector3f p2, moment_m, moment, delta_ang_vel, delta_ang_vel_wc;

    // linear part
    applyForce(f);

    // moment
    m3dSubtractVectors3(p2, p, position);
    m3dCrossProduct(moment, p2, f);

    moment_m[0] = m3dDotProduct(moment, &axis[0]);
    moment_m[1] = m3dDotProduct(moment, &axis[4]);
    moment_m[2] = m3dDotProduct(moment, &axis[8]);

    //delta angular velocity in model coord
    delta_ang_vel[0] = inv_inertia[0] * moment_m[0] + inv_inertia[3] * moment_m[1] + inv_inertia[6] * moment_m[2];
    delta_ang_vel[1] = inv_inertia[1] * moment_m[0] + inv_inertia[4] * moment_m[1] + inv_inertia[7] * moment_m[2];
    delta_ang_vel[2] = inv_inertia[2] * moment_m[0] + inv_inertia[5] * moment_m[1] + inv_inertia[8] * moment_m[2];

    //transfer it to world coord
    delta_ang_vel_wc[0] = delta_ang_vel[0] * axis[0] + delta_ang_vel[1] * axis[4] + delta_ang_vel[2] * axis[8];
    delta_ang_vel_wc[1] = delta_ang_vel[0] * axis[1] + delta_ang_vel[1] * axis[5] + delta_ang_vel[2] * axis[9];
    delta_ang_vel_wc[2] = delta_ang_vel[0] * axis[2] + delta_ang_vel[1] * axis[6] + delta_ang_vel[2] * axis[10];
    m3dScaleVector3(delta_ang_vel_wc, dt);

    m3dAddVectors3(angular_velocity, angular_velocity, delta_ang_vel_wc);
}

void DynamicModel::applyCFDAerodynamics(DynamicModel* target, bool visualize, M3DMatrix44f cvmatrix)
{
    if (!obj) {
        // printf("[Physics] Skip: no object\n");
        return;
    }
    if (obj->getNumIndices() == 0) {
        // printf("[Physics] Skip: no indices\n");
        return;
    }
    if (!target) target = this;

    // printf("[Physics] Starting CFD for %p (target %p), indices=%d\n", this, target, obj->getNumIndices());

    int numIndices = obj->getNumIndices();
    GLushort* indices = obj->getIndices();
    GLfloat* verts = obj->getVertices();
    GLfloat* norms = obj->getNormals();

    if (visualize && cvmatrix) {
        glPushMatrix();
        glLoadMatrixf(cvmatrix);
        glDisable(GL_LIGHTING);
        glBegin(GL_LINES);
    }

    for (int i = 0; i < numIndices; i += 3) {
        if (i + 2 >= numIndices) break; 
        
        int i0 = indices[i];
        int i1 = indices[i+1];
        int i2 = indices[i+2];

        M3DVector3f v0 = {verts[i0*3], verts[i0*3+1], verts[i0*3+2]};
        M3DVector3f v1 = {verts[i1*3], verts[i1*3+1], verts[i1*3+2]};
        M3DVector3f v2 = {verts[i2*3], verts[i2*3+1], verts[i2*3+2]};

        M3DVector3f center;
        center[0] = (v0[0] + v1[0] + v2[0]) / 3.0f;
        center[1] = (v0[1] + v1[1] + v2[1]) / 3.0f;
        center[2] = (v0[2] + v1[2] + v2[2]) / 3.0f;

        M3DVector3f n0 = {norms[i0*3], norms[i0*3+1], norms[i0*3+2]};
        M3DVector3f n1 = {norms[i1*3], norms[i1*3+1], norms[i1*3+2]};
        M3DVector3f n2 = {norms[i2*3], norms[i2*3+1], norms[i2*3+2]};
        M3DVector3f n;
        n[0] = (n0[0] + n1[0] + n2[0]) / 3.0f;
        n[1] = (n0[1] + n1[1] + n2[1]) / 3.0f;
        n[2] = (n0[2] + n1[2] + n2[2]) / 3.0f;
        
        float nmag = m3dGetMagnitude(n);
        if (nmag < 0.0001f) continue; 
        m3dScaleVector3(n, 1.0f / nmag);

        M3DVector3f e1, e2, cross;
        m3dSubtractVectors3(e1, v1, v0);
        m3dSubtractVectors3(e2, v2, v0);
        m3dCrossProduct(cross, e1, e2);
        float area = m3dGetMagnitude(cross) * 0.5f;
        if (std::isnan(area) || area < 0.00001f) continue;

        M3DVector3f wnormal;
        wnormal[0] = waxis[0]*n[0] + waxis[4]*n[1] + waxis[8]*n[2];
        wnormal[1] = waxis[1]*n[0] + waxis[5]*n[1] + waxis[9]*n[2];
        wnormal[2] = waxis[2]*n[0] + waxis[6]*n[1] + waxis[10]*n[2];
        m3dNormalizeVector(wnormal);

        M3DVector3f wcenter;
        wcenter[0] = waxis[0]*center[0] + waxis[4]*center[1] + waxis[8]*center[2] + waxis[12];
        wcenter[1] = waxis[1]*center[0] + waxis[5]*center[1] + waxis[9]*center[2] + waxis[13];
        wcenter[2] = waxis[2]*center[0] + waxis[6]*center[1] + waxis[10]*center[2] + waxis[14];

        // Local velocity calculation (Linear + Angular component) for Damping
        M3DVector3f r_arm, v_rot, v_surf;
        m3dSubtractVectors3(r_arm, wcenter, target->wpos);
        m3dCrossProduct(v_rot, target->angular_velocity, r_arm);
        
        v_surf[0] = target->wvel[0] + v_rot[0];
        v_surf[1] = target->wvel[1] + v_rot[1];
        v_surf[2] = target->wvel[2] + v_rot[2];

        M3DVector3f v_rel;
        v_rel[0] = -v_surf[0];
        v_rel[1] = -v_surf[1];
        v_rel[2] = -v_surf[2];
        
        float vdotn = m3dDotProduct(v_rel, wnormal);
        
        if (vdotn < 0) {
            float pressure = 0.5f * 1.225f * vdotn * vdotn;
            
            M3DVector3f force;
            force[0] = -wnormal[0] * pressure * area;
            force[1] = -wnormal[1] * pressure * area;
            force[2] = -wnormal[2] * pressure * area;
            
            float mag = m3dGetMagnitude(force);
            if (std::isnan(mag)) {
                // printf("[Physics] ERROR: NaN force detected!\n");
                continue;
            }
            
            if (mag > 10000.0f) {
                m3dScaleVector3(force, 10000.0f / mag);
            }

            target->applyForce(wcenter, force);

            if (visualize && cvmatrix) {
                float magnitude = m3dGetVectorLength(force);
                if (magnitude > 0.1f)
                {
                    M3DVector3f f_log;
                    m3dCopyVector3(f_log, force);
                    m3dScaleVector3(f_log, log10f(magnitude) / magnitude);

                    glColor3f(2.0, 0.0, 0.0);
                    glVertex3d(wcenter[0], wcenter[1], wcenter[2]);
                    glVertex3d(wcenter[0] + f_log[0], wcenter[1] + f_log[1], wcenter[2] + f_log[2]);
                }
            }
        }
    }

    if (visualize && cvmatrix) {
        glEnd();
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
    // printf("[Physics] CFD complete for %p\n", this);
}

void DynamicModel::applyTorque(M3DVector3f moment)
{
    M3DVector3f delta_ang_vel, delta_ang_vel_wc;

    //delta angular velocity in model coord
    delta_ang_vel[0] = (inv_inertia[0] * moment[0] + inv_inertia[3] * moment[1] + inv_inertia[6] * moment[2]) * dt;
    delta_ang_vel[1] = (inv_inertia[1] * moment[0] + inv_inertia[4] * moment[1] + inv_inertia[7] * moment[2]) * dt;
    delta_ang_vel[2] = (inv_inertia[2] * moment[0] + inv_inertia[5] * moment[1] + inv_inertia[8] * moment[2]) * dt;

    //transfer it to world coord
    delta_ang_vel_wc[0] = delta_ang_vel[0] * axis[0] + delta_ang_vel[1] * axis[4] + delta_ang_vel[2] * axis[8];
    delta_ang_vel_wc[1] = delta_ang_vel[0] * axis[1] + delta_ang_vel[1] * axis[5] + delta_ang_vel[2] * axis[9];
    delta_ang_vel_wc[2] = delta_ang_vel[0] * axis[2] + delta_ang_vel[1] * axis[6] + delta_ang_vel[2] * axis[10];

    m3dAddVectors3(angular_velocity, angular_velocity, delta_ang_vel_wc);
}

void DynamicModel::updateDynamic()
{
    GLdouble angv = m3dGetVectorLength(angular_velocity),
        vel = m3dGetVectorLength(velocity);

    if (angv > 0.0)
    {
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(angv * dt * 180.0 / 3.14159, angular_velocity[0] / angv, angular_velocity[1] / angv, angular_velocity[2] / angv);
        glMultMatrixf(axis);
        glGetFloatv(GL_MODELVIEW_MATRIX, axis);
        glPopMatrix();
        
        //normalization
        m3dNormalizeVector(&axis[0]);
        m3dCrossProduct(&axis[8], &axis[0], &axis[4]);
        m3dCrossProduct(&axis[4], &axis[8], &axis[0]);
        m3dNormalizeVector(&axis[4]);
        m3dNormalizeVector(&axis[8]);
    }

    //velocity
    position[0] += velocity[0] * dt;
    position[1] += velocity[1] * dt;
    position[2] += velocity[2] * dt;

}
void DynamicModel::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
    glPushMatrix();

    glTranslated(position[0], position[1], position[2]);
    glMultMatrixf(axis);

    if (obj != nullptr)
        obj->draw(3, GL_TRIANGLES, model_view_loc);

    displayChildren(cvmatrix, model_view_loc);

    glPopMatrix();
}

void DynamicModel::visualize(M3DMatrix44f cvmatrix)
{
    glPushMatrix();

    glPushMatrix();
    glLoadMatrixf(cvmatrix);
    glTranslated(wpos[0], wpos[1], wpos[2]);

    //If it's the base, show angular vel
    if (parent_joint == nullptr)
    {
        glColor3f(1.0, 1.0, 0.0);
        glBegin(GL_LINES);
        glVertex3d(0, 0, 0);
        glVertex3d(angular_velocity[0] * VEC_SIZE, angular_velocity[1] * VEC_SIZE, angular_velocity[2] * VEC_SIZE);
        glEnd();
    }

    //vel
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(wvel[0] * VEC_SIZE, wvel[1] * VEC_SIZE, wvel[2] * VEC_SIZE);
    glEnd();

    glPopMatrix();

    //translation
    glTranslated(position[0], position[1], position[2]);
    
    //y-axis
    glLineWidth(2);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(axis[0] * VEC_SIZE, axis[1] * VEC_SIZE, axis[2] * VEC_SIZE);
    glEnd();

    //y-axis
    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(axis[4] * VEC_SIZE, axis[5] * VEC_SIZE, axis[6] * VEC_SIZE);
    glEnd();

    //z-axis
    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(axis[8] * VEC_SIZE, axis[9] * VEC_SIZE, axis[10] * VEC_SIZE);
    glEnd();

    //rotation
    glMultMatrixf(axis);

    glColor3f(1.0, 1.0, 1.0);
    glutSolidCube(0.8);

    visualizeChildren(cvmatrix);

    glPopMatrix();
}

void DynamicModel::attach(Joint *joint)
{
    children.push_back(joint);
}

void DynamicModel::displayChildren(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
    glPushMatrix();

    for (auto i = children.begin(); i != children.end(); i++)
        (*i)->display(cvmatrix, model_view_loc);

    glPopMatrix();
}

void DynamicModel::visualizeChildren(M3DMatrix44f cvmatrix)
{
    glPushMatrix();

    for (auto i = children.begin(); i != children.end(); i++)
        (*i)->visualize(cvmatrix);

    glPopMatrix();
}

void DynamicModel::updateChildrenDynamic()
{
    for (auto i = children.begin(); i != children.end(); i++)
        (*i)->update();
}

void DynamicModel::updateEffect(DynamicModel* base)
{
    if (base == nullptr)
        base == this;

    applyEffect(base);

    for (auto i = children.begin(); i != children.end(); i++)
        (*i)->updateEffect(base);
}

void DynamicModel::updateChildrenPV(DynamicModel* base)
{
    for (auto i = children.begin(); i != children.end(); i++)
        (*i)->updatePositionVelocity(base);
}

void DynamicModel::updatePositionVelocity(DynamicModel* base)
{
    M3DVector3f np, dp, nv, acc;
    M3DMatrix44f temp;

    //init the matrix
    if (parent_joint == nullptr)
    {
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    //translation
    glTranslated(position[0], position[1], position[2]);

    //rotation
    glMultMatrixf(axis);

    glGetFloatv(GL_MODELVIEW_MATRIX, temp);

    m3dCopyVector3(np, &temp[12]);
    m3dSubtractVectors3(nv, np, wpos);
    m3dScaleVector3(nv, 1.0 / dt);

    //save new pos and velocity
    m3dCopyVector3(wvel, nv);
    m3dCopyVector3(wpos, np);

    //expertimental, calculate the axis under world coord.
    M3DMatrix44f iden;
    m3dLoadIdentity44(iden);
    m3dMatrixMultiply44(waxis, temp, iden);

    updateChildrenPV(base);

    //recover the matrix
    if (parent_joint == nullptr)
        glPopMatrix();
}

#define AIR_DENSITY 1.05

Aerofoil::Aerofoil(ObjModel* obj, float area_front, float area_side, float area_up, float dx, float dy, float dz, float liftfactor) :
    area{ area_front,area_side,area_up }, dragCoeff{ dx,dy,dz }, liftFactor(liftfactor), DynamicModel(obj, 0.0){}

void Aerofoil::applyEffect(DynamicModel *base)
{
    M3DVector3f force;
    float dot, forceMag;

    // Relative wind in world coord is -wvel
    // Project velocity into local coordinate system
    float vx = m3dDotProduct(wvel, &waxis[0]); // Forward
    float vy = m3dDotProduct(wvel, &waxis[4]); // Side
    float vz = m3dDotProduct(wvel, &waxis[8]); // Up

    float speedSq = vx*vx + vy*vy + vz*vz;
    float speed = sqrtf(speedSq);

    // Calculate AoA (angle between velocity and forward axis in the XZ plane)
    float aoa = 0.0f;
    if (speed > 0.1f) {
        aoa = atan2f(-vz, vx); 
    }

    // Lift Coefficient Cl approximation
    float cl = 0.0f;
    if (abs(aoa) < stallAngle) {
        cl = liftFactor * aoa;
    } else {
        // Post-stall drop off
        cl = liftFactor * stallAngle * (stallAngle / abs(aoa)) * 0.5f;
    }

    // Lift Force (Perpendicular to velocity, but for simplicity here we use local 'Up')
    forceMag = 0.5f * speedSq * AIR_DENSITY * cl * area[2];
    m3dCopyVector3(force, &waxis[8]);
    m3dScaleVector3(force, forceMag);
    base->applyForce(wpos, force);

    // Drag (Parasitic + Induced)
    // Parasitic drag for each axis
    for (int i = 0; i < 3; i++)
    {
        dot = m3dDotProduct(wvel, &waxis[i*4]);
        forceMag = 0.5f * dot * dot * AIR_DENSITY * dragCoeff[i] * area[i];
        forceMag *= (dot > 0 ? -1.0f : 1.0f);

        m3dCopyVector3(force, &waxis[i*4]);
        m3dScaleVector3(force, forceMag);
        base->applyForce(wpos, force);
    }

    // Induced drag (simplified: proportional to cl^2)
    float inducedDragCoeff = 0.1f; 
    float inducedDragMag = 0.5f * speedSq * AIR_DENSITY * (cl * cl * inducedDragCoeff) * area[2];
    m3dCopyVector3(force, wvel);
    if (speed > 0.1f) {
        m3dScaleVector3(force, -inducedDragMag / speed);
        base->applyForce(wpos, force);
    }
}

float Aerofoil::getAoA() {
    float vx = m3dDotProduct(wvel, &waxis[0]);
    float vz = m3dDotProduct(wvel, &waxis[8]);
    return atan2f(-vz, vx);
}

float Thruster::setThrust(float target, float ratio)
{
    thrust = ratio * target + (1.0 - ratio) * thrust;
    thrust = max(-max_thrust, min(max_thrust, thrust));

    return thrust / max_thrust;
}

float Thruster::getThrust()
{
    return thrust;
}

float Thruster::getMaxThrust()
{
    return max_thrust;
}

void Thruster::applyEffect(DynamicModel* base)
{
    M3DVector3f force;
    m3dCopyVector3(force, &waxis[0]);
    m3dScaleVector3(force, thrust);

    base->applyForce(wpos, force);
}
