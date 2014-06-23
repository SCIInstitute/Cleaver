#include "Matrix3x3.h"
#include <string>
#include <cstring>
#include <sstream>
#include <assert.h>

namespace cleaver
{

Matrix3x3::Matrix3x3()
{
    memset(m_d, 0, 9*sizeof(double));
}

Matrix3x3::Matrix3x3(const vec3 &a, const vec3 &b, const vec3 &c, VectorType type)
{
    if(type == Column){
        m_d[0] = a.x;  m_d[1] = b.x;  m_d[2] = c.x;
        m_d[3] = a.y;  m_d[4] = b.y;  m_d[5] = c.y;
        m_d[6] = a.z;  m_d[7] = b.z;  m_d[8] = c.z;
    }
    else if(type == Row){
        m_d[0] = a.x;  m_d[1] = a.y;  m_d[2] = a.z;
        m_d[3] = b.x;  m_d[4] = b.y;  m_d[5] = b.z;
        m_d[6] = c.x;  m_d[7] = c.y;  m_d[8] = c.z;
    }
    else{
        memset(m_d, 0, 9*sizeof(double));
    }
}

Matrix3x3::Matrix3x3(double a, double b, double c,
                  double d, double e, double f,
                  double g, double h, double i)
{
    m_d[0] = a;
    m_d[1] = b;
    m_d[2] = c;
    m_d[3] = d;
    m_d[4] = e;
    m_d[5] = f;
    m_d[6] = g;
    m_d[7] = h;
    m_d[8] = i;
}

double& Matrix3x3::operator()(int row, int column)
{
    assert(row >= 0 && row <= 2 && column >= 0 && column <= 2);

    return m_d[3*row + column];
}

double Matrix3x3::operator()(int row, int column) const
{
    assert(row >= 0 && row <= 2 && column >= 0 && column <= 2);

    return m_d[3*row + column];
}

Matrix3x3 Matrix3x3::inverse() const
{
    Matrix3x3 inv;

    const Matrix3x3 &A = *this;

    double determinant =    +A(0,0)*(A(1,1)*A(2,2)-A(2,1)*A(1,2))
                            -A(0,1)*(A(1,0)*A(2,2)-A(1,2)*A(2,0))
                            +A(0,2)*(A(1,0)*A(2,1)-A(1,1)*A(2,0));
    double invdet = 1/determinant;
    inv(0,0) =  (A(1,1)*A(2,2)-A(2,1)*A(1,2))*invdet;
    inv(0,1) = -(A(0,1)*A(2,2)-A(0,2)*A(2,1))*invdet;
    inv(0,2) =  (A(0,1)*A(1,2)-A(0,2)*A(1,1))*invdet;

    inv(1,0) = -(A(1,0)*A(2,2)-A(1,2)*A(2,0))*invdet;
    inv(1,1) =  (A(0,0)*A(2,2)-A(0,2)*A(2,0))*invdet;
    inv(1,2) = -(A(0,0)*A(1,2)-A(1,0)*A(0,2))*invdet;

    inv(2,0) =  (A(1,0)*A(2,1)-A(2,0)*A(1,1))*invdet;
    inv(2,1) = -(A(0,0)*A(2,1)-A(2,0)*A(0,1))*invdet;
    inv(2,2) =  (A(0,0)*A(1,1)-A(1,0)*A(0,1))*invdet;

    return inv;
}

Matrix3x3 Matrix3x3::transpose() const
{
    const Matrix3x3 &A = *this;
    return Matrix3x3(A.row(0), A.row(1), A.row(2));
}

vec3 Matrix3x3::row(int row) const
{
    assert(row >= 0 && row <= 2);

    return vec3(3*row + 0, 3*row + 1, 3*row + 2);
}

vec3 Matrix3x3::column(int column) const
{
    assert(column >= 0 && column <= 2);

    return vec3(0 + column, 3 + column, 6 + column);
}

Matrix3x3 Matrix3x3::Identity()
{
    return Matrix3x3(vec3::unitX, vec3::unitY, vec3::unitZ);
}

Matrix3x3& Matrix3x3::operator=(const Matrix3x3 &B)
{
    for(int i=0; i < 3; i++){
        for(int j=0; j < 3; j++){
            this->operator()(i,j) = B(i,j);
        }
    }

    return *this;
}

vec3 operator*(const Matrix3x3 &A, const vec3 &x)
{
    return vec3(dot(A.row(0), x),
                dot(A.row(1), x),
                dot(A.row(2), x));
}

Matrix3x3 operator*(const Matrix3x3 &A, const Matrix3x3 &B)
{
    Matrix3x3 R;

    for(int i=0; i < 3; i++){
        for(int j=0; j < 3; j++){
            for(int k=0; k < 3; k++){
                R(i,j) = R(i,j) + A(i,k) * B(k,j);
            }
        }
    }

    return R;
}

Matrix3x3 operator+(const Matrix3x3 &A, const Matrix3x3 &B)
{
    Matrix3x3 R;

    for(int i=0; i < 3; i++){
        for(int j=0; j < 3; j++){
            R(i,j) = A(i,j) + B(i,j);
        }
    }

    return R;
}

Matrix3x3 operator-(const Matrix3x3 &A, const Matrix3x3 &B)
{
    Matrix3x3 R;

    for(int i=0; i < 3; i++){
        for(int j=0; j < 3; j++){
            R(i,j) = A(i,j) - B(i,j);
        }
    }

    return R;
}

} // namespace Cleaver
