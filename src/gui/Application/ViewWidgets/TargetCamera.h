#ifndef TARGETCAMERA_H
#define TARGETCAMERA_H

#include "Camera.h"
#include <Cleaver/vec3.h>
#include <Cleaver/BoundingBox.h>

class TargetCamera : public Camera
{
public:
    TargetCamera();

    virtual cleaver::vec3 e() const;   // eye location
    virtual cleaver::vec3 t() const;   // target location
    virtual cleaver::vec3 u() const;   // up vector
    virtual cleaver::vec3 s() const;   // scale vector

    virtual float* viewMatrix(); // return viewMatrix in Col-Major

    virtual void reset();
    virtual void pan(float dx, float dy);
    virtual void rotate(float theta, float phi);
    virtual void zoom(float dz);

    void setView(const cleaver::vec3 &eye, const cleaver::vec3 &target);
    void setTargetBounds(const cleaver::BoundingBox &bounds);

private:

    void computeViewMatrix();

    cleaver::vec3 m_e;       // eye location
    cleaver::vec3 m_t;       // target location
    cleaver::vec3 m_u;       // up direction
    cleaver::vec3 m_r;       // right direction
    cleaver::vec3 m_viewDir; // view direction
    cleaver::BoundingBox m_targetBounds; // target bounds

    float m_scale;
    float m_viewMatrix[16];
};

#endif // TARGETCAMERA_H
