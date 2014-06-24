//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Tetrahedral Mesher
// -- 3D Float Point Data Field
//
//  Author: Jonathan Bronson (bronson@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2011, 2012, Jonathan Bronson
//  Scientific Computing & Imaging Institute
//  University of Utah
//
//  Permission is  hereby  granted, free  of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files  ( the "Software" ),  to  deal in  the  Software without
//  restriction, including  without limitation the rights to  use,
//  copy, modify,  merge, publish, distribute, sublicense,  and/or
//  sell copies of the Software, and to permit persons to whom the
//  Software is  furnished  to do  so,  subject  to  the following
//  conditions:
//
//  The above  copyright notice  and  this permission notice shall
//  be included  in  all copies  or  substantial  portions  of the
//  Software.
//
//  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
//  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
//  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
//  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//  USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#include <math.h>
#include "ScalarField.h"
#include "vec3.h"


namespace cleaver
{

template <typename T>
cleaver::CenteringType ScalarField<T>::DefaultCenteringType = cleaver::CellCentered;

template <typename T>
ScalarField<T>::ScalarField(T *data, int w, int h, int d)
    : m_w(w), m_h(h), m_d(d), m_data(data)
{
    // default to data bounds
    m_scale = vec3(vec3::unitX.x, vec3::unitY.y, vec3::unitZ.z);
    m_scaleInv = m_scale;

    m_centeringType = ScalarField<T>::DefaultCenteringType;
    m_bounds = dataBounds();
}

template <typename T>
ScalarField<T>::~ScalarField()
{
    // no memory cleanup
}

template <typename T>
double ScalarField<T>::valueAt(double x, double y, double z) const
{
    x = (x - m_bounds.origin.x)*m_scaleInv.x;
    y = (y - m_bounds.origin.y)*m_scaleInv.y;
    z = (z - m_bounds.origin.z)*m_scaleInv.z;

    if(m_centeringType == CellCentered){
        x -= 0.5f;
        y -= 0.5f;
        z -= 0.5f;
    }

    double t = fmod(x,1.0);
    double u = fmod(y,1.0);
    double v = fmod(z,1.0);

    int i0 = (int)floor(x);   int i1 = i0+1;
    int j0 = (int)floor(y);   int j1 = j0+1;
    int k0 = (int)floor(z);   int k1 = k0+1;


    if(m_centeringType == CellCentered)
    {
        i0 = clamp(i0, 0, m_w-1);
        j0 = clamp(j0, 0, m_h-1);
        k0 = clamp(k0, 0, m_d-1);

        i1 = clamp(i1, 0, m_w-1);
        j1 = clamp(j1, 0, m_h-1);
        k1 = clamp(k1, 0, m_d-1);
    }
    else if(m_centeringType == NodeCentered)
    {
        i0 = clamp(i0, 0, m_w-2);
        j0 = clamp(j0, 0, m_h-2);
        k0 = clamp(k0, 0, m_d-2);

        i1 = clamp(i1, 0, m_w-2);
        j1 = clamp(j1, 0, m_h-2);
        k1 = clamp(k1, 0, m_d-2);
    }


    double C000 = m_data[i0 + j0*m_w + k0*m_w*m_h];
    double C001 = m_data[i0 + j0*m_w + k1*m_w*m_h];
    double C010 = m_data[i0 + j1*m_w + k0*m_w*m_h];
    double C011 = m_data[i0 + j1*m_w + k1*m_w*m_h];
    double C100 = m_data[i1 + j0*m_w + k0*m_w*m_h];
    double C101 = m_data[i1 + j0*m_w + k1*m_w*m_h];
    double C110 = m_data[i1 + j1*m_w + k0*m_w*m_h];
    double C111 = m_data[i1 + j1*m_w + k1*m_w*m_h];

    return double((1-t)*(1-u)*(1-v)*C000 + (1-t)*(1-u)*(v)*C001 +
                 (1-t)*  (u)*(1-v)*C010 + (1-t)*  (u)*(v)*C011 +
                   (t)*(1-u)*(1-v)*C100 +   (t)*(1-u)*(v)*C101 +
                   (t)*  (u)*(1-v)*C110 +   (t)*  (u)*(v)*C111);
}

template <typename T>
double ScalarField<T>::valueAt(const vec3 &x) const
{
    return valueAt((double)x.x,(double)x.y,(double)x.z);
}

template <typename T>
T* ScalarField<T>::data() const
{
    return m_data;
}

template <typename T>
T& ScalarField<T>::data(int i, int j, int k) const
{
    BoundingBox bounds = dataBounds();
    int w = (int)bounds.size.x;
    int h = (int)bounds.size.y;
    int d = (int)bounds.size.z;

    return m_data[k*w*h + j*w + i];
}


template <typename T>
void ScalarField<T>::setBounds(const BoundingBox &bounds)
{
    m_bounds = bounds;
}

template <typename T>
BoundingBox ScalarField<T>::bounds() const
{
    return m_bounds;
}

template <typename T>
BoundingBox ScalarField<T>::dataBounds() const
{
    switch(m_centeringType){
        case CellCentered:
            return BoundingBox(vec3::zero, vec3(m_w, m_h, m_d));
            break;

        case NodeCentered:
            return BoundingBox(vec3::zero, vec3(m_w-1, m_h-1, m_d-1));
            break;

        default:
            std::cerr << "Bad CenterType Defined, Returning empty BoundingBox" << std::endl;
            return BoundingBox(vec3::zero, vec3::zero);
            break;
    }
}

template <typename T>
void ScalarField<T>::setScale(const vec3 &scale)
{
    m_scale = scale;
    m_scaleInv = vec3(1.0/scale.x, 1.0/scale.y, 1.0/scale.z);
    m_bounds.origin = vec3(m_bounds.origin.x*m_scale.x,
                           m_bounds.origin.y*m_scale.y,
                           m_bounds.origin.z*m_scale.z);
    m_bounds.size =    vec3(m_bounds.size.x*m_scale.x,
                            m_bounds.size.y*m_scale.y,
                            m_bounds.size.z*m_scale.z);
}

template <typename T>
const vec3& ScalarField<T>::scale() const
{
    return m_scale;
}

template <typename T>
void ScalarField<T>::setCenterType(CenteringType center)
{
    m_centeringType = center;
}

template <typename T>
CenteringType ScalarField<T>::getCenterType() const
{
    return m_centeringType;
}

template <typename T>
void ScalarField<T>::setData(T *data)
{
    m_data = data;
}


// explicit instantion of acceptable types
// TODO: find way to hide implementation but allow aribtrary instantion
template class ScalarField<char>;
template class ScalarField<int>;
template class ScalarField<float>;
template class ScalarField<double>;
template class ScalarField<unsigned char>;
template class ScalarField<unsigned int>;

template class ScalarField<long int>;
template class ScalarField<long double>;

}
