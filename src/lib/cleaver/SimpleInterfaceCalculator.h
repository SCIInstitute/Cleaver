//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Simple Interface Calculator
//
// Author: Jonathan Bronson (bronson@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2017 Jonathan Bronson
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

#ifndef SIMPLE_INTERFACE_CALCULATOR_H
#define SIMPLE_INTERFACE_CALCULATOR_H

#include "InterfaceCalculator.h"
#include "TetMesh.h"
#include "AbstractVolume.h"
#include "vec3.h"
#include "Vertex.h"

namespace cleaver
{

/**
 * This interface calculator avoids calculating interface positions
 * and instead always returns interfaces in the centers of each
 * k-cell of the graph. These interfaces will never be in violation.
 */
class SimpleInterfaceCalculator : public InterfaceCalculator
{
  public:
    SimpleInterfaceCalculator(TetMesh *mesh, AbstractVolume *volume);
    virtual void computeCutForEdge(HalfEdge *edge);
    virtual void computeTripleForFace(HalfFace *face);
    virtual void computeQuadrupleForTet(Tet *tet);

  private:
    TetMesh *m_mesh;
    AbstractVolume *m_volume;
};

}

#endif // SIMPLE_INTERFACE_CALCULATOR_H
