//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
// -- Constant Field Class
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


#ifndef CONSTANTFIELD_H
#define CONSTANTFIELD_H


#include "ScalarField.h"
#include "BoundingBox.h"

namespace cleaver
{

template <typename T>
class ConstantField : public ScalarField<T>
{
public:
    ConstantField(T value, const BoundingBox &bounds);
    ConstantField(T value, int w, int h, int d);
    void setValue(T value);
    virtual double valueAt(double x, double y, double z) const;
    virtual double valueAt(const cleaver::vec3 &x) const;
    virtual cleaver::BoundingBox bounds() const;
private:
    T m_value;
    int m_w, m_h, m_d;
    BoundingBox m_bounds;
};

typedef ConstantField<float>  ConstantFloatField;
typedef ConstantField<double> ConstantDoubleField;

}

#endif // CONSTANTFIELD_H
