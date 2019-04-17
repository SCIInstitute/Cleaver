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

#include "LinearViolationChecker.h"
#include <cmath>

namespace cleaver
{

LinearViolationChecker::LinearViolationChecker(
    TetMesh *mesh) : m_mesh(mesh) {}

//===================================================================
// - checkIfCutViolatesVertices(HalfEdge *edge)
//===================================================================
void LinearViolationChecker::checkIfCutViolatesVertices(HalfEdge *edge)
{
  Vertex *cut = edge->cut;
  vec3 a = edge->mate->vertex->pos();
  vec3 b = edge->vertex->pos();
  vec3 c = cut->pos();

  double t = L2(c - a) / L2(b - a);

  // Check Violations
  if (t <= edge->alpha) {
    cut->violating = true;
    cut->closestGeometry = edge->mate->vertex;
  } else if (t >= (1 - edge->mate->alpha)) {
    cut->violating = true;
    cut->closestGeometry = edge->vertex;
  } else {
    cut->violating = false;
  }
}

//===============================================================
// - checkIfTripleViolatesVertices(HalfFace *face)
// This method generalizes the rules that dictate whether a cutpoint
// violates a lattice vertex. Similarly, we also want to know when a
// Triple Point violates such a vertex so that it can be included in
// the warping rules. The generalization follows by extending lines
// from the alpha points, to their opposite corners. The intersection
// of these two edges forms a region of violation for the triple point
// and a vertex. This test must be performed three times, once for each
// vertex in the triangle. Only one vertex can be violated at a time,
// and which vertex is violated is stored.
//===============================================================
void LinearViolationChecker::checkIfTripleViolatesVertices(HalfFace *face)
{
  HalfEdge *edges[EDGES_PER_FACE];
  Vertex *verts[VERTS_PER_FACE];
  Vertex *triple = face->triple;
  triple->violating = false;
  triple->closestGeometry = nullptr;

  m_mesh->getAdjacencyListsForFace(face, verts, edges);

  vec3 v0 = verts[0]->pos();
  vec3 v1 = verts[1]->pos();
  vec3 v2 = verts[2]->pos();
  vec3 trip = triple->pos();

  // check v0
  if (!triple->violating) {
    vec3 e1 = normalize(v0 - v2);       vec3 e2 = normalize(v0 - v1);
    vec3 t1 = normalize(trip - v2);     vec3 t2 = normalize(trip - v1);

    // WARNING: Old Method Normalized Alpha to new edge length: Consider replacing alpha with LENGTH
    double alpha1 = edges[2]->alphaForVertex(verts[0]);
    double alpha2 = edges[1]->alphaForVertex(verts[0]);

    vec3 c1 = v0*(1 - alpha1) + alpha1*v1;
    vec3 c2 = v0*(1 - alpha2) + alpha2*v2;

    c1 = normalize(c1 - v2);
    c2 = normalize(c2 - v1);

    if (dot(e1, t1) >= dot(e1, c1) &&
      dot(e2, t2) >= dot(e2, c2)) {
      triple->violating = true;
      triple->closestGeometry = verts[0];
      return;
    }
  }

  // check v1
  if (!triple->violating) {
    vec3 e1 = normalize(v1 - v0);       vec3 e2 = normalize(v1 - v2);
    vec3 t1 = normalize(trip - v0);     vec3 t2 = normalize(trip - v2);

    double alpha1 = edges[0]->alphaForVertex(verts[1]);
    double alpha2 = edges[2]->alphaForVertex(verts[1]);

    vec3 c1 = v1*(1 - alpha1) + alpha1*v2;
    vec3 c2 = v1*(1 - alpha2) + alpha2*v0;

    c1 = normalize(c1 - v0);
    c2 = normalize(c2 - v2);

    if (dot(e1, t1) >= dot(e1, c1) &&
      dot(e2, t2) >= dot(e2, c2)) {
      triple->violating = true;
      triple->closestGeometry = verts[1];
      return;
    }
  }

  // check v2
  if (!triple->violating) {
    vec3 e1 = normalize(v2 - v1);      vec3 e2 = normalize(v2 - v0);
    vec3 t1 = normalize(trip - v1);    vec3 t2 = normalize(trip - v0);

    double alpha1 = edges[1]->alphaForVertex(verts[2]);
    double alpha2 = edges[0]->alphaForVertex(verts[2]);

    vec3 c1 = v2*(1 - alpha1) + alpha1*v0;
    vec3 c2 = v2*(1 - alpha2) + alpha2*v1;

    c1 = normalize(c1 - v1);
    c2 = normalize(c2 - v0);

    if (dot(e1, t1) >= dot(e1, c1) &&
      dot(e2, t2) >= dot(e2, c2)) {
      triple->violating = true;
      triple->closestGeometry = verts[2];
      return;
    }
  }
}

//===============================================================
// - checkIfQuadrupleViolatesVertices(Tet *tet)
//
// This method generalizes the rules that dictate whether a triplepoint
// violates a lattice vertex. Similarly, we also want to know when a
// Quadruple Point violates such a vertex so that it can be included
// in the warping rules. The generalization follows by extending lines
// from the alpha points, to their opposite corners. The intersection
// of these three edges forms a region of violation for the quadruple
// point and a vertex. This test must be performed four times, once for
// each vertex in the Lattice Tet. Only one vertex can be violated at
// a time, and which vertex is violated is stored.
//===============================================================
void LinearViolationChecker::checkIfQuadrupleViolatesVertices(Tet *tet)
{
  // Return immediately if quadruple doesn't exist
  if (!tet->quadruple || tet->quadruple->order() != Order::QUAD)
    return;

  Vertex *quad = tet->quadruple;
  quad->violating = false;

  Vertex *verts[VERTS_PER_TET];
  HalfEdge *edges[EDGES_PER_TET];
  HalfFace *faces[FACES_PER_TET];

  m_mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

  vec3 v1 = verts[0]->pos();
  vec3 v2 = verts[1]->pos();
  vec3 v3 = verts[2]->pos();
  vec3 v4 = verts[3]->pos();
  vec3 q = quad->pos();

  // check v1 - using edges e1, e2, e3
  if (!quad->violating) {
    float t1 = edges[0]->alphaForVertex(verts[0]);
    vec3 ev1 = (1 - t1)*v1 + t1*v2;
    vec3  n1 = normalize(cross(v3 - ev1, v4 - ev1));
    vec3  q1 = q - ev1;
    double d1 = dot(n1, q1);

    float t2 = edges[1]->alphaForVertex(verts[0]);
    vec3 ev2 = (1 - t2)*v1 + t2*v3;
    vec3  n2 = normalize(cross(v4 - ev2, v2 - ev2));
    vec3  q2 = q - ev2;
    double d2 = dot(n2, q2);

    float t3 = edges[2]->alphaForVertex(verts[0]);
    vec3 ev3 = (1 - t3)*v1 + t3*v4;                            // edge violation crosspoint
    vec3  n3 = normalize(cross(v2 - ev3, v3 - ev3));           // normal to plane
    vec3  q3 = q - ev3;                                        // quadruple in locall coordinate fram
    double d3 = dot(n3, q3);                                   // distance from quad to plane

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quad->violating = true;
      quad->closestGeometry = verts[0];
    }
  }

