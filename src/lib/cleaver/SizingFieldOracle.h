//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
// -- SizingFieldOracle
//
//  Author: Jonathan Bronson (bronson@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2011, 2012, 2013 Jonathan Bronson
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


#ifndef SIZINGFIELDORACLE_H
#define SIZINGFIELDORACLE_H

#include "ScalarField.h"
#include "Octree.h"

namespace cleaver
{

class SizingFieldOracle
{
public:
    SizingFieldOracle(const AbstractScalarField *sizingField = nullptr, const BoundingBox &bounds = BoundingBox());

    void setSizingField(const AbstractScalarField *sizingField);
    void setBoundingBox(const BoundingBox &bounds);
    void createOctree();

    double getMinLFS(int xLocCode, int yLocCode, int zLocCode, int level) const;

    enum ConstructionType { Fast, Accurate };

protected:

    void sanityTest1(); // test for self-consistency
    void sanityTest2(); // test against sizing field

    double adaptCell(OTCell *cell);
    void printTree(OTCell *myCell, int n);


    const AbstractScalarField *m_sizingField;
    BoundingBox          m_bounds;
    Octree              *m_tree;

    ConstructionType m_constructionType;
};

}

#endif // SIZINGFIELDORACLE_H
