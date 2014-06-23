#ifndef PLANEFIELD_H
#define PLANEFIELD_H

#include <Cleaver/ScalarField.h>
#include <Cleaver/BoundingBox.h>


class PlaneField : public cleaver::FloatField
{
public:
    PlaneField(double a, double b, double c, double d);
    PlaneField(const cleaver::vec3 &n, double d);
    PlaneField(const cleaver::vec3 &n, const cleaver::vec3 &p);
    PlaneField(const cleaver::vec3 &p1, const cleaver::vec3 &p2, const cleaver::vec3 &p3);

    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    void setBounds(const cleaver::BoundingBox &bounds);
    virtual cleaver::BoundingBox bounds() const;

private:
    cleaver::BoundingBox m_bounds;
    cleaver::vec3 n;
    double d;
};

#endif // PLANEFIELD_H