  // check v2 - using edges e1(v1), e4(v3), e5(v4)
  if (!quad->violating) {
    float t1 = edges[0]->alphaForVertex(verts[1]);
    vec3 ev1 = (1 - t1)*v2 + t1*v1;
    vec3  n1 = normalize(cross(v4 - ev1, v3 - ev1));
    vec3  q1 = q - ev1;
    double d1 = dot(n1, q1);

    float t2 = edges[4]->alphaForVertex(verts[1]);
    vec3 ev2 = (1 - t2)*v2 + t2*v4;
    vec3  n2 = normalize(cross(v3 - ev2, v1 - ev2));
    vec3  q2 = q - ev2;
    double d2 = dot(n2, q2);

    float t3 = edges[3]->alphaForVertex(verts[1]);
    vec3 ev3 = (1 - t3)*v2 + t3*v3;
    vec3  n3 = normalize(cross(v1 - ev3, v4 - ev3));
    vec3  q3 = q - ev3;
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quad->violating = true;
      quad->closestGeometry = verts[1];
    }
  }

  // check v3 - using edges e2, e4, e6
  if (!quad->violating) {
    double t1 = edges[1]->alphaForVertex(verts[2]);
    vec3 ev1 = (1 - t1)*v3 + t1*v1;
    vec3  n1 = normalize(cross(v2 - ev1, v4 - ev1));
    vec3  q1 = q - ev1;
    double d1 = dot(n1, q1);

    double t2 = edges[5]->alphaForVertex(verts[2]);
    vec3 ev2 = (1 - t2)*v3 + t2*v4;
    vec3  n2 = normalize(cross(v1 - ev2, v2 - ev2));
    vec3  q2 = q - ev2;
    double d2 = dot(n2, q2);

    double t3 = edges[3]->alphaForVertex(verts[2]);
    vec3 ev3 = (1 - t3)*v3 + t3*v2;
    vec3  n3 = normalize(cross(v4 - ev3, v1 - ev3));
    vec3  q3 = q - ev3;
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quad->violating = true;
      quad->closestGeometry = verts[2];
    }
  }

  // check v4 - using edges e3, e5, e6
  if (!quad->violating) {
    double t1 = edges[2]->alphaForVertex(verts[3]);
    vec3 ev1 = (1 - t1)*v4 + t1*v1;
    vec3  n1 = normalize(cross(v3 - ev1, v2 - ev1));
    vec3  q1 = q - ev1;
    double d1 = dot(n1, q1);

    double t2 = edges[5]->alphaForVertex(verts[3]);
    vec3 ev2 = (1 - t2)*v4 + t2*v3;
    vec3  n2 = normalize(cross(v2 - ev2, v1 - ev2));
    vec3  q2 = q - ev2;
    double d2 = dot(n2, q2);

    double t3 = edges[4]->alphaForVertex(verts[3]);
    vec3 ev3 = (1 - t3)*v4 + t3*v2;
    vec3  n3 = normalize(cross(v1 - ev3, v3 - ev3));
    vec3  q3 = q - ev3;
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quad->violating = true;
      quad->closestGeometry = verts[3];
    }
  }
}

