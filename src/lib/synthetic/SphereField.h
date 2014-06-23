#ifndef SPHEREFIELD_H
#define SPHEREFIELD_H

#include <Cleaver/ScalarField.h>
#include <Cleaver/BoundingBox.h>

class SphereSizingField;

class SphereField : public cleaver::FloatField
{
public:
    SphereField(const cleaver::vec3 &cx, float r, const cleaver::BoundingBox &bounds);

    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    void setBounds(const cleaver::BoundingBox &bounds);
    virtual cleaver::BoundingBox bounds() const;

    friend class SphereSizingField;
    friend class SphereVaryingField;

private:
    cleaver::BoundingBox m_bounds;
    cleaver::vec3 m_cx;
    float m_r;
};

#endif // SPHEREFIELD_H
