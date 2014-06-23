#ifndef TRACKBALLCAMERA_H
#define TRACKBALLCAMERA_H

#include "Camera.h"
#include <QQuaternion>
#include <QVector2D>
#include <Cleaver/vec3.h>
#include <Cleaver/BoundingBox.h>

class TrackballCamera : public Camera
{
public:
    TrackballCamera();



    virtual cleaver::vec3 e() const;   // eye location
    virtual cleaver::vec3 t() const;   // target location
    virtual cleaver::vec3 u() const;   // up vector
    virtual cleaver::vec3 s() const;   // scale vector

    virtual float* viewMatrix(); // return viewMatrix in Col-Major

    virtual void reset();
    virtual void pan(float dx, float dy);
    virtual void rotate(float theta, float phi);
    virtual void zoom(float dz);
    void rotateBetween(const QVector2D &s1, const QVector2D &s2);

    void setView(const cleaver::vec3 &eye, const cleaver::vec3 &target);
    void setTargetBounds(const cleaver::BoundingBox &bounds);

    void setBallSize(int w, int h);

    QQuaternion rot();

private:

    void computeViewMatrix();
    QVector3D screenToBall(const QVector2D &s);

    QVector3D m_eye;     // eye location
    QVector3D m_target;  // target location
    QVector3D m_up;      // up direction
    QVector3D m_right;   // right direction
    QVector3D m_viewDir;     // view direction
    cleaver::BoundingBox m_targetBounds; // target bounds

    float m_scale;
    float m_viewMatrix[16];

    int m_width;
    int m_height;

    QQuaternion m_orientation;
};

#endif // TRACKBALLCAMERA_H
