#ifndef BLOBBYFIELD_H
#define BLOBBYFIELD_H

#include <Cleaver/ScalarField.h>
#include <Cleaver/BoundingBox.h>
#include <vector>

#include "SphereField.h"

class BlobbyField : public cleaver::FloatField
{
public:
    BlobbyField(const cleaver::vec3 &cx, float r, int nspheres, const cleaver::BoundingBox &bounds);
    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;

    void setBounds(const cleaver::BoundingBox &bounds);
    virtual cleaver::BoundingBox bounds() const;
    float meta(float r) const;
    std::vector<cleaver::vec3> spheres;
    std::vector<float> rad;
    float a,b,c;
private:
    cleaver::BoundingBox m_bounds;
    cleaver::vec3 m_cx;
    float m_r;
};

#endif // SPHEREFIELD_H
