#include "PlaneField.h"

PlaneField::PlaneField(double a, double b, double c, double d) :
    n(cleaver::vec3(a,b,c)), d(d)
{
}

PlaneField::PlaneField(const cleaver::vec3 &n, double d) :
    n(normalize(n)), d(d)
{
}

PlaneField::PlaneField(const cleaver::vec3 &n, const cleaver::vec3 &p) :
    n(n)
{
    d = n.dot(-1*p);
}

PlaneField::PlaneField(const cleaver::vec3 &p1,
                       const cleaver::vec3 &p2,
                       const cleaver::vec3 &p3)
{
    n = normalize(((p2 - p1).cross(p3 - p1)));
    d = -n.dot(p1);
}


double PlaneField::valueAt(double x, double y, double z) const
{
    return valueAt(cleaver::vec3(x,y,z));
}

double PlaneField::valueAt(const cleaver::vec3 &x) const
{
    return (dot(n,x) + d) / length(n);
}

void PlaneField::setBounds(const cleaver::BoundingBox &bounds)
{
    m_bounds = bounds;
}

cleaver::BoundingBox PlaneField::bounds() const
{
    return m_bounds;
}
