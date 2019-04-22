#include "Plane.h"

namespace cleaver{

Plane::Plane(double a, double b, double c, double d) : n(a,b,d), d(d)
{
}

Plane::Plane(const vec3 &n, double d) : n(n), d(d)
{
}

Plane::Plane(const vec3 &p, const vec3 &n) : n(n)
{
    d = -n.dot(p);
}

void Plane::toScalars(double &a, double &b, double &c, double &d)
{
    a = n.x;
    b = n.y;
    c = n.z;
    d = this->d;
}

Plane Plane::throughPoints(const vec3 &p1, const vec3 &p2, const vec3 &p3)
{
    vec3 n = normalize(((p2 - p1).cross(p3 - p1)));
    double d = -n.dot(p1);
    return Plane(n,d);
}

}
