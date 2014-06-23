#ifndef ABSTRACTSCALARFIELD_H
#define ABSTRACTSCALARFIELD_H

#include "AbstractField.h"
#include "vec3.h"

namespace cleaver
{

class AbstractScalarField : public AbstractField<double>
{
public:
    virtual ~AbstractScalarField();
    virtual double valueAt(double x, double y, double z) const = 0;
    virtual double operator()(double x, double y, double z) const
    {
        return valueAt(x,y,z);
    }
    virtual double valueAt(const vec3 &x) const
    {
        return valueAt(x.x, x.y, x.z);
    }

    virtual BoundingBox bounds() const = 0;

    void setName(const std::string &name);
    std::string name() const;

protected:
        std::string m_name;
};

inline AbstractScalarField::~AbstractScalarField(){ }
inline void AbstractScalarField::setName(const std::string &name){ m_name = name; }
inline std::string AbstractScalarField::name() const { return m_name; }


}

#endif // ABSTRACTSCALARFIELD_H
