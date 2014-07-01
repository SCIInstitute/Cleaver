#include "TrackballCamera.h"
#include <Cleaver/vec3.h>
#include <cmath>
#include <QMatrix4x4>

using namespace cleaver;

#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif


double angleBetween(const QVector3D &v1, const QVector3D &v2)
{
    return acos( QVector3D::dotProduct(v1,v2) / (v1.length()*v2.length()) );
}

TrackballCamera::TrackballCamera()
{
    reset();
}

void TrackballCamera::reset()
{
    float view_distance = 3.5*cleaver::length(m_targetBounds.center() - m_targetBounds.origin);
    vec3 target_center = m_targetBounds.center();
    m_target = QVector3D(target_center.x, target_center.y, target_center.z);
    m_eye.setX(0); // m_target.x() +  view_distance);
    m_eye.setY(0); //m_target.y() +  view_distance);
    m_eye.setZ(m_target.z() - view_distance);

    m_scale  = (m_target - m_eye).length();

    // temporary up vector
    m_up = QVector3D(0,1,0);

    m_viewDir = (m_target - m_eye).normalized();
    m_right = QVector3D::crossProduct(m_viewDir, m_up);
    m_up = QVector3D::crossProduct(m_right, m_viewDir);

    m_orientation = QQuaternion();
    QMatrix4x4 matrix;
    //0.261093 -0.435186 -0.861652 0
    //-0.0523809 0.884911 -0.462805 0
    //0.963891 0.165969 0.208249 0
    // 0 0 0 1


    computeViewMatrix();
}

cleaver::vec3 TrackballCamera::e() const
{
    vec3 eye(m_eye.x(), m_eye.y(), m_eye.z());
    return eye;
}

cleaver::vec3 TrackballCamera::t() const
{
    vec3 target(m_target.x(), m_target.y(), m_target.z());
    return target;
}

cleaver::vec3 TrackballCamera::u() const
{
    vec3 up(m_up.x(), m_up.y(), m_up.z());
    return up;
}

cleaver::vec3 TrackballCamera::s() const
{
    return vec3(1,1,1);
}

float* TrackballCamera::viewMatrix()
{
    return m_viewMatrix;
}

void TrackballCamera::computeViewMatrix()
{
    //------------------
    m_viewMatrix[0]  = m_right.x();
    m_viewMatrix[4]  = m_right.y();
    m_viewMatrix[8]  = m_right.z();
    m_viewMatrix[12] = 0;
    //------------------
    m_viewMatrix[1]  = m_up.x();
    m_viewMatrix[5]  = m_up.y();
    m_viewMatrix[9]  = m_up.z();
    m_viewMatrix[13] = 0;
    //------------------
    m_viewMatrix[2]  = -m_viewDir.x();
    m_viewMatrix[6]  = -m_viewDir.y();
    m_viewMatrix[10] = -m_viewDir.z();
    m_viewMatrix[14] = 0;
    //------------------

    m_viewMatrix[12] = -1*QVector3D::dotProduct(m_right,   m_eye);
    m_viewMatrix[13] = -1*QVector3D::dotProduct(m_up,      m_eye);
    m_viewMatrix[14] =    QVector3D::dotProduct(m_viewDir, m_eye);
    //------------------
    m_viewMatrix[3] = 0;
    m_viewMatrix[7] = 0;
    m_viewMatrix[11] = 0;
    m_viewMatrix[15] = 1.0;
}


QVector3D TrackballCamera::screenToBall(const QVector2D &s)
{
    // Scale bounds to [0,0] - [2,2]
    double x = s.x() / (m_width/2.0);
    double y = s.y() / (m_height/2.0);

    // Translate 0,0 to the center
    x = x - 1;

    // Flip so +Y is up instead of down
    y = y - 1;

    double z2 = 1 - x*x - y*y;
    double z = z2 > 0 ? sqrt(z2) : 0;

    // tp is now the (x,y,z) coordinate of the
    // point on there sphere beneat the mouse
    QVector3D p(x, y, z);
    p.normalize();
    return p;
}

void TrackballCamera::rotateBetween(const QVector2D &s1, const QVector2D &s2)
{
    // first convert to sphere coordinates
    QVector3D p1 = screenToBall(s1);
    QVector3D p2 = screenToBall(s2);


    // get rotation axis
    QVector3D axis = QVector3D::crossProduct(p1, p2);
    axis = axis.normalized();

    // get angle
    float angle = angleBetween(p1, p2);


    // rotate the camera
    QQuaternion delta = QQuaternion::fromAxisAndAngle(axis, 180*angle/M_PI);

    delta = delta.normalized();

    //m_orientation = m_orientation * delta;
    m_orientation = delta * m_orientation;
    m_orientation.normalize();

    //m_viewDir = delta.rotatedVector(m_viewDir);
    //m_up      = delta.rotatedVector(m_up);
    //m_right   = delta.rotatedVector(m_right);


    // WHY ARE NOT CALLING m.rotate(quaternion)?  WHERE IS THE QUATERNION EVEN APPLIED?
    // ALSO, HOW CAN THERE BE ROTATION AT ALL????

    //q *= delta;
    // compose delta with the previous orientation
    computeViewMatrix();
}


void TrackballCamera::setView(const vec3 &eye, const vec3 &target)
{
    /*
    m_e = eye;
    m_t = target;

    vec3 v = normalize(m_t - m_e);
    m_viewDir = QVector3D(v.x, v.y, v.z);
    m_r = cross(v, vec3(0,1,0));
    m_u = cross(m_r, v);

    m_scale = length(m_t - m_e);

    computeViewMatrix();
    */
}

void TrackballCamera::zoom(float dz)
{

    if(dz > 0)
        m_scale /= 1.05f;
    else
        m_scale *= 1.05f;

    // don't allow inversion
    if(m_scale == 0)
        m_scale += 0.0001f;

    m_eye = m_target - m_scale*m_viewDir;

    computeViewMatrix();

}

void TrackballCamera::pan(float dx, float dy)
{
    m_eye += 0.05f*(dx*m_right + dy*m_up);
    m_target += 0.05f*(dx*m_right + dy*m_up);
    computeViewMatrix();
}

void TrackballCamera::rotate(float theta, float phi)
{
    /*
    theta *= 0.002f;
    phi   *= 0.002f;
    vec3 origin = m_e - m_t;
    m_e.z = origin.z*cos(theta) - origin.x*sin(theta);
    m_e.x = origin.z*sin(theta) + origin.x*cos(theta);
    m_e.y = origin.y;
    m_e += m_t;

    this->setView(m_e, m_t);

    phi += asin((m_e.y - m_t.y) / m_scale);
    float h = sin(phi)*m_scale;
    float d = cos(phi)*m_scale;

    vec3 flatView(m_viewDir.x(), 0, m_viewDir.z());
    flatView = normalize(flatView);

    m_e.x = m_t.x - d*flatView.x;
    m_e.y = m_t.y + h;
    m_e.z = m_t.z - d*flatView.z;

    this->setView(m_e, m_t);

    computeViewMatrix();
    */

    std::cout << "THIS FUNCTION SHOULD NEVER BE CALLED" << std::endl;
}

void TrackballCamera::setTargetBounds(const cleaver::BoundingBox &bounds)
{
    m_targetBounds = bounds;
}


void TrackballCamera::setBallSize(int w, int h)
{
    m_width = w;
    m_height = h;
}

QQuaternion TrackballCamera::rot()
{
    return m_orientation;
}
