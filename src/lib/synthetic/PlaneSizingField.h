#ifndef PLANESIZINGFIELD_H
#define PLANESIZINGFIELD_H

#include <cleaver/ScalarField.h>
#include "PlaneField.h"

class PlaneSizingField : public cleaver::FloatField
{
public:
    PlaneSizingField(const PlaneField *field);

    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    virtual cleaver::BoundingBox bounds() const;

private:
    const PlaneField *m_planeField;
};
#endif // PLANESIZINGFIELD_H
