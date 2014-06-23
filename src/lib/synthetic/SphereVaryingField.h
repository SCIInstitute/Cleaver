#ifndef SPHEREVARYINGFIELD_H
#define SPHEREVARYINGFIELD_H

#include <Cleaver/ScalarField.h>
#include "SphereField.h"

class SphereVaryingField : public cleaver::FloatField
{
public:
    SphereVaryingField(const SphereField* field);


    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    virtual cleaver::BoundingBox bounds() const;

private:
    const SphereField *m_sphereField;
};

#endif // SPHEREVARYINGFIELD_H
