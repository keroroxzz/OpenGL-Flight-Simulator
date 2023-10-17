#include "Joints.h"

void Joint::update()
{
    child->updateDynamic();
    child->updateChildrenDynamic();
}

void Joint::updateEffect(DynamicModel* base)
{
    child->updateEffect(base);
}

FixedJoint::FixedJoint(DynamicModel* parent, DynamicModel* child, M3DVector3f pos, float r, float p, float y) :Joint(parent, child)
{
    glPushMatrix();

    //calculate the matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(pos[0], pos[1], pos[2]);
    glRotated(r, 1.0, 0.0, 0.0);
    glRotated(p, 0.0, 1.0, 0.0);
    glRotated(y, 0.0, 0.0, 1.0);

    glGetFloatv(GL_MODELVIEW_MATRIX, T);
    m3dInvertMatrix44(IT, T);

    glPopMatrix();

    //attact joint
    parent->attach(this);
    child->parent_joint = this;
}

void FixedJoint::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);

    child->display(cvmatrix, model_view_loc);

    glPopMatrix();
}

void FixedJoint::visualize(M3DMatrix44f cvmatrix)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);

    child->visualize(cvmatrix);

    glPopMatrix();
}

void FixedJoint::updatePositionVelocity(DynamicModel* base)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);

    child->updatePositionVelocity(base);

    glPopMatrix();
}

RevoluteJoint::RevoluteJoint(DynamicModel* parent, DynamicModel* child, M3DVector3f pos, float r, float p, float y, float ax, float ay, float az) :FixedJoint(parent, child, pos, r, p, y)
{
    angle = 0.0;

    //init axis value
    raxis[0] = ax;
    raxis[1] = ay;
    raxis[2] = az;

    //normalize it
    m3dNormalizeVector(raxis);
}

void RevoluteJoint::display(M3DMatrix44f cvmatrix, GLint model_view_loc)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);
    glRotatef(angle, raxis[0], raxis[1], raxis[2]);

    child->display(cvmatrix, model_view_loc);

    glPopMatrix();
}

void RevoluteJoint::visualize(M3DMatrix44f cvmatrix)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);
    glRotatef(angle, raxis[0], raxis[1], raxis[2]);

    child->visualize(cvmatrix);

    glPopMatrix();
}

void RevoluteJoint::updatePositionVelocity(DynamicModel* base)
{
    glPushMatrix();

    //apply joint transformation
    glMultMatrixf(T);
    glRotatef(angle, raxis[0], raxis[1], raxis[2]);

    child->updatePositionVelocity(base);

    glPopMatrix();
}

void RevoluteJoint::moveAngle(float target, float ratio)
{
    angle = ratio * target + (1.0 - ratio) * angle;
}