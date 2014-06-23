#include "SphereVaryingField.h"
#include <algorithm>
#include <cmath>

SphereVaryingField::SphereVaryingField(const SphereField* field)
{
      m_sphereField = field;
}

double SphereVaryingField::valueAt(double x, double y, double z) const
{
    return valueAt(cleaver::vec3(x,y,z));
}

double SphereVaryingField::valueAt(const cleaver::vec3 &x) const
{
    double min = 0.05*m_sphereField->m_r;
    double max = 15*min;

    double t = (x.x / bounds().size.x);


    double tc = (x.z / bounds().size.z);
    double ta = fabs(tc - 0.5f) / 0.5f;

    t += ta;
    double d = (1-t)*min + (t)*max;

    return d;
}

cleaver::BoundingBox SphereVaryingField::bounds() const
{
    return m_sphereField->bounds();
}
