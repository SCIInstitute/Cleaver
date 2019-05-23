#ifndef TORUSFIELD_H
#define TORUSFIELD_H

#include <cleaver/ScalarField.h>
#include <cleaver/BoundingBox.h>
#include <vector>

class TorusField : public cleaver::FloatField
{
public:
    TorusField(const cleaver::vec3 &cx, float ur, float vr, const cleaver::BoundingBox &bounds);

    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    void setBounds(const cleaver::BoundingBox &bounds);
    virtual cleaver::BoundingBox bounds() const;

    std::vector<cleaver::vec3>  tensorAt(const cleaver::vec3 &x) const;


private:
    cleaver::BoundingBox m_bounds;
    cleaver::vec3 m_cx;             // center
    float m_ur;                     // minor radius
    float m_vr;                     // minor radius
};

#endif // TORUSFIELD_H
