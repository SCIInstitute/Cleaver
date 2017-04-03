//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Linear Interface Calculator
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

#include "LinearInterfaceCalculator.h"

#include "Plane.h"
#include <cmath>

namespace cleaver
{
// TODO(jonbronson): Move exterior interface check into a separate
// BoundaryInterfaceCalculator method that implements the interface.


LinearInterfaceCalculator::LinearInterfaceCalculator(
    TetMesh *mesh, AbstractVolume *volume) : m_mesh(mesh), m_volume(volume) {}


void LinearInterfaceCalculator::computeCutForEdge(HalfEdge *edge) {

  // order verts
  Vertex *v2 = edge->vertex;
  Vertex *v1 = edge->mate->vertex;

  // set as evaluated
  edge->evaluated = true;
  edge->mate->evaluated = true;

  // do labels differ?
  if (v1->label == v2->label)
    return;

  // added feb 20 to attempt boundary conforming
  if ((v1->isExterior && !v2->isExterior) || (!v1->isExterior && v2->isExterior))
  {
    // place a cut exactly on boundary.
    // for now, put it exactly halfway, this is wrong, but it's just a test
    vec3 a, b;
    if (v1->isExterior)
    {
      a = v2->pos();
      b = v1->pos();
    } else {
      a = v1->pos();
      b = v2->pos();
    }

    double t = 1000;
    Vertex *cut = new Vertex(m_volume->numberOfMaterials());

    if (b.x > m_volume->bounds().maxCorner().x)
    {
      double tt = (m_volume->bounds().maxCorner().x - a.x) / (b.x - a.x);
      if (tt < t)
        t = tt;
    } else if (b.x < m_volume->bounds().minCorner().x)
    {
      double tt = (m_volume->bounds().minCorner().x - a.x) / (b.x - a.x);
      if (tt < t)
        t = tt;
    }

    if (b.y > m_volume->bounds().maxCorner().y)
    {
      double tt = (m_volume->bounds().maxCorner().y - a.y) / (b.y - a.y);
      if (tt < t)
        t = tt;
    } else if (b.y < m_volume->bounds().minCorner().y)
    {
      double tt = (m_volume->bounds().minCorner().y - a.y) / (b.y - a.y);
      if (tt < t)
        t = tt;
    }

    if (b.z > m_volume->bounds().maxCorner().z)
    {
      double tt = (m_volume->bounds().maxCorner().z - a.z) / (b.z - a.z);
      if (tt < t)
        t = tt;
    } else if (b.z < m_volume->bounds().minCorner().z)
    {
      double tt = (m_volume->bounds().minCorner().z - a.z) / (b.z - a.z);
      if (tt < t)
        t = tt;
    }

    // now use t to compute real point
    if (v1->isExterior)
      cut->pos() = v2->pos()*(1 - t) + v1->pos()*t;
    else
      cut->pos() = v1->pos()*(1 - t) + v2->pos()*t;

    cut->label = std::min(v1->label, v2->label);
    cut->lbls[v1->label] = true;
    cut->lbls[v2->label] = true;

    // check violating condition
    if ((t <= edge->alpha) || (t >= (1 - edge->mate->alpha)))
      cut->violating = true;
    else
      cut->violating = false;

    if (t < 0.5) {
      if (v1->isExterior)
        cut->closestGeometry = v2;
      else
        cut->closestGeometry = v1;
    } else {
      if (v1->isExterior)
        cut->closestGeometry = v1;
      else
        cut->closestGeometry = v2;
    }

    edge->cut = cut;
    edge->mate->cut = cut;
    cut->order() = Order::CUT;
    return;
  }

  //---- The Following is the STANDARD cut computation code

  int a_mat = v1->label;
  int b_mat = v2->label;


  double a1 = m_volume->valueAt(v1->pos(), a_mat); //  v1->vals[a_mat];
  double a2 = m_volume->valueAt(v2->pos(), a_mat); //  v2->vals[a_mat];
  double b1 = m_volume->valueAt(v1->pos(), b_mat); //  v1->vals[b_mat];
  double b2 = m_volume->valueAt(v2->pos(), b_mat); //  v2->vals[b_mat];
  double top = (a1 - b1);
  double bot = (b2 - a2 + a1 - b1);
  double t = top / bot;

  Vertex *cut = new Vertex(m_volume->numberOfMaterials());
  t = std::max(t, 0.0);
  t = std::min(t, 1.0);
  cut->pos() = v1->pos()*(1 - t) + v2->pos()*t;

  if (t < 0.5)
    cut->closestGeometry = v1;
  else
    cut->closestGeometry = v2;

  // doesn't really matter which
  cut->label = a_mat;
  cut->lbls[v1->label] = true;
  cut->lbls[v2->label] = true;

  // check violating condition
  if ((t <= edge->alpha) || (t >= (1 - edge->mate->alpha)))
    cut->violating = true;
  else
    cut->violating = false;

  edge->cut = cut;
  edge->mate->cut = cut;
  cut->order() = Order::CUT;
}


void LinearInterfaceCalculator::computeTripleForFace(HalfFace *face) {
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

  vec3 result = vec3::zero;

  double a1, b1, c1, d1;
  double a2, b2, c2, d2;
  double a3, b3, c3, d3;

  // get materials
  int m1 = v1->label;
  int m2 = v2->label;
  int m3 = v3->label;

  // added feb 20 to attempt boundary conforming
  if (v1->isExterior || v2->isExterior || v3->isExterior)
  {
    // place a triple exactly on boundary.
    // for now, put in middle, its wrong but just to test
    int external_vertex;
    for (int i = 0; i < 3; i++) {
      if (verts[i]->isExterior) {
        external_vertex = i;
        break;
      }
    }

    vec3 a = edges[(external_vertex + 1) % 3]->cut->pos();
    vec3 b = edges[(external_vertex + 2) % 3]->cut->pos();


    Vertex *triple = new Vertex(m_volume->numberOfMaterials());
    triple->pos() = (0.5)*(a + b);
    triple->lbls[v1->label] = true;
    triple->lbls[v2->label] = true;
    triple->lbls[v3->label] = true;
    triple->order() = Order::TRIP;
    triple->violating = false;
    triple->closestGeometry = nullptr;
    face->triple = triple;
    if (face->mate)
      face->mate->triple = triple;

    return;
  }

  //computeTripleForFace2(face);
  //return;


  // determine orientation, pick an axis
  vec3 n = normalize((v2->pos() - v1->pos()).cross(v3->pos() - v1->pos()));

  int axis;
  double nx = fabs(n.dot(vec3(1, 0, 0)));
  double ny = fabs(n.dot(vec3(0, 1, 0)));
  double nz = fabs(n.dot(vec3(0, 0, 1)));

  if (nx >= ny && nx >= nz)
    axis = 1;
  else if (ny >= nx && ny >= nz)
    axis = 2;
  else
    axis = 3;

  if (axis == 1)
  {

    vec3 p1_m1 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m1), v1->pos().z);
    vec3 p2_m1 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m1), v2->pos().z);
    vec3 p3_m1 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m1), v3->pos().z);

    vec3 p1_m2 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m2), v1->pos().z);
    vec3 p2_m2 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m2), v2->pos().z);
    vec3 p3_m2 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m2), v3->pos().z);

    vec3 p1_m3 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m3), v1->pos().z);
    vec3 p2_m3 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m3), v2->pos().z);
    vec3 p3_m3 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m3), v3->pos().z);

    Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
    Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
    Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

    plane1.toScalars(a1, b1, c1, d1);
    plane2.toScalars(a2, b2, c2, d2);
    plane3.toScalars(a3, b3, c3, d3);

    double A[2][2];
    double b[2];

    A[0][0] = (a3 / b3 - a1 / b1);  //(a3/c3 - a1/c1);
    A[0][1] = (c3 / b3 - c1 / b1);  //(b3/c3 - b1/c1);
    A[1][0] = (a3 / b3 - a2 / b2);  //(a3/c3 - a2/c2);
    A[1][1] = (c3 / b3 - c2 / b2);  //(b3/c3 - b2/c2);

    b[0] = (d1 / b1 - d3 / b3);     //(d1/c1 - d3/c3);
    b[1] = (d2 / b2 - d3 / b3);     //(d2/c2 - d3/c3);

    // solve using cramers rule
    vec3 result2d = vec3::zero;

    double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];

    result2d.x = (b[0] * A[1][1] - b[1] * A[0][1]) / det;
    result2d.y = (b[1] * A[0][0] - b[0] * A[1][0]) / det;

    // intersect triangle plane with line (from result2d point along axis)
    vec3 origin(0, result2d.x, result2d.y);
    vec3 ray(1, 0, 0);
    bool success = planeIntersect(v1, v2, v3, origin, ray, result);
    if (!success)
    {
      //std::cout << "Failed to Project Triple BACK into 3D: Using Barycenter" << std::endl;
      //result = (1.0/3.0)*(v1->pos() + v2->pos() + v3->pos());
      //axis = 2;
      std::cout << "Failed Axis==1, the most likely candidate to succeeed..." << std::endl;
      exit(0);
    }

  } else if (axis == 2)
  {
    vec3 p1_m1 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m1), v1->pos().z);
    vec3 p2_m1 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m1), v2->pos().z);
    vec3 p3_m1 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m1), v3->pos().z);

    vec3 p1_m2 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m2), v1->pos().z);
    vec3 p2_m2 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m2), v2->pos().z);
    vec3 p3_m2 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m2), v3->pos().z);

    vec3 p1_m3 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m3), v1->pos().z);
    vec3 p2_m3 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m3), v2->pos().z);
    vec3 p3_m3 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m3), v3->pos().z);

    Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
    Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
    Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

    plane1.toScalars(a1, b1, c1, d1);
    plane2.toScalars(a2, b2, c2, d2);
    plane3.toScalars(a3, b3, c3, d3);

    double A[2][2];
    double b[2];

    A[0][0] = (a3 / b3 - a1 / b1);  //(a3/c3 - a1/c1);
    A[0][1] = (c3 / b3 - c1 / b1);  //(b3/c3 - b1/c1);
    A[1][0] = (a3 / b3 - a2 / b2);  //(a3/c3 - a2/c2);
    A[1][1] = (c3 / b3 - c2 / b2);  //(b3/c3 - b2/c2);

    b[0] = (d1 / b1 - d3 / b3);     //(d1/c1 - d3/c3);
    b[1] = (d2 / b2 - d3 / b3);     //(d2/c2 - d3/c3);

    // solve using cramers rule
    vec3 result2d = vec3::zero;

    double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];

    result2d.x = (b[0] * A[1][1] - b[1] * A[0][1]) / det;
    result2d.y = (b[1] * A[0][0] - b[0] * A[1][0]) / det;

    // intersect triangle plane with line (from result2d point along axis)
    vec3 origin(result2d.x, 0, result2d.y);
    vec3 ray(0, 1, 0);
    bool success = planeIntersect(v1, v2, v3, origin, ray, result);
    if (!success)
    {
      //axis = 3;
      std::cout << "Failed Axis==2, the most likely candidate to succeeed..." << std::endl;
      exit(0);
    }
  } else if (axis == 3)
  {
    vec3 p1_m1 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m1), v1->pos().y);
    vec3 p2_m1 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m1), v2->pos().y);
    vec3 p3_m1 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m1), v3->pos().y);

    vec3 p1_m2 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m2), v1->pos().y);
    vec3 p2_m2 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m2), v2->pos().y);
    vec3 p3_m2 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m2), v3->pos().y);

    vec3 p1_m3 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m3), v1->pos().y);
    vec3 p2_m3 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m3), v2->pos().y);
    vec3 p3_m3 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m3), v3->pos().y);

    Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
    Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
    Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

    plane1.toScalars(a1, b1, c1, d1);
    plane2.toScalars(a2, b2, c2, d2);
    plane3.toScalars(a3, b3, c3, d3);

    double A[2][2];
    double b[2];

    A[0][0] = (a3 / b3 - a1 / b1);  //(a3/c3 - a1/c1);
    A[0][1] = (c3 / b3 - c1 / b1);  //(b3/c3 - b1/c1);
    A[1][0] = (a3 / b3 - a2 / b2);  //(a3/c3 - a2/c2);
    A[1][1] = (c3 / b3 - c2 / b2);  //(b3/c3 - b2/c2);

    b[0] = (d1 / b1 - d3 / b3);     //(d1/c1 - d3/c3);
    b[1] = (d2 / b2 - d3 / b3);     //(d2/c2 - d3/c3);

    // solve using cramers rule
    vec3 result2d = vec3::zero;

    double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];

    result2d.x = (b[0] * A[1][1] - b[1] * A[0][1]) / det;
    result2d.y = (b[1] * A[0][0] - b[0] * A[1][0]) / det;

    // intersect triangle plane with line (from result2d point along axis)
    vec3 origin(result2d.x, result2d.y, 0);
    vec3 ray(0, 0, 1);
    bool success = planeIntersect(v1, v2, v3, origin, ray, result);
    if (!success)
    {
      std::cout << "Failed Axis==3, the most likely candidate to succeeed..." << std::endl;
      exit(0);
    }
  }

  forcePointIntoTriangle(v1->pos(), v2->pos(), v3->pos(), result);


  //-------------------------------------------------------
  // Create the Triple Vertex
  //-------------------------------------------------------
  Vertex *triple = new Vertex(m_volume->numberOfMaterials());
  triple->pos() = result;
  triple->lbls[v1->label] = true;
  triple->lbls[v2->label] = true;
  triple->lbls[v3->label] = true;
  triple->order() = Order::TRIP;
  triple->violating = false;
  triple->closestGeometry = nullptr;
  face->triple = triple;
  if (face->mate)
    face->mate->triple = triple;
}


