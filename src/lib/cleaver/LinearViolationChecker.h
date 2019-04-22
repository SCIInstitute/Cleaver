//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Linear Violation Checker
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

#ifndef LINEAR_VIOLATION_CHECKER_H_
#define LINEAR_VIOLATION_CHECKER_H_

#include "ViolationChecker.h"
#include "HalfEdge.h"
#include "HalfFace.h"
#include "TetMesh.h"
#include "Tet.h"

namespace cleaver
{

class LinearViolationChecker : public ViolationChecker
{
  public:
    LinearViolationChecker(TetMesh *mesh);

    virtual void checkIfCutViolatesVertices(HalfEdge *edge);
    virtual void checkIfTripleViolatesVertices(HalfFace *face);
    virtual void checkIfQuadrupleViolatesVertices(Tet *tet);
    virtual void checkIfTripleViolatesEdges(HalfFace *face);
    virtual void checkIfQuadrupleViolatesEdges(Tet *tet);
    virtual void checkIfQuadrupleViolatesFaces(Tet *tet);

  private:
    TetMesh *m_mesh;
};

}

#endif // LINEAR_VIOLATION_CHECKER_H_
