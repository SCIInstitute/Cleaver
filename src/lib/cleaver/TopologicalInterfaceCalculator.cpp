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

#include "TopologicalInterfaceCalculator.h"

#include "Plane.h"
#include <cmath>

int SolveQuadric(double c[3], double s[2]);
int SolveCubic(double c[4], double s[3]);
void clipRoots(double s[3], int &num_roots);

namespace cleaver
{
// TODO(jonbronson): Move exterior interface check into a separate
// BoundaryInterfaceCalculator method that implements the interface.


TopologicalInterfaceCalculator::TopologicalInterfaceCalculator(
    TetMesh *mesh, AbstractVolume *volume) : m_mesh(mesh), m_volume(volume) {}


void TopologicalInterfaceCalculator::computeCutForEdge(HalfEdge *edge) {
  double t_ab;
  double t_ac;
  double t_bc;
  bool ac_crossing = false;
  bool bc_crossing = false;

  // order verts
  Vertex *v2 = edge->vertex;
  Vertex *v1 = edge->mate->vertex;

  // set as evaluated
  edge->evaluated = true;
  edge->mate->evaluated = true;

  // do labels differ?
  if (v1->label == v2->label)
    return;

  double t1 = 0.000000;
  double t2 = 0.333333;
  double t3 = 0.666666;
  double t4 = 1.000000;

  vec3 x1 = (1 - t1)*v1->pos() + t1*v2->pos();
  vec3 x2 = (1 - t2)*v1->pos() + t2*v2->pos();
  vec3 x3 = (1 - t3)*v1->pos() + t3*v2->pos();
  vec3 x4 = (1 - t4)*v1->pos() + t4*v2->pos();

  int a_mat = v1->label;
  int b_mat = v2->label;


  {
    double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, b_mat);
    double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, b_mat);
    double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, b_mat);
    double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, b_mat);

    vec3 p1 = vec3(t1, y1, 0.0);
    vec3 p2 = vec3(t2, y2, 0.0);
    vec3 p3 = vec3(t3, y3, 0.0);
    vec3 p4 = vec3(t4, y4, 0.0);


    double c[4];             // coefficients (a,b,c,d)
    double s[3];             // solutions    (roots)
    int num_roots = 0;

    computeLagrangePolynomial(p1, p2, p3, p4, c);

    if (c[3] == 0)
      num_roots = SolveQuadric(c, s);
    else
      num_roots = SolveCubic(c, s);
    clipRoots(s, num_roots);

    if (num_roots != 1)
    {
      std::cout << "wow, unexpected for this dataset!" << std::endl;
      std::cout << "roots = [";
      for (int i = 0; i < num_roots; i++)
        std::cout << s[i] << ((i + 1 < num_roots) ? ", " : "] ");
      std::cout << std::endl;

      std::cout << "Points: ["
        << "(" << t1 << "," << y1 << "),"
        << "(" << t2 << "," << y2 << "),"
        << "(" << t3 << "," << y3 << "),"
        << "(" << t4 << "," << y4 << ")]" << std::endl;
      std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
        << ", c=" << c[2] << ", d=" << c[3] << std::endl;

      //exit(0);
      // badEdges.push_back(v1->pos());
      // badEdges.push_back(v2->pos());
    }

    t_ab = s[0];
  }

  // Now Test if a 3rd Material pops up
  if (m_volume->numberOfMaterials() > 2)
  {

    // get 3rd material
    int c_mat = -1;
    for (int m = 0; m < m_volume->numberOfMaterials(); m++) {
      if (m != a_mat && m != b_mat)
      {
        c_mat = m;
        break;
      }
    }

    // compute crossing parameter t_ac (a,c crossing)
    {
      double a1 = m_volume->valueAt(v1->pos(), a_mat);
      double a2 = m_volume->valueAt(v2->pos(), a_mat);
      double c1 = m_volume->valueAt(v1->pos(), c_mat);
      double c2 = m_volume->valueAt(v2->pos(), c_mat);

      // since a is definitely maximum on v1, can only
      // be a crossing if c is greater than a on v2
      if (c2 > a2)
      {
        double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, c_mat);
        double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, c_mat);
        double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, c_mat);
        double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, c_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1, p2, p3, p4, c);

        if (c[3] == 0)
          num_roots = SolveQuadric(c, s);
        else
          num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if (num_roots != 1)
        {
          std::cout << "wow, unexpected for this dataset!" << std::endl;
          std::cout << "roots = [";
          for (int i = 0; i < num_roots; i++)
            std::cout << s[i] << ((i + 1 < num_roots) ? ", " : "] ");
          std::cout << std::endl;

          std::cout << "Points: ["
            << "(" << t1 << "," << y1 << "),"
            << "(" << t2 << "," << y2 << "),"
            << "(" << t3 << "," << y3 << "),"
            << "(" << t4 << "," << y4 << ")]" << std::endl;
          std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
            << ", c=" << c[2] << ", d=" << c[3] << std::endl;

          //exit(0);
          // badEdges.push_back(v1->pos());
          // badEdges.push_back(v2->pos());
        }

        t_ac = s[0];

        vec3 pos = (1 - t_ac)*v1->pos() + t_ac*v2->pos();

        double ac = m_volume->valueAt(pos, a_mat);
        double  b = m_volume->valueAt(pos, b_mat);
        if (ac >= b && t_ac >= 0.0 && t_ac <= 1.0)
        {
          ac_crossing = true;
        }
      }
    }

    // compute crossing parameter t_bc (b,c crossing)
    {
      double c1 = m_volume->valueAt(v1->pos(), c_mat);
      double c2 = m_volume->valueAt(v2->pos(), c_mat);
      double b1 = m_volume->valueAt(v1->pos(), b_mat);
      double b2 = m_volume->valueAt(v2->pos(), b_mat);

      // since b is definitely maximum on v2, can only
      // be a crossing if c is greater than b on v1
      if (c1 > b1)
      {
        double y1 = m_volume->valueAt(x1, b_mat) - m_volume->valueAt(x1, c_mat);
        double y2 = m_volume->valueAt(x2, b_mat) - m_volume->valueAt(x2, c_mat);
        double y3 = m_volume->valueAt(x3, b_mat) - m_volume->valueAt(x3, c_mat);
        double y4 = m_volume->valueAt(x4, b_mat) - m_volume->valueAt(x4, c_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1, p2, p3, p4, c);

        if (c[3] == 0)
          num_roots = SolveQuadric(c, s);
        else
          num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if (num_roots != 1)
        {
          std::cout << "wow, unexpected for this dataset!" << std::endl;
          std::cout << "roots = [";
          for (int i = 0; i < num_roots; i++)
            std::cout << s[i] << ((i + 1 < num_roots) ? ", " : "] ");
          std::cout << std::endl;

          std::cout << "Points: ["
            << "(" << t1 << "," << y1 << "),"
            << "(" << t2 << "," << y2 << "),"
            << "(" << t3 << "," << y3 << "),"
            << "(" << t4 << "," << y4 << ")]" << std::endl;
          std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
            << ", c=" << c[2] << ", d=" << c[3] << std::endl;

          //exit(0);
          // badEdges.push_back(v1->pos());
          // badEdges.push_back(v2->pos());
        }

        t_bc = s[0];

        vec3 pos = (1 - t_bc)*v1->pos() + t_bc*v2->pos();

        double bc = m_volume->valueAt(pos, b_mat);
        double  a = m_volume->valueAt(pos, a_mat);
        if (bc >= a && t_bc >= 0.0 && t_bc <= 1.0)
        {
          bc_crossing = true;
        }
      }
    }

    // if 3rd material pops up, handle it
    if (ac_crossing && bc_crossing)
    {

      // put topological cut haflway between ac/bc interfaces
      double tt = 0.5f*(t_ac + t_bc);

      Vertex *cut = new Vertex(m_volume->numberOfMaterials());
      tt = std::max(tt, 0.0);
      tt = std::min(tt, 1.0);

      // check violating condition
      if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
        cut->violating = true;
      else
        cut->violating = false;

      cut->order() = Order::CUT;
      cut->pos() = v1->pos()*(1 - tt) + v2->pos()*tt;

      if (tt < 0.5)
        cut->closestGeometry = v1;
      else
        cut->closestGeometry = v2;

      // doesn't really matter which
      cut->label = c_mat;
      cut->lbls[c_mat] = true;

      // check violating condition
      if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
        cut->violating = true;
      else
        cut->violating = false;

      cut->order() = Order::CUT;

      //---------------------
      // attach cut to edge
      //---------------------
      cut->phantom = false;   // does it actually split a topology?
      edge->cut = cut;
      edge->mate->cut = cut;
    }
  }
}

