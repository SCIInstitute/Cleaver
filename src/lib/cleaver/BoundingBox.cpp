#include "BoundingBox.h"
#include <algorithm>

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
