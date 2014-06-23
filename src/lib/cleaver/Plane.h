#ifndef CLEAVER_PLANE_H
#define CLEAVER_PLANE_H

#include "vec3.h"

namespace cleaver{

class Plane{
public:
    Plane(double a, double b, double c, double d);
    Plane(const vec3 &n, double d);
    Plane(const vec3 &p, const vec3 &n);    
    void toScalars(double &a, double &b, double &c, double &d);
    static Plane throughPoints(const vec3 &p1, const vec3 &p2, const vec3 &p3);
    vec3 n;
    double d;
};

}  // namespace Cleaver


#endif //CLEAVER_PLANE_H
