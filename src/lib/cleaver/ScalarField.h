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

#ifndef SCALARFIELD_H
#define SCALARFIELD_H

#include "AbstractScalarField.h"
#include "BoundingBox.h"
#include "vec3.h"

namespace cleaver
{

enum CenteringType { NodeCentered, CellCentered };

template <typename T>
class ScalarField : public AbstractScalarField
{
public:

public:
    ScalarField(T *data = 0, int w=0, int h=0, int d=0);
    ~ScalarField();

    virtual double valueAt(const vec3 &x) const;
    virtual double valueAt(double x, double y, double z) const;

    virtual double tricubicValueAt(const vec3 &x) const;
    virtual double tricubicValueAt(double x, double y, double z) const;

    virtual vec3 tricubicGradientAt(const vec3 &x) const;
    virtual vec3 tricubicGradientAt(double x, double y, double z) const;

    virtual double cachedTricubicValueAt(const vec3 &x) const;
    virtual double cachedTricubicValueAt(double x, double y, double z) const;

    virtual vec3  cachedTricubicGradientAt(const vec3 &x) const;
    virtual vec3  cachedTricubicGradientAt(double x, double y, double z) const;


    void setData(T *data);
    T* data() const;
    T& data(int i, int j, int k) const;

    void setCenterType(CenteringType center);
    CenteringType getCenterType() const;    

    void setBounds(const BoundingBox &bounds);
    virtual BoundingBox bounds() const;

    BoundingBox dataBounds() const;

    void setScale(const vec3 &scale);
    const vec3& scale() const;

    virtual vec3 gradientAt(double x, double y, double z) const;
    virtual vec3 gradientAt(const vec3 &x) const;

    virtual vec3 FD_central_gradientAt(double x, double y, double z) const;
    virtual vec3 FD_central_gradientAt(const vec3 &x) const;

    virtual double convolutionTricubicValueAt(const vec3 &x) const;
    virtual double convolutionTricubicValueAt(double x, double y, double z) const;
    virtual double convolutionBicubicValueAt(double x, double y, double z) const;
    virtual double convolutionCubicValueAt(double x, double y, double z) const;
    virtual double convolutionInterpolate(double P[], double t) const;

    virtual double convolve(int m, double x) const;
    virtual double dconvolve(int m, double x) const;
    virtual double convolutionValueAt(double x, double y, double z) const;
    virtual double convolutionValueAt(const vec3 &x) const;
    virtual vec3 convolutionGradientAt(double x, double y, double z) const;
    virtual vec3 convolutionGradientAt(const vec3 &x) const;

private:

    CenteringType m_centeringType;
    vec3 m_scale;                   // spatial scaling
    vec3 m_scaleInv;                // inverse scaling
    BoundingBox m_bounds;           // spatial dimensions
    int m_w, m_h, m_d;              // data dimensions
    T *m_data;                      // data

    static CenteringType DefaultCenteringType;
};

typedef ScalarField<float>  FloatField;
typedef ScalarField<double> DoubleField;

}

#endif // SCALARFIELD_H
