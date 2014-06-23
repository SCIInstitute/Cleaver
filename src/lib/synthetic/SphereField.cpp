#include "SphereField.h"
#include <cmath>

using namespace cleaver;

SphereField::SphereField(const vec3 &cx, float r, const BoundingBox &bounds) :
    m_bounds(bounds), m_cx(cx), m_r(r)
{
}

double SphereField::valueAt(double x, double y, double z) const
{
    return valueAt(vec3(x,y,z));
}

double SphereField::valueAt(const vec3 &x) const
{
    double d = m_r - fabs(length(x - m_cx));
    return d;
}

void SphereField::setBounds(const cleaver::BoundingBox &bounds)
{
    m_bounds = bounds;
}


BoundingBox SphereField::bounds() const
{
    return m_bounds;
}


