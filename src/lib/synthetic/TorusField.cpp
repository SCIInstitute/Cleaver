#include "TorusField.h"
#include <cmath>
#include <cassert>
#include <vector>

using namespace cleaver;

TorusField::TorusField(const vec3 &cx, float ur, float vr, const BoundingBox &bounds) :
    m_bounds(bounds), m_cx(cx), m_ur(ur), m_vr(vr)
{
}


double TorusField::valueAt(double x, double y, double z) const
{
 return valueAt(vec3(x,y,z));
}

double TorusField::valueAt(const vec3 &x) const
{
    vec3 xx = x - m_cx;

    double ring1 = (m_ur - sqrt(xx.y*xx.y + xx.z*xx.z));
    double total = ring1*ring1 + xx.x*xx.x - m_vr*m_vr;

    return -1*total;
}

void TorusField::setBounds(const BoundingBox &bounds)
{
    m_bounds = bounds;
}

BoundingBox TorusField::bounds() const
{
    return m_bounds;
}


void getorthobasis(vec3 &v1, vec3 &v2, vec3 &v3)
{
    double invLen;

    if (fabs(v1[0]) > fabs(v1[1]))
    {
        invLen = 1.0 / sqrt(v1[0] * v1[0] + v1[2] * v1[2]);
        v2[0] = -v1[2] * invLen;
        v2[1] = 0.0;
        v2[2] = v1[0] * invLen;
    }
    else
    {
        invLen = 1.0 / sqrt(v1[1] * v1[1] + v1[2] * v1[2]);
        v2[0] = 0.0;
        v2[1] = v1[2] * invLen;
        v2[2] = -v1[1] * invLen;
    }
    v3 = cross(v1, v2);

    assert(length(v1) > 0.0);
    assert(length(v2) > 0.0);
    assert(length(v3) > 0.0);
}

std::vector<vec3> TorusField::tensorAt(const vec3 &x) const
{
    // first build basis
    vec3 out = vec3(x - m_cx);
    vec3  up;    // = vec3(1,0,0);
    vec3 rot;    // = normalize(up.cross(out));

    float a = 4; // anisotropy factor

    getorthobasis(out, up, rot);

    std::vector<vec3> tensor;
    tensor.push_back(out);
    tensor.push_back(up);
    tensor.push_back(rot);

    return tensor;
}


