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

    void setWarning(const bool &warning);
    bool getWarning() const;

    void setError(const std::string &error);
    std::string getError() const;

protected:
        std::string m_name;
        bool m_warning;
        std::string m_error;
};

inline AbstractScalarField::~AbstractScalarField(){ }
inline void AbstractScalarField::setName(const std::string &name){ m_name = name; }
inline std::string AbstractScalarField::name() const { return m_name; }
inline void AbstractScalarField::setWarning(const bool &warning) { m_warning = warning; }
inline bool AbstractScalarField::getWarning() const { return m_warning; }
inline void AbstractScalarField::setError(const std::string &error) { m_error = error; }
inline std::string AbstractScalarField::getError() const { return m_error; }
}

#endif // ABSTRACTSCALARFIELD_H
