#include "SphereSizingField.h"
#include <algorithm>
#include <cmath>

SphereSizingField::SphereSizingField(const SphereField *field)
{
    m_sphereField = field;
}

double SphereSizingField::valueAt(double x, double y, double z) const
{
    return valueAt(cleaver::vec3(x,y,z));
}

double SphereSizingField::valueAt(const cleaver::vec3 &x) const
{
    double d = fabs(m_sphereField->valueAt(x));

    d = std::max(d, 0.15*m_sphereField->m_r);

    return d;
}

cleaver::BoundingBox SphereSizingField::bounds() const
{
    return m_sphereField->bounds();
}