/*
void TopologicalInterfaceCalculator::computeCutForEdge2(HalfEdge *edge) {
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


  }

  int a_mat = v1->label;
  int b_mat = v2->label;

  double a1 = m_volume->valueAt(v1->pos(), a_mat); //  v1->vals[a_mat];
  double a2 = m_volume->valueAt(v2->pos(), a_mat); //  v2->vals[a_mat];
  double b1 = m_volume->valueAt(v1->pos(), b_mat); //  v1->vals[b_mat];
  double b2 = m_volume->valueAt(v2->pos(), b_mat); //  v2->vals[b_mat];
  double top = (a1 - b1);
  double bot = (b2 - a2 + a1 - b1);
  double t = top / bot;

  // Now Test If a 3rd material pops up, test
  if (m_volume->numberOfMaterials() > 2)
  {
    bool ac_crossing = false;
    bool bc_crossing = false;
    double t_ac = -1;
    double t_bc = -1;

    // get 3rd material
    int c_mat = -1;
    for (int m = 0; m < m_volume->numberOfMaterials(); m++)
    {
      if (m != a_mat && m != b_mat)
      {
        c_mat = m;
        break;
      }
    }

    // compute crossing parameter t_ac (a,c crossing)
    {
      double a1 = m_volume->valueAt(v1->pos(), a_mat);
      double a2 = m_volume->valueAt(v2->pos(), a_mat);
      double c1 = m_volume->valueAt(v1->pos(), c_mat);
      double c2 = m_volume->valueAt(v2->pos(), c_mat);

      // since a is definitely maximum on v1, can only
      // be a crossing if c is greater than a on v2
      if (c2 > a2)
      {
        double top = (a1 - c1);
        double bot = (c2 - a2 + a1 - c1);
        double t = top / bot;

        vec3   pos = v1->pos()*(1 - t) + v2->pos()*(t);
        //double ac  = a1*(1-t) + a2*(t);
        double ac = m_volume->valueAt(pos, c_mat);

        if (ac >= m_volume->valueAt(pos, b_mat) && t >= 0.0 && t <= 1.0)
        {
          ac_crossing = true;
          t_ac = t;
        }
      }
    }

    // compute crossing parameter t_bc (b,c crossing)
    {
      double c1 = m_volume->valueAt(v1->pos(), c_mat);
      double c2 = m_volume->valueAt(v2->pos(), c_mat);
      double b1 = m_volume->valueAt(v1->pos(), b_mat);
      double b2 = m_volume->valueAt(v2->pos(), b_mat);

      // since b is definitely maximum on v2, can only
      // be a crossing if c is greater than b on v1
      if (c1 > b1)
      {
        double top = (c1 - b1);
        double bot = (b2 - c2 + c1 - b1);
        double t = top / bot;

        vec3   pos = v1->pos()*(1 - t) + v2->pos()*(t);
        //double bc  = c1*(1-t) + c2*(t);
        double bc = m_volume->valueAt(pos, c_mat);

        if (bc >= m_volume->valueAt(pos, a_mat) && t >= 0.0 && t <= 1.0)
        {
          bc_crossing = true;
          t_bc = t;
        }
      }
    }

    // if 3rd material pops up, handle it
    if (ac_crossing && bc_crossing) {

      // put topological cut haflway between ac/bc interfaces
      double tt = 0.5f*(t_ac + t_bc);

      Vertex *cut = new Vertex(m_volume->numberOfMaterials());
      tt = std::max(tt, 0.0);
      tt = std::min(tt, 1.0);

      // check violating condition
      if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
        cut->violating = true;
      else
        cut->violating = false;

      cut->order() = Order::CUT;
      cut->pos() = (1 - tt)*v1->pos() + tt*v2->pos();

      if (tt < 0.5)
        cut->closestGeometry = v1;
      else
        cut->closestGeometry = v2;

      // doesn't really matter which
      cut->label = c_mat;
      cut->lbls[c_mat] = true;

      //---------------------
      // attach cut to edge
      //---------------------
      cut->phantom = false;   // does it actually split a topology?
      edge->cut = cut;
      edge->mate->cut = cut;

    }
  }
}
*/