//======================================================================
//  checkIfTripleViolatesEdges()
//
//  This method checks whether a triple violates either of 3 edges
// that surround its face. This is used in the second phase of
// the warping algorithm.
//======================================================================
void LinearViolationChecker::checkIfTripleViolatesEdges(HalfFace *face)
{
  // Return immediately if triple doesn't exist
  if (!face->triple || face->triple->order() != Order::TRIP)
    return;

  Vertex *triple = face->triple;
  triple->violating = false;

  double d[3];
  double d_min = 100000;
  bool violating[3] = { 0 };

  Vertex   *verts[3];
  HalfEdge *edges[3];

  m_mesh->getAdjacencyListsForFace(face, verts, edges);

  Vertex *v1 = verts[0];
  Vertex *v2 = verts[1];
  Vertex *v3 = verts[2];

  vec3 p1 = verts[0]->pos();
  vec3 p2 = verts[1]->pos();
  vec3 p3 = verts[2]->pos();
  vec3 trip = triple->pos();

  // check violating edge0
  {
    vec3 e1 = normalize(p3 - p2);
    vec3 e2 = normalize(p2 - p3);
    vec3 t1 = normalize(trip - p2);
    vec3 t2 = normalize(trip - p3);

    double alpha1 = edges[1]->alphaForVertex(v3);
    double alpha2 = edges[2]->alphaForVertex(v2);

    vec3 c1 = p3*(1 - alpha1) + alpha1*p1;
    vec3 c2 = p2*(1 - alpha2) + alpha2*p1;

    c1 = normalize(c1 - p2);
    c2 = normalize(c2 - p3);

    if (dot(e1, t1) > dot(e1, c1) ||
      dot(e2, t2) > dot(e2, c2)) {

      double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
      double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

      if (dot1 > dot2)
        d[0] = acos(dot1);
      else
        d[0] = acos(dot2);

      violating[0] = true;
    }
  }

  // check violating edge1
  {
    vec3 e1 = normalize(p3 - p1);
    vec3 e2 = normalize(p1 - p3);
    vec3 t1 = normalize(trip - p1);
    vec3 t2 = normalize(trip - p3);

    double alpha1 = edges[0]->alphaForVertex(v3);
    double alpha2 = edges[2]->alphaForVertex(v1);

    vec3 c1 = p3*(1 - alpha1) + alpha1*p2;
    vec3 c2 = p1*(1 - alpha2) + alpha2*p2;

    c1 = normalize(c1 - p1);
    c2 = normalize(c2 - p3);

    if (dot(e1, t1) > dot(e1, c1) ||
      dot(e2, t2) > dot(e2, c2)) {

      double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
      double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

      if (dot1 > dot2)
        d[1] = acos(dot1);
      else
        d[1] = acos(dot2);

      violating[1] = true;
    }
  }

  // check violating edge2
  {
    vec3 e1 = normalize(p2 - p1);
    vec3 e2 = normalize(p1 - p2);
    vec3 t1 = normalize(trip - p1);
    vec3 t2 = normalize(trip - p2);

    double alpha1 = edges[0]->alphaForVertex(v2);
    double alpha2 = edges[1]->alphaForVertex(v1);

    vec3 c1 = p2*(1 - alpha1) + alpha1*p3;
    vec3 c2 = p1*(1 - alpha2) + alpha2*p3;

    c1 = normalize(c1 - p1);
    c2 = normalize(c2 - p2);

    if (dot(e1, t1) > dot(e1, c1) ||
      dot(e2, t2) > dot(e2, c2)) {

      double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
      double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

      if (dot1 > dot2)
        d[2] = acos(dot1);
      else
        d[2] = acos(dot2);

      violating[2] = true;
    }
  }

  // compare violatings, choose minimum
  for (int i = 0; i < 3; i++) {
    if (violating[i] && d[i] < d_min) {
      triple->violating = true;
      triple->closestGeometry = edges[i];
      d_min = d[i];
    }
  }
}

