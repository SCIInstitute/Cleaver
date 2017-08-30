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

#include "SimpleInterfaceCalculator.h"

#include "Plane.h"
#include <cmath>

namespace cleaver
{

SimpleInterfaceCalculator::SimpleInterfaceCalculator(
    TetMesh *mesh, AbstractVolume *volume) : m_mesh(mesh), m_volume(volume) {}


void SimpleInterfaceCalculator::computeCutForEdge(HalfEdge *edge) {
  // order verts
  Vertex *v2 = edge->vertex;
  Vertex *v1 = edge->mate->vertex;

  // set as evaluated
  edge->evaluated = true;
  edge->mate->evaluated = true;

  // do labels differ?
  if (v1->label == v2->label)
    return;

  Vertex *cut = new Vertex(m_volume->numberOfMaterials());
  cut->pos() = 0.5*v1->pos() + 0.5*v2->pos();
   
  // doesn't really matter which
  cut->label = v1->label;
  cut->lbls[v1->label] = true;
  cut->lbls[v2->label] = true;

  // should never be in violation
  cut->violating = false;
  cut->closestGeometry = nullptr;

  edge->cut = cut;
  edge->mate->cut = cut;
  cut->order() = Order::CUT;
}


void SimpleInterfaceCalculator::computeTripleForFace(HalfFace *face) {
  // set as evaluated
  face->evaluated = true;
  if (face->mate)
    face->mate->evaluated = true;

  // only continue if 3 cuts exist
  if (!face->halfEdges[0]->cut || !face->halfEdges[1]->cut || !face->halfEdges[2]->cut)
    return;

  Vertex *verts[3];
  HalfEdge *edges[3];

  m_mesh->getAdjacencyListsForFace(face, verts, edges);
  Vertex *v1 = verts[0];
  Vertex *v2 = verts[1];
  Vertex *v3 = verts[2];
  
  //-------------------------------------------------------
  // Create the Triple Vertex
  //-------------------------------------------------------
  Vertex *triple = new Vertex(m_volume->numberOfMaterials());
  triple->pos() = (1.0 / 3.0)*(v1->pos() + v2->pos() + v3->pos());
  triple->lbls[v1->label] = true;
  triple->lbls[v2->label] = true;
  triple->lbls[v3->label] = true;
  triple->label = std::min(v1->label, v2->label);
  triple->order() = Order::TRIP;
  triple->violating = false;
  triple->closestGeometry = nullptr;
  face->triple = triple;
  if (face->mate)
    face->mate->triple = triple;
}


void SimpleInterfaceCalculator::computeQuadrupleForTet(Tet *tet) {
  // set as evaluated
  tet->evaluated = true;

  Vertex *verts[4];
  HalfEdge *edges[6];
  HalfFace *faces[4];

  m_mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

  // only continue if 6 cuts exist
  for (int e = 0; e < 6; e++) {
    if (!edges[e]->cut)
      return;
  }

  Vertex *quadruple = new Vertex(m_volume->numberOfMaterials());
  Vertex *v1 = verts[0];
  Vertex *v2 = verts[1];
  Vertex *v3 = verts[2];
  Vertex *v4 = verts[3];
  quadruple->pos() = (1.0 / 4.0)*(v1->pos() + v2->pos() + v3->pos() + v4->pos());
  quadruple->lbls[v1->label] = true;
  quadruple->lbls[v2->label] = true;
  quadruple->lbls[v3->label] = true;
  quadruple->lbls[v4->label] = true;
  quadruple->label = std::min(v1->label, v2->label);
  tet->quadruple = quadruple;
  tet->quadruple->violating = false;
}

} // namespace cleaver