void TopologicalInterfaceCalculator::computeTripleForFace(HalfFace *face) {
 // set as evaluated
  face->evaluated = true;
  if (face->mate)
    face->mate->evaluated = true;

  // if  least 1 topological cut exists, determine how to place other missing cuts
  if ((face->halfEdges[0]->cut && face->halfEdges[0]->cut->phantom) ||
    (face->halfEdges[1]->cut && face->halfEdges[1]->cut->phantom) ||
    (face->halfEdges[2]->cut && face->halfEdges[2]->cut->phantom)) {

    int mat_count = 3;
    int top_count = 0;
    int mats[3];
    for (int e = 0; e < 3; e++) {
      mats[e] = face->halfEdges[e]->vertex->label;
      if (face->halfEdges[e]->cut && !face->halfEdges[e]->cut->phantom)
        top_count++;
    }
    if (mats[0] == mats[1] || mats[0] == mats[2])
      mat_count--;
    if (mats[1] == mats[2])
      mat_count--;

    // 2-mat has 2 cases, only 1 case needs a triple
    if (mat_count == 2)
    {
      // if there are 2 top cuts, there is no topological triple
      if (top_count == 2)
        return;

      // first, determine which edge is which
      HalfEdge *edge_with_3_mats = nullptr;
      HalfEdge *edge_with_2_mats = nullptr;
      HalfEdge *edge_with_1_mats = nullptr;
      int e1 = -1, e2 = -1, e3 = -1;

      for (int e = 0; e < 3; e++) {
        // 1 edge must have 0 mat cuts (2 verts with same label)
        if (face->halfEdges[e]->vertex->label == face->halfEdges[e]->mate->vertex->label)
        {
          edge_with_1_mats = face->halfEdges[e];
          e1 = e;
          continue;
        }
        // 1 edge must have 1 mat cuts (materials equal to vertices of edge with top cut)
        if (!face->halfEdges[e]->cut || (face->halfEdges[e]->cut && face->halfEdges[e]->cut->phantom))
        {
          edge_with_2_mats = face->halfEdges[e];
          e2 = e;
          continue;
        }
        // 1 edge must have 2 mat cuts (the only non-phantom topological cut)
        if (face->halfEdges[e]->cut && !face->halfEdges[e]->cut->phantom)
        {
          edge_with_3_mats = face->halfEdges[e];
          e3 = e;
          continue;
        }
      }

      // Sanity Check
      if ((!edge_with_1_mats || !edge_with_2_mats || !edge_with_3_mats) &&
        !face->halfEdges[0]->vertex->isExterior &&
        !face->halfEdges[1]->vertex->isExterior &&
        !face->halfEdges[2]->vertex->isExterior)
      {
        std::cerr << "Failed to understand a Topological Triple case. Aborting" << std::endl;
        exit(13);
      }

      //-------------------------------------------------------------------------
      // new logic moves the topological cut to interfaces nearest the 1-mat edge
      //-------------------------------------------------------------------------
      Vertex *v1 = edge_with_3_mats->vertex;
      Vertex *v2 = edge_with_3_mats->mate->vertex;
      Vertex *near = nullptr;
      Vertex *far = nullptr;

      // 1. Check is v1 or v2 the vertex incident to the edge with 1 material
      if (edge_with_1_mats->vertex == v1 || edge_with_1_mats->mate->vertex == v1)
      {
        near = v1;
        far = v2;
      } else if (edge_with_1_mats->vertex == v2 || edge_with_1_mats->mate->vertex == v2)
      {
        near = v2;
        far = v1;
      } else {
        std::cerr << "Fatal Error. Bad Triangle Setup. Aborting." << std::endl;
        exit(0);
      }

      // 2. Recompute the 2 crossings
      int a_mat = near->label;
      int b_mat = far->label;
      int c_mat = -1;
      for (int m = 0; m < m_volume->numberOfMaterials(); m++)
      {
        if (m != a_mat && m != b_mat)
        {
          c_mat = m;
          break;
        }
      }
      double t_ac;
      double t_bc;
      double t1 = 0.000000;
      double t2 = 0.333333;
      double t3 = 0.666666;
      double t4 = 1.000000;
      vec3 x1 = (1 - t1)*near->pos() + t1*far->pos();
      vec3 x2 = (1 - t2)*near->pos() + t2*far->pos();
      vec3 x3 = (1 - t3)*near->pos() + t3*far->pos();
      vec3 x4 = (1 - t4)*near->pos() + t4*far->pos();

      // find ac crossing first
      {
        double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, c_mat);
        double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, c_mat);
        double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, c_mat);
        double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, c_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1, p2, p3, p4, c);

        if (c[3] == 0)
          num_roots = SolveQuadric(c, s);
        else
          num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if (num_roots != 1)
        {
          std::cout << "wow, unexpected for this dataset!" << std::endl;
          std::cout << "roots = [";
          for (int i = 0; i < num_roots; i++)
            std::cout << s[i] << ((i + 1 < num_roots) ? ", " : "] ");
          std::cout << std::endl;

          std::cout << "Points: ["
            << "(" << t1 << "," << y1 << "),"
            << "(" << t2 << "," << y2 << "),"
            << "(" << t3 << "," << y3 << "),"
            << "(" << t4 << "," << y4 << ")]" << std::endl;
          std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
            << ", c=" << c[2] << ", d=" << c[3] << std::endl;

          //exit(0);
          // badEdges.push_back(v1->pos());
          // badEdges.push_back(v2->pos());
        }

        t_ac = s[0];
      }
      // find bc crossing second
      {
        double y1 = m_volume->valueAt(x1, b_mat) - m_volume->valueAt(x1, c_mat);
        double y2 = m_volume->valueAt(x2, b_mat) - m_volume->valueAt(x2, c_mat);
        double y3 = m_volume->valueAt(x3, b_mat) - m_volume->valueAt(x3, c_mat);
        double y4 = m_volume->valueAt(x4, b_mat) - m_volume->valueAt(x4, c_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1, p2, p3, p4, c);

        if (c[3] == 0)
          num_roots = SolveQuadric(c, s);
        else
          num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if (num_roots != 1)
        {
          std::cout << "wow, unexpected for this dataset!" << std::endl;
          std::cout << "roots = [";
          for (int i = 0; i < num_roots; i++)
            std::cout << s[i] << ((i + 1 < num_roots) ? ", " : "] ");
          std::cout << std::endl;

          std::cout << "Points: ["
            << "(" << t1 << "," << y1 << "),"
            << "(" << t2 << "," << y2 << "),"
            << "(" << t3 << "," << y3 << "),"
            << "(" << t4 << "," << y4 << ")]" << std::endl;
          std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
            << ", c=" << c[2] << ", d=" << c[3] << std::endl;

          // exit(0);
          // badEdges.push_back(near->pos());
          // badEdges.push_back(far->pos());
        }

        t_bc = s[0];
      }


      // 3. Move cut to the one that's closer to incident vertex (near)
      if (t_ac < t_bc)
      {
        t_ac += 1E-2;
        edge_with_3_mats->cut->pos() = (1 - t_ac)*near->pos() + t_ac*far->pos();
      } else
      {
        t_bc += 1E-2;
        edge_with_3_mats->cut->pos() = (1 - t_bc)*near->pos() + t_bc*far->pos();
      }
    }

    return;


  }
}