//==================================================================
//  checkIfQuadrupleViolatesEdges()
//==================================================================
void LinearViolationChecker::checkIfQuadrupleViolatesEdges(Tet *tet)
{
  // Return immediately if quadruple doesn't exist
  if (!tet->quadruple || tet->quadruple->order() != Order::QUAD)
    return;

  Vertex *quadruple = tet->quadruple;
  quadruple->violating = false;

  Vertex   *verts[VERTS_PER_TET];
  HalfEdge *edges[EDGES_PER_TET];
  HalfFace *faces[FACES_PER_TET];

  m_mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

  vec3 v1 = verts[0]->pos();
  vec3 v2 = verts[1]->pos();
  vec3 v3 = verts[2]->pos();
  vec3 v4 = verts[3]->pos();
  vec3 q = quadruple->pos();

  // check edge1, using edges e2,e3, e4,e5
  if (!quadruple->violating) {
    float t1 = edges[1]->alphaForVertex(verts[0]);   // e2
    float t2 = edges[2]->alphaForVertex(verts[0]);   // e3
    vec3 c1 = (1 - t1)*v1 + t1*v3;
    vec3 c2 = (1 - t2)*v1 + t2*v4;
    vec3 n1 = normalize(cross(c1 - v2, c2 - v2));
    vec3 q1 = q - v2;
    double d1 = dot(n1, q1);

    float t3 = edges[3]->alphaForVertex(verts[1]);   // e4
    float t4 = edges[4]->alphaForVertex(verts[1]);   // e6
    vec3 c3 = (1 - t3)*v2 + t3*v3;
    vec3 c4 = (1 - t4)*v2 + t4*v4;
    vec3 n2 = normalize(cross(c4 - v1, c3 - v1));
    vec3 q2 = q - v1;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[0];
    }
  }

  // check edge2, using edges e1,e3, e4,e6
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[0]);  // e1
    float t2 = edges[2]->alphaForVertex(verts[0]);  // e3
    vec3 c1 = (1 - t1)*v1 + t1*v2;
    vec3 c2 = (1 - t2)*v1 + t2*v4;
    vec3 n1 = normalize(cross(c2 - v3, c1 - v3));
    vec3 q1 = q - v3;
    double d1 = dot(n1, q1);

    float t3 = edges[3]->alphaForVertex(verts[2]);  // e4
    float t4 = edges[5]->alphaForVertex(verts[2]);  // e5
    vec3 c3 = (1 - t3)*v3 + t3*v2;
    vec3 c4 = (1 - t4)*v3 + t4*v4;
    vec3 n2 = normalize(cross(c3 - v1, c4 - v1));
    vec3 q2 = q - v1;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[1];
    }
  }

  // check edge3, using edges e1,e2, e5,e6
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[0]); // e1
    float t2 = edges[1]->alphaForVertex(verts[0]); // e2
    vec3 c1 = (1 - t1)*v1 + t1*v2;
    vec3 c2 = (1 - t2)*v1 + t2*v3;
    vec3 n1 = normalize(cross(c1 - v4, c2 - v4));
    vec3 q1 = q - v4;
    double d1 = dot(n1, q1);

    float t3 = edges[4]->alphaForVertex(verts[3]); // e5
    float t4 = edges[5]->alphaForVertex(verts[3]); // e6
    vec3 c3 = (1 - t3)*v4 + t3*v2;
    vec3 c4 = (1 - t4)*v4 + t4*v3;
    vec3 n2 = normalize(cross(c4 - v1, c3 - v1));
    vec3 q2 = q - v1;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[2];
    }
  }

  // check edge4, using edges e1,e5, e2,e6
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[1]); // e1
    float t2 = edges[4]->alphaForVertex(verts[1]); // e5
    vec3 c1 = (1 - t1)*v2 + t1*v1;
    vec3 c2 = (1 - t2)*v2 + t2*v4;
    vec3 n1 = normalize(cross(c1 - v3, c2 - v3));
    vec3 q1 = q - v3;
    double d1 = dot(n1, q1);

    float t3 = edges[1]->alphaForVertex(verts[2]); // e2
    float t4 = edges[5]->alphaForVertex(verts[2]); // e5
    vec3 c3 = (1 - t3)*v3 + t3*v1;
    vec3 c4 = (1 - t4)*v3 + t4*v4;
    vec3 n2 = normalize(cross(c4 - v2, c3 - v2));
    vec3 q2 = q - v2;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[3];
    }
  }

  // check edge5, using edges e1,e4, e3, e6
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[1]); // e1
    float t2 = edges[3]->alphaForVertex(verts[1]); // e4
    vec3 c1 = (1 - t1)*v2 + t1*v1;
    vec3 c2 = (1 - t2)*v2 + t2*v3;
    vec3 n1 = normalize(cross(c2 - v4, c1 - v4));
    vec3 q1 = q - v4;
    double d1 = dot(n1, q1);

    float t3 = edges[2]->alphaForVertex(verts[3]); // e3
    float t4 = edges[5]->alphaForVertex(verts[3]); // e6
    vec3 c3 = (1 - t3)*v4 + t3*v1;
    vec3 c4 = (1 - t4)*v4 + t4*v3;
    vec3 n2 = normalize(cross(c3 - v2, c4 - v2));
    vec3 q2 = q - v2;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[4];
    }
  }

  // check edge6, using edges e2,e4, e3,e5
  if (!quadruple->violating) {
    float t1 = edges[1]->alphaForVertex(verts[2]); // e2
    float t2 = edges[3]->alphaForVertex(verts[2]); // e4
    vec3 c1 = (1 - t1)*v3 + t1*v1;
    vec3 c2 = (1 - t2)*v3 + t2*v2;
    vec3 n1 = normalize(cross(c1 - v4, c2 - v4));
    vec3 q1 = q - v4;
    double d1 = dot(n1, q1);

    float t3 = edges[2]->alphaForVertex(verts[3]); // e3
    float t4 = edges[4]->alphaForVertex(verts[3]); // e5
    vec3 c3 = (1 - t3)*v4 + t3*v1;
    vec3 c4 = (1 - t4)*v4 + t4*v2;
    vec3 n2 = normalize(cross(c4 - v3, c3 - v3));
    vec3 q2 = q - v3;
    double d2 = dot(n2, q2);

    if (d1 > 0 && d2 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = edges[5];
    }
  }
}

