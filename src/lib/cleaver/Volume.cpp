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

#include "Volume.h"
#include "BoundingBox.h"

namespace cleaver
{

Volume::Volume()
{
}

Volume::Volume(const cleaver::Volume &volume)
{
    this->m_bounds      = volume.m_bounds;
    this->m_sizingField = volume.m_sizingField;
    this->m_valueFields = volume.m_valueFields;
}

Volume::Volume(const std::vector<AbstractScalarField*> &fields, int width, int height, int depth) :
    m_valueFields(fields), m_sizingField(0), m_bounds(BoundingBox(vec3::zero, vec3(width, height, depth)))
{
    if(m_valueFields.size() > 0)
    {
        if(width == 0)
            width = (int) m_valueFields[0]->bounds().size.x;
        if(height == 0)
            height = (int) m_valueFields[0]->bounds().size.y;
        if(depth == 0)
            depth = (int) m_valueFields[0]->bounds().size.z;

        m_bounds = BoundingBox(vec3::zero, vec3(width, height, depth));

    }
}

Volume::Volume(const std::vector<AbstractScalarField*> &fields, vec3 &size) :
    m_valueFields(fields),m_sizingField(0),  m_bounds(BoundingBox(vec3::zero, size))
{
    if(m_valueFields.size() > 0)
    {
        if(size.x == 0)
            size.x = m_valueFields[0]->bounds().size.x;
        if(size.y == 0)
            size.y = m_valueFields[0]->bounds().size.y;
        if(size.z == 0)
            size.z = m_valueFields[0]->bounds().size.z;

        m_bounds = BoundingBox(vec3::zero, size);

    }
}

Volume& Volume::operator= (const Volume &volume)
{
    this->m_bounds      = volume.m_bounds;
    this->m_sizingField = volume.m_sizingField;
    this->m_valueFields = volume.m_valueFields;
    return *this;
}

void Volume::setSize(int width, int height, int depth)
{
    m_bounds.size = vec3(width, height, depth);
}

void Volume::setSizingField(AbstractScalarField *field)
{
    this->m_sizingField = field;
}

AbstractScalarField* Volume::getSizingField() const
{
    return m_sizingField;
}

void Volume::setName(const std::string &name)
{
    m_name = name;
}

std::string Volume::name() const
{
    return m_name;
}

int Volume::maxAt(float x, float y, float z) const
{
    return maxAt(vec3(x,y,z));
}

int Volume::maxAt(const vec3 &x) const
{
    double maxValue = valueAt(x,0);
    unsigned int   maxLabel = 0;

    for(int i=1; i < numberOfMaterials(); i++)
    {
        double value = valueAt(x,i);
        if(value > maxValue)
        {
            maxValue = value;
            maxLabel = i;
        }
    }

    return maxLabel;
}

double Volume::valueAt(const vec3 &x, int material) const
{
    vec3 tx = vec3((x.x / m_bounds.size.x)*m_valueFields[material]->bounds().size.x,
                   (x.y / m_bounds.size.y)*m_valueFields[material]->bounds().size.y,
                   (x.z / m_bounds.size.z)*m_valueFields[material]->bounds().size.z);
    return m_valueFields[material]->valueAt(tx);
}

double Volume::valueAt(double x, double y, double z, int material) const
{
    vec3 tx = vec3((x / m_bounds.size.x)*m_valueFields[material]->bounds().size.x,
                   (y / m_bounds.size.y)*m_valueFields[material]->bounds().size.y,
                   (z / m_bounds.size.z)*m_valueFields[material]->bounds().size.z);
    return m_valueFields[material]->valueAt(tx);
}

int Volume::numberOfMaterials() const
{
    return static_cast<int>(m_valueFields.size());
}

const BoundingBox& Volume::bounds() const
{
    return m_bounds;
}

void Volume::addMaterial(AbstractScalarField *field)
{
    // check that it's not already present
    for(size_t m=0; m < m_valueFields.size(); m++)
    {
        if(m_valueFields[m] == field)
            return;
    }

    // otherwise, add it
    m_valueFields.push_back(field);
}

void Volume::removeMaterial(AbstractScalarField *field)
{
    std::vector<cleaver::AbstractScalarField*>::iterator iter;

    for(iter = m_valueFields.begin(); iter != m_valueFields.end(); iter++)
    {
        if(*iter == field)
        {
            iter = m_valueFields.erase(iter);
            if(iter == m_valueFields.end())
                break;
        }
    }
}

}