void TopologicalInterfaceCalculator::computeQuadrupleForTet(Tet *tet) {
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
  tet->quadruple->phantom = true;
  tet->quadruple->violating = false;
}


//=========================================================
// Compute the Interpolating Cubic Lagrange Polynomial
// for the 4 Given 3D Points.
//=========================================================
void TopologicalInterfaceCalculator::computeLagrangePolynomial(const vec3 &p1,
  const vec3 &p2, const vec3 &p3, const vec3 &p4, double coefficients[4]) {

  //--------------------------------------------------------------------------------
  // L_k = PRODUCT_From[i=0,i!=k]_To[n]      (x - x_i) / (x_k - x_i)
  //
  // Cubic has special form:
  //
  // L_k = x^3 - a1.x^2 - a2.x^2 - a3.x^2 + a1.a2.x + a1.a3.x + a2.a3.x - a1.a2.a3
  //
  // Grouping terms to the form
  // L_k = a.x^3 + b.x^2 + c.x + d
  //   a = 1
  //   b = -a1 -a2 - a3
  //   c = a1*a2 + a2*a3 + a3*a1
  //   d = -a1*a2*a3
  //  divisor term being (x_k - a1)(x_k - a2)(x_k - a3)
  //
  // The final polynomial can be computed in terms of coefficients.
  //--------------------------------------------------------------------------------

  std::vector<vec3> p;
  p.push_back(p1);
  p.push_back(p2);
  p.push_back(p3);
  p.push_back(p4);


  double L[4][4];
  double LX[4][3];
  //double bot[4];  // unreferenced


  for (int k = 0; k < 4; k++)
  {
    // initialize values to 1
    L[k][0] = L[k][1] = L[k][2] = L[k][3] = 1;

    int idx = 0;
    for (int i = 0; i < 4; i++)
    {
      // skip i=k case
      if (i == k)
        continue;

      // grab the appropriate X values for each basis
      LX[k][idx++] = p[i].x;
    }

    /*a*/ L[k][0] = 1;
    /*b*/ L[k][1] = -LX[k][0] - LX[k][1] - LX[k][2];
    /*c*/ L[k][2] = LX[k][0] * LX[k][1] + LX[k][1] * LX[k][2] + LX[k][2] * LX[k][0];
    /*d*/ L[k][3] = -1 * (LX[k][0] * LX[k][1] * LX[k][2]);

    double bot = (p[k].x - LX[k][0])*(p[k].x - LX[k][1])*(p[k].x - LX[k][2]);

    L[k][0] /= bot;
    L[k][1] /= bot;
    L[k][2] /= bot;
    L[k][3] /= bot;
  }

  //-------------------------------
  // Add basis functions together
  //-------------------------------
  double a = p1.y*L[0][0] + p2.y*L[1][0] + p3.y*L[2][0] + p4.y*L[3][0];
  double b = p1.y*L[0][1] + p2.y*L[1][1] + p3.y*L[2][1] + p4.y*L[3][1];
  double c = p1.y*L[0][2] + p2.y*L[1][2] + p3.y*L[2][2] + p4.y*L[3][2];
  double d = p1.y*L[0][3] + p2.y*L[1][3] + p3.y*L[2][3] + p4.y*L[3][3];

  coefficients[0] = d;
  coefficients[1] = c;
  coefficients[2] = b;
  coefficients[3] = a;
}

// TODO(jonbronson): Move out into separate geometry calculator
bool TopologicalInterfaceCalculator::planeIntersect(Vertex *v1, Vertex *v2,
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
void TopologicalInterfaceCalculator::forcePointIntoTriangle(
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
