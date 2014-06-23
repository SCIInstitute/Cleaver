

#ifndef ABSTRACTFIELD_H
#define ABSTRACTFIELD_H

#include "BoundingBox.h"
#include "vec3.h"

namespace cleaver
{

template <typename T>
class AbstractField
{
    public:
    virtual ~AbstractField(){}
    virtual T valueAt(double x, double y, double z) const = 0;
    virtual T operator()(double x, double y, double z) const { return valueAt(x,y,z); }
    virtual T valueAt(const vec3 &x) const { return valueAt(x.x, x.y, x.z); }
    virtual BoundingBox bounds() const = 0;

    void setName(const std::string &name){ m_name = name; }
    std::string name() const { return m_name; }

    protected:
        std::string m_name;
};

}

#endif // ABSTRACTFIELD_H
