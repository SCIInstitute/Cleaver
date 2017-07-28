//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Tet Class
//
// Author:   Jonathan Bronson (bronson@sci.utah.edu)
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

#include "Tet.h"
#include <cmath>

namespace cleaver
{

Tet::Tet() : quadruple(nullptr), mat_label(-1), output(false), evaluated(false), flagged(false)
{
  faces[0] = faces[1] = faces[2] = faces[3] = nullptr;
  tets[0] = tets[1] = tets[2] = tets[3] = nullptr;
  parent = -1;
}

Tet::Tet(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, int material) :
  quadruple(nullptr), mat_label(material), output(false), evaluated(false), flagged(false)
{
  // initialize face info to empty
  faces[0] = faces[1] = faces[2] = faces[3] = nullptr;
  tets[0] = tets[1] = tets[2] = tets[3] = nullptr;
  parent = -1;

  // add adjacency info
  verts[0] = v1;
  verts[1] = v2;
  verts[2] = v3;
  verts[3] = v4;

  // both ways
  v1->tets.push_back(this);
  v2->tets.push_back(this);
  v3->tets.push_back(this);
  v4->tets.push_back(this);

}

Tet::~Tet()
{
}

float Tet::minAngle()
{
  float min = 180;

  //each tet has 6 dihedral angles between pairs of faces
  //compute the face normals for each face
  vec3 face_normals[4];

  for (int j=0; j<4; j++) {
    vec3 v0 = this->verts[(j+1)%4]->pos();
    vec3 v1 = this->verts[(j+2)%4]->pos();
    vec3 v2 = this->verts[(j+3)%4]->pos();
    vec3 normal = normalize(cross(v1-v0,v2-v0));

    // make sure normal faces 4th (opposite) vertex
    vec3 v3 = this->verts[(j+0)%4]->pos();
    vec3 v3_dir = normalize(v3 - v0);
    if(dot(v3_dir, normal) > 0)
      normal *= -1;

    face_normals[j] = normal;
  }
  //now compute the 6 dihedral angles between each pair of faces
  for (int j=0; j<4; j++)
    for (int k=j+1; k<4; k++) {
      double dot_product = dot(face_normals[j], face_normals[k]);
      dot_product = std::min(1.,std::max(dot_product,-1.));

      double dihedral_angle = 180.0 - acos(dot_product) * 180.0 / PI;
      dihedral_angle = std::min(180.,std::max(0.,dihedral_angle));

      if (dihedral_angle < min)
        min = static_cast<float>(dihedral_angle);
    }
  return min;
}

float Tet::maxAngle()
{
  float max = 0;

  //each tet has 6 dihedral angles between pairs of faces
  //compute the face normals for each face
  vec3 face_normals[4];

  for (int j=0; j<4; j++) {
    vec3 v0 = this->verts[(j+1)%4]->pos();
    vec3 v1 = this->verts[(j+2)%4]->pos();
    vec3 v2 = this->verts[(j+3)%4]->pos();
    vec3 normal = normalize(cross(v1-v0,v2-v0));

    // make sure normal faces 4th (opposite) vertex
    vec3 v3 = this->verts[(j+0)%4]->pos();
    vec3 v3_dir = normalize(v3 - v0);
    if(dot(v3_dir, normal) > 0)
      normal *= -1;

    face_normals[j] = normal;
  }
  //now compute the 6 dihedral angles between each pair of faces
  for (int j=0; j<4; j++)
    for (int k=j+1; k<4; k++) {
      double dot_product = dot(face_normals[j], face_normals[k]);
      dot_product = std::min(1.,std::max(dot_product,-1.));

      double dihedral_angle = 180.0 - acos(dot_product) * 180.0 / PI;
      dihedral_angle = std::min(180.,std::max(0.,dihedral_angle));

      if(dihedral_angle > max)
        max = (float)dihedral_angle;
    }
  return max;
}

//===================================================
//  tet_volume()
//
// Helper function to compute the oriented volume
// of a tet, identified by its 4 vertices.
//===================================================
double Tet::volume() const
{
  vec3 a = verts[0]->pos();
  vec3 b = verts[1]->pos();
  vec3 c = verts[2]->pos();
  vec3 d = verts[3]->pos();

  return dot(a - d, cross(b-d, c-d)) / 6.0;
}

//===================================================
//  contains()
//
// Returns true if the given vertex is one of the 
// four vertices composing this tetrahedron.
//===================================================
bool Tet::contains(Vertex *v) const
{
  return verts[0] == v || 
         verts[1] == v || 
         verts[2] == v ||
         verts[3] == v;
}

} // namespace cleaver
