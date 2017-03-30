//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Volume Class
//
// Author: Jonathan Bronson (bronson@sci.utah.ed)
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

#ifndef VOLUME_H
#define VOLUME_H

#include <vector>
#include <string>
#include "vec3.h"
#include "ScalarField.h"
#include "BoundingBox.h"
#include "AbstractVolume.h"

namespace cleaver
{

class Volume : public AbstractVolume
{
public:
    Volume();
    Volume(const Volume &volume);
    Volume(const std::vector<AbstractScalarField*> &fields, int width=0, int height=0, int depth=0);
    Volume(const std::vector<AbstractScalarField*> &fields, vec3 &size);
    Volume& operator= (const Volume &volume);

    void setName(const std::string &name);
    virtual std::string name() const;
    virtual double valueAt(const vec3 &x, int material) const;
    virtual double valueAt(double x, double y, double z, int material) const;
    virtual int maxAt(float x, float y, float z) const;
    virtual int maxAt(const vec3 &x) const;
    virtual int numberOfMaterials() const;
    virtual const BoundingBox& bounds() const;

    float lfsAt(const vec3 &x) const;
    float lfsAt(float x, float y, float z) const;

    void setSize(int width, int height, int depth);
    void setSizingField(AbstractScalarField *field);

    AbstractScalarField* getSizingField() const;
    AbstractScalarField* getMaterial(int i) const { return m_valueFields[i]; }

    void addMaterial(AbstractScalarField *field);
    void removeMaterial(AbstractScalarField *field);

private:
    std::string m_name;
    std::vector<AbstractScalarField*> m_valueFields;
    AbstractScalarField* m_sizingField;
    cleaver::BoundingBox m_bounds;
};

}

#endif // VOLUME_H