// TODO(jonbronson): Move to alternative InterfaceCalculator
/*
void LinearInterfaceCalculator::computeTripleForFace2(HalfFace *face)
{
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

  // get materials
  int m1 = verts[0]->label;
  int m2 = verts[1]->label;
  int m3 = verts[2]->label;

  // Create Matrix with Material Values
  Matrix3x3 M;
  M(0, 0) = m_volume->valueAt(verts[0]->pos(), m1);
  M(0, 1) = m_volume->valueAt(verts[0]->pos(), m2);
  M(0, 2) = m_volume->valueAt(verts[0]->pos(), m3);
  M(1, 0) = m_volume->valueAt(verts[1]->pos(), m1);
  M(1, 1) = m_volume->valueAt(verts[1]->pos(), m2);
  M(1, 2) = m_volume->valueAt(verts[1]->pos(), m3);
  M(2, 0) = m_volume->valueAt(verts[2]->pos(), m1);
  M(2, 1) = m_volume->valueAt(verts[2]->pos(), m2);
  M(2, 2) = m_volume->valueAt(verts[2]->pos(), m2);

  // Solve Inverse
  Matrix3x3 Inv = M.inverse();

  // Multiply Inverse by 1 column vector [1,1,1]^T
  vec3 one(1, 1, 1);
  vec3 slambda = Inv*one;
  vec3  lambda = slambda / L1(slambda);


  vec3 result(lambda.dot(v1->pos()),
    lambda.dot(v2->pos()),
    lambda.dot(v2->pos()));

  if (lambda.x < 0 || lambda.y < 0 || lambda.z < 0)
    std::cout << "Triple location suggests topology is wrong" << std::endl;

  force_point_in_triangle(v1->pos(), v2->pos(), v3->pos(), result);


  //-------------------------------------------------------
  // Create the Triple Vertex
  //-------------------------------------------------------
  Vertex *triple = new Vertex(m_volume->numberOfMaterials());
  triple->pos() = result;
  triple->lbls[v1->label] = true;
  triple->lbls[v2->label] = true;
  triple->lbls[v3->label] = true;
  triple->order() = Order::TRIP;
  triple->violating = false;
  triple->closestGeometry = nullptr;
  face->triple = triple;
  if (face->mate)
    face->mate->triple = triple;
}
*/