//================================================================
//  checkIfQuadrupleViolatesFaces()
//================================================================
void LinearViolationChecker::checkIfQuadrupleViolatesFaces(Tet *tet)
{
  // Return immediately if quadruple doesn't exist
  if (!tet->quadruple || tet->quadruple->order() != Order::QUAD)
    return;

  Vertex *quadruple = tet->quadruple;
  quadruple->violating = false;

  Vertex   *verts[VERTS_PER_TET];
  HalfEdge *edges[EDGES_PER_TET];
  HalfFace *faces[FACES_PER_TET];

  m_mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

  vec3 v1 = verts[0]->pos();
  vec3 v2 = verts[1]->pos();
  vec3 v3 = verts[2]->pos();
  vec3 v4 = verts[3]->pos();
  vec3 q = quadruple->pos();

  // check face1, using edges e3, e5, e6
  if (!quadruple->violating) {
    float t1 = edges[2]->alphaForVertex(verts[0]);
    float t2 = edges[4]->alphaForVertex(verts[1]);
    float t3 = edges[5]->alphaForVertex(verts[2]);

    vec3 c1 = (1 - t1)*v1 + t1*v4;
    vec3 c2 = (1 - t2)*v2 + t2*v4;
    vec3 c3 = (1 - t3)*v3 + t3*v4;

    vec3 n1 = normalize(cross(c2 - v1, c3 - v1));
    vec3 n2 = normalize(cross(c3 - v2, c1 - v2));
    vec3 n3 = normalize(cross(c1 - v3, c2 - v3));

    vec3 q1 = q - v1;
    vec3 q2 = q - v2;
    vec3 q3 = q - v3;

    double d1 = dot(n1, q1);
    double d2 = dot(n2, q2);
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = faces[0];
    }
  }
  // check face2, using edges e1, e4, e6
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[0]);
    float t2 = edges[3]->alphaForVertex(verts[2]);
    float t3 = edges[5]->alphaForVertex(verts[3]);

    vec3 c1 = (1 - t1)*v1 + t1*v2;
    vec3 c2 = (1 - t2)*v3 + t2*v2;
    vec3 c3 = (1 - t3)*v4 + t3*v2;

    vec3 n1 = normalize(cross(c2 - v1, c3 - v1));
    vec3 n2 = normalize(cross(c3 - v3, c1 - v3));
    vec3 n3 = normalize(cross(c1 - v4, c2 - v4));

    vec3 q1 = q - v1;
    vec3 q2 = q - v3;
    vec3 q3 = q - v4;

    double d1 = dot(n1, q1);
    double d2 = dot(n2, q2);
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = faces[1];
    }
  }
  // check face3, using edges e2, e4, e6
  if (!quadruple->violating) {
    float t1 = edges[1]->alphaForVertex(verts[0]);
    float t2 = edges[3]->alphaForVertex(verts[1]);
    float t3 = edges[4]->alphaForVertex(verts[3]);

    vec3 c1 = (1 - t1)*v1 + t1*v3;
    vec3 c2 = (1 - t2)*v2 + t2*v3;
    vec3 c3 = (1 - t3)*v4 + t3*v3;

    vec3 n1 = normalize(cross(c3 - v1, c2 - v1));
    vec3 n2 = normalize(cross(c1 - v2, c3 - v2));
    vec3 n3 = normalize(cross(c2 - v4, c1 - v4));

    vec3 q1 = q - v1;
    vec3 q2 = q - v2;
    vec3 q3 = q - v4;

    double d1 = dot(n1, q1);
    double d2 = dot(n2, q2);
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = faces[2];
    }
  }
  // check face4, using edges, e1, e2, e3
  if (!quadruple->violating) {
    float t1 = edges[0]->alphaForVertex(verts[1]);
    float t2 = edges[1]->alphaForVertex(verts[2]);
    float t3 = edges[2]->alphaForVertex(verts[3]);

    vec3 c1 = (1 - t1)*v2 + t1*v1;
    vec3 c2 = (1 - t2)*v3 + t2*v1;
    vec3 c3 = (1 - t3)*v4 + t3*v1;

    vec3 n1 = normalize(cross(c3 - v2, c2 - v2));
    vec3 n2 = normalize(cross(c1 - v3, c3 - v3));
    vec3 n3 = normalize(cross(c2 - v4, c1 - v4));

    vec3 q1 = q - v2;
    vec3 q2 = q - v3;
    vec3 q3 = q - v4;

    double d1 = dot(n1, q1);
    double d2 = dot(n2, q2);
    double d3 = dot(n3, q3);

    if (d1 > 0 && d2 > 0 && d3 > 0) {
      quadruple->violating = true;
      quadruple->closestGeometry = faces[3];
    }
  }
}


}
