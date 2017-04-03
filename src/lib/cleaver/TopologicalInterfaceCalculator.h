//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Topological Interface Calculator
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

#ifndef TOPOLOGICAL_INTERFACE_CALCULATOR_H
#define TOPOLOGICAL_INTERFACE_CALCULATOR_H

#include "InterfaceCalculator.h"
#include "TetMesh.h"
#include "AbstractVolume.h"
#include "vec3.h"
#include "Vertex.h"

namespace cleaver
{

class TopologicalInterfaceCalculator : public InterfaceCalculator
{
  public:
    TopologicalInterfaceCalculator(TetMesh *mesh, AbstractVolume *volume);
    virtual void computeCutForEdge(HalfEdge *edge);
    virtual void computeTripleForFace(HalfFace *face);
    virtual void computeQuadrupleForTet(Tet *tet);

  private:
    TetMesh *m_mesh;
    AbstractVolume *m_volume;

    bool planeIntersect(Vertex *v1, Vertex *v2, Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, float epsilon = 1E-4);
    void forcePointIntoTriangle(vec3 a, vec3 b, vec3 c, vec3 &p);
    void computeLagrangePolynomial(const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4, double coefficients[4]);
};

} // namespace cleaver

#endif // LINEAR_INTERFACE_CALCULATOR_H