void LinearInterfaceCalculator::computeQuadrupleForTet(Tet *tet) {
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

  // TODO:   Implement Compute Quadruple

  // for now, take middle

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

// TODO(jonbronson): Move out into separate geometry calculator
bool LinearInterfaceCalculator::planeIntersect(Vertex *v1, Vertex *v2,
    Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, float epsilon)
{
  //-------------------------------------------------
  // if v1, v2, and v3 are not unique, return FALSE
  //-------------------------------------------------
  if (v1->isEqualTo(v2) || v2->isEqualTo(v3) || v1->isEqualTo(v3))
    return false;
  else if (L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v1->pos() - v3->pos()) < epsilon)
    return false;


  vec3 p1 = origin;
  vec3 p2 = origin + ray;
  vec3 p3 = v1->pos();

  vec3 n = normalize(cross(normalize(v3->pos() - v1->pos()), normalize(v2->pos() - v1->pos())));

  double top = n.dot(p3 - p1);
  double bot = n.dot(p2 - p1);

  double t = top / bot;

  pt = origin + t*ray;

  if (pt != pt)
    return false;
  else
    return true;
}



// TODO(jonbronson): Consider moving into separate calculator/snapper.
void LinearInterfaceCalculator::forcePointIntoTriangle(
    vec3 a, vec3 b, vec3 c, vec3 &p)
{
  // Compute vectors
  vec3 v0 = c - a;
  vec3 v1 = b - a;
  vec3 v2 = p - a;

  // Compute dot products
  double dot00 = dot(v0, v0);
  double dot01 = dot(v0, v1);
  double dot02 = dot(v0, v2);
  double dot11 = dot(v1, v1);
  double dot12 = dot(v1, v2);

  // Compute barycentric coordinates
  double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  double v = (dot00 * dot12 - dot01 * dot02) * invDenom;
  double w = 1 - u - v;

  vec3 test = (1 - u - v)*a + v*b + u*c;

  // Check if point is in triangle
  u = std::max(0.0, u);
  v = std::max(0.0, v);
  w = std::max(0.0, w);

  double L1 = u + v + w;
  if (L1 > 0) {
    u /= L1;
    v /= L1;
  }

  p = (1 - u - v)*a + v*b + u*c;
}


} // namespace cleaver
