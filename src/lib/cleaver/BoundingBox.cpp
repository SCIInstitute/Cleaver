#include "BoundingBox.h"

namespace cleaver
{

vec3 BoundingBox::center() const
{
    return origin + 0.5*size;
}

vec3 BoundingBox::minCorner() const
{
    return origin;
}

vec3 BoundingBox::maxCorner() const
{
    return origin+size;
}

const bool BoundingBox::contains(const vec3 &x) const
{
    return (x >= this->minCorner() &&
            x <= this->maxCorner());
}

const bool BoundingBox::contains(const BoundingBox &box) const
{
    return(box.minCorner() >= this->minCorner() &&
           box.maxCorner() <= this->maxCorner());
}

const bool BoundingBox::intersects(const BoundingBox &box) const
{
    return(this->minCorner() < box.maxCorner() &&
           this->maxCorner() > box.minCorner());
}

/*
vec3 BoundingBox::intersectionWithLine(vec3 &p1, vec3 &p2) const
{
    vec3 l = normalize(p2 - p1);
    vec3 l0 = p1;

    // 6 planes to check, find closest intersection that is ON the box
    vec3 result;

    Planes planes[6];
    planes[0] = posXPlane();
    planes[1] = negXPlane();
    planes[2] = posYPlane();
    planes[3] = negYPlane();
    planes[4] = posZPlane();
    planes[5] = negZPlane();

    for(int p = 0; p < 6; p++)
    {
        // compute point

        // is point on the face?

        // is it closest so far?
    }

    return result;
}

Plane BoundingBox::posXPlane()
{
    return Plane(vec3(1,0,0), maxCorner());
}

Plane BoundingBox::negXPlane()
{
    return Plane(vec3(-1,0,0), minCorner());
}

Plane BoundingBox::posYPlane()
{
    return Plane(vec3(0,1,0), maxCorner());
}

Plane BoundingBox::negYPlane()
{
    return Plane(vec3(0,-1,0), minCorner());
}

Plane BoundingBox::posZPlane()
{
    return Plane(vec3(0,0,1), maxCorner());
}

Plane BoundingBox::negZPlane()
{
    return Plane(vec3(0,0,-1), minCorner());
}
*/

BoundingBox BoundingBox::merge(const BoundingBox &a, const BoundingBox &b)
{
    BoundingBox result;

    result.origin.x = std::min(a.origin.x, b.origin.x);
    result.origin.y = std::min(a.origin.y, b.origin.y);
    result.origin.z = std::min(a.origin.z, b.origin.z);

    result.size.x = std::max(a.maxCorner().x, b.maxCorner().x) - result.origin.x;
    result.size.y = std::max(a.maxCorner().y, b.maxCorner().y) - result.origin.y;
    result.size.z = std::max(a.maxCorner().z, b.maxCorner().z) - result.origin.z;

    return result;
}

}

