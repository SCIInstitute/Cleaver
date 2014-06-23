#include "TargetCamera.h"
#include <cmath>

using namespace cleaver;

TargetCamera::TargetCamera()
{
    reset();
}

cleaver::vec3 TargetCamera::e() const
{
    return m_e;
}

cleaver::vec3 TargetCamera::t() const
{
    return m_t;
}

cleaver::vec3 TargetCamera::u() const
{
    return m_u;
}

cleaver::vec3 TargetCamera::s() const
{
    return cleaver::vec3(m_scale, m_scale, m_scale);
}

float* TargetCamera::viewMatrix()
{
    return m_viewMatrix;
}

void TargetCamera::computeViewMatrix()
{

    cleaver::vec3 forward = cleaver::normalize(m_t - m_e);
    cleaver::vec3    side = cleaver::normalize(cross(forward, m_u));
    cleaver::vec3      up = cleaver::normalize(cross(side, forward));

    //------------------
    m_viewMatrix[0]  = (float)side.x;
    m_viewMatrix[4]  = (float)side.y;
    m_viewMatrix[8]  = (float)side.z;
    m_viewMatrix[12] = (float)0;
    //------------------
    m_viewMatrix[1]  = (float)up.x;
    m_viewMatrix[5]  = (float)up.y;
    m_viewMatrix[9]  = (float)up.z;
    m_viewMatrix[13] = (float)0;
    //------------------
    m_viewMatrix[2]  = (float)-forward.x;
    m_viewMatrix[6]  = (float)-forward.y;
    m_viewMatrix[10] = (float)-forward.z;
    m_viewMatrix[14] = (float)0;
    //------------------

    m_viewMatrix[12] = (float)(-1*dot(    side, m_e));
    m_viewMatrix[13] = (float)(-1*dot(      up, m_e));
    m_viewMatrix[14] = (float)(dot( forward, m_e));
    //------------------
    m_viewMatrix[3] = 0;
    m_viewMatrix[7] = 0;
    m_viewMatrix[11] = 0;
    m_viewMatrix[15] = 1.0;
}

void TargetCamera::reset()
{
    double view_distance = 1.5*cleaver::length(m_targetBounds.center() - m_targetBounds.origin);
    m_t   = m_targetBounds.center();
    m_e.x = m_t.x + view_distance;
    m_e.y = m_t.y + view_distance;
    m_e.z = m_t.z +-view_distance;

    m_scale = (float)length(m_t - m_e);

    m_u.x = 0;
    m_u.y = 1;
    m_u.z = 0;

    m_viewDir = normalize(m_t - m_e);
    m_r = cross(m_viewDir, vec3(0,1,0));
    m_u = cross(m_r, m_viewDir);

    computeViewMatrix();
}

void TargetCamera::setView(const vec3 &eye, const vec3 &target)
{
    m_e = eye;
    m_t = target;

    m_viewDir = normalize(m_t - m_e);
    m_r = cross(m_viewDir, vec3(0,1,0));
    m_u = cross(m_r, m_viewDir);

    m_scale = (float)length(m_t - m_e);

    computeViewMatrix();
}

void TargetCamera::zoom(float dz)
{
    if(dz > 0)
        m_scale /= 1.05f;
    else
        m_scale *= 1.05f;

    // don't allow inversion
    if(m_scale == 0)
        m_scale += 0.0001f;

    m_e = m_t - m_scale*m_viewDir;

    computeViewMatrix();
}

void TargetCamera::pan(float dx, float dy)
{
    m_e += 0.05*(dx*m_r + dy*m_u);
    m_t += 0.05*(dx*m_r + dy*m_u);
    computeViewMatrix();
}

void TargetCamera::rotate(float theta, float phi)
{
    theta *= 0.002f;
    phi   *= 0.002f;
    vec3 origin = m_e - m_t;
    m_e.z = origin.z*cos(theta) - origin.x*sin(theta);
    m_e.x = origin.z*sin(theta) + origin.x*cos(theta);
    m_e.y = origin.y;
    m_e += m_t;

    this->setView(m_e, m_t);

    phi += (float)asin((m_e.y - m_t.y) / m_scale);
    float h = sin(phi)*m_scale;
    float d = cos(phi)*m_scale;

    vec3 flatView(m_viewDir.x, 0, m_viewDir.z);
    flatView = normalize(flatView);

    m_e.x = m_t.x - d*flatView.x;
    m_e.y = m_t.y + h;
    m_e.z = m_t.z - d*flatView.z;

    this->setView(m_e, m_t);

    computeViewMatrix();
}

void TargetCamera::setTargetBounds(const cleaver::BoundingBox &bounds)
{
    m_targetBounds = bounds;
}
