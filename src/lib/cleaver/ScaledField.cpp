//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
// -- Inverse Field Class
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
//---------------------------------------------------------------

#include "ScaledField.h"

namespace cleaver
{

template <typename T>
ScaledField<T>::ScaledField(const ScalarField<T> *field, float scale) : m_field(field), m_scale(scale)
{
}

template <typename T>
void ScaledField<T>::setField(const ScalarField<T> *field)
{
    m_field = field;
}

template <typename T>
void ScaledField<T>::setScale(float scale)
{
    m_scale = scale;
}

template <typename T>
float ScaledField<T>::valueAt(float x, float y, float z) const
{
    return m_scale*m_field->valueAt(x,y,z);
}

template <typename T>
float ScaledField<T>::valueAt(const vec3 &x) const
{
    return m_scale*m_field->valueAt(x);
}

template <typename T>
BoundingBox ScaledField<T>::bounds() const
{
    return m_field->bounds();
}

}
