#ifndef SPHERESIZINGFIELD_H
#define SPHERESIZINGFIELD_H

#include <cleaver/ScalarField.h>
#include "SphereField.h"

class SphereSizingField : public cleaver::FloatField
{
public:
    SphereSizingField(const SphereField* field);

    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    virtual cleaver::BoundingBox bounds() const;

private:
    const SphereField *m_sphereField;
};

#endif // SPHERESIZINGFIELD_H
