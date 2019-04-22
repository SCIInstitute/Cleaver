#ifndef MATRIX3X3_H
#define MATRIX3X3_H

#include "vec3.h"

namespace cleaver
{

class Matrix3x3
{
public:
    enum VectorType { Column, Row };

public:
    Matrix3x3();
    Matrix3x3(const vec3 &a, const vec3 &b, const vec3 &c, VectorType type = Column );
    Matrix3x3(double a, double b, double c,
              double d, double e, double f,
              double g, double h, double i);

    double& operator()(int row, int column);
    double operator()(int row, int column) const;

    Matrix3x3 inverse() const;
    Matrix3x3 transpose() const;

    vec3 row(int row) const;
    vec3 column(int column) const;

    static Matrix3x3 Identity();

    Matrix3x3& operator=(const Matrix3x3 &b);

private:
    double m_d[9];
};

vec3      operator*(const Matrix3x3 &A, const vec3 &x);
Matrix3x3 operator*(const Matrix3x3 &A, const Matrix3x3 &B);
Matrix3x3 operator-(const Matrix3x3 &A, const Matrix3x3 &B);
Matrix3x3 operator+(const Matrix3x3 &A, const Matrix3x3 &B);


}

#endif // MATRIX3X3_H
