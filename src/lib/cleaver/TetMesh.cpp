//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- TetMesh Class
//
// Primary Author: Josh Levine (jlevine@sci.utah.edu)
// Secondary Author: Jonathan Bronson (bronson@sci.utah.ed)
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

#include "TetMesh.h"
#include "BoundingBox.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <queue>
#include <deque>
#include <set>
#include <algorithm>
#include <exception>
#include <stdint.h>
#include <jsoncpp/json.h>
#include "Util.h"
#include "Matlab.h"
#include "Status.h"



using namespace std;

#ifndef PI
#define PI 3.14159265
#endif

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif


namespace cleaver
{
  // order of vertices for each face
  // the way this is ordered, a tet doesn't need vertex pointers directly
  // it can get a unique ordered list by going dereferencing hf's and their he
  int VERT_FACE_LIST[4][3] = {
    {1,2,3},    // 0-face
    {2,0,3},    // 1-face
    {3,0,1},    // 2-face
    {0,2,1}     // 3-face
  };


  TetMesh::TetMesh() : imported(false), time(0)
  {
  }

  TetMesh::TetMesh(BoundingBox b) : imported(false), time(0), bounds(b)
  {
  }

  TetMesh::TetMesh(const std::vector<Vertex*> &verts, const std::vector<Tet*> &tets) :
    verts(verts), tets(tets), imported(false), time(0)
  {
    computeBounds();
  }

  TetMesh::~TetMesh() {
    // Delete of half edges in map of vert pairs
    for (auto & x : halfEdges)
    {
      x.second->halfFaces.clear();
      delete x.second;
    }

    // delete tets verts, faces, etc
    for (size_t f = 0; f < faces.size(); f++) {
      delete faces[f];
    }

    for (size_t v = 0; v < verts.size(); v++) {
      delete verts[v];
    }
    for (size_t t = 0; t < tets.size(); t++) {
      delete tets[t];
    }

    verts.clear();
    faces.clear();
    tets.clear();
    halfFaces.clear();
  }

  void TetMesh::computeBounds()
  {
    vec3 mincorner;
    vec3 maxcorner;

    for(size_t v=0; v < verts.size(); v++)
    {
      Vertex *vertex = verts[v];

      mincorner.x = std::min(mincorner.x, vertex->pos().x);
      mincorner.y = std::min(mincorner.y, vertex->pos().y);
      mincorner.z = std::min(mincorner.z, vertex->pos().z);

      maxcorner.x = std::max(maxcorner.x, vertex->pos().x);
      maxcorner.y = std::max(maxcorner.y, vertex->pos().y);
      maxcorner.z = std::max(maxcorner.z, vertex->pos().z);
    }

    bounds = BoundingBox(mincorner, maxcorner - mincorner);
  }

  void TetMesh::updateBounds(Vertex *vertex)
  {
    vec3 mincorner = bounds.origin;
    vec3 maxcorner = bounds.origin + bounds.size;

    mincorner.x = std::min(mincorner.x, vertex->pos().x);
    mincorner.y = std::min(mincorner.y, vertex->pos().y);
    mincorner.z = std::min(mincorner.z, vertex->pos().z);

    maxcorner.x = std::max(maxcorner.x, vertex->pos().x);
    maxcorner.y = std::max(maxcorner.y, vertex->pos().y);
    maxcorner.z = std::max(maxcorner.z, vertex->pos().z);

    bounds = BoundingBox(mincorner, maxcorner - mincorner);
  }

  size_t TetMesh::fixVertexWindup(bool verbose) {
    if (verbose) std::cout <<
      "Fixing Vertex wind-up..." << std::endl;
    size_t count = 0;
    Status s(tets.size());
    std::vector<Tet*>::iterator iter = tets.begin();
    // loop over all tets in the mesh
    while(iter != tets.end()) {
      Tet* tet = *iter;
      vec3 v1 = tet->verts[0]->pos();
      vec3 v2 = tet->verts[1]->pos();
      vec3 v3 = tet->verts[2]->pos();
      vec3 v4 = tet->verts[3]->pos();
      //check the wind up.
      if ((v4 - v1).dot((v2 - v1).cross(v3 - v1)) < 0.f) {
        //re-order since v4 is on the wrong side.
        Vertex* tmp = tet->verts[2];
        tet->verts[2] = tet->verts[3];
        tet->verts[3] = tmp;
        count++;
      }
      iter++;
      if (verbose)
        s.printStatus();
    }
    if (verbose)
      s.done();

    if (verbose) {
      std::cout << "Fixed " << count << " Tet vertex wind-ups." << std::endl;
    }
    return count;
  }

  //===================================================
  // writeOff()
  //
  // Public method that writes the mesh
  // in the geomview off file format.
  //===================================================
  //void TetMesh::writeOff(const string &filename)
  //{
  //    // to implement
  //}


  static float INTERFACE_COLORS[12][3] = {
    {141/255.0f, 211/255.0f, 199/255.0f},
    {255/255.0f, 255/255.0f, 179/255.0f},
    {190/255.0f, 186/255.0f, 218/255.0f},
    {251/255.0f, 128/255.0f, 114/255.0f},
    {128/255.0f, 177/255.0f, 211/255.0f},
    {253/255.0f, 180/255.0f, 98/255.0f},
    {179/255.0f, 222/255.0f, 105/255.0f},
    {252/255.0f, 205/255.0f, 229/255.0f},
    {217/255.0f, 217/255.0f, 217/255.0f},
    {188/255.0f, 128/255.0f, 189/255.0f},
    {204/255.0f, 235/255.0f, 197/255.0f}
  };


  //===================================================
  // writeStencilPly()
  //  This is a debugging function that creates a mesh
  // of all the stencil faces of a mesh.
  //===================================================
  void TetMesh::writeStencilPly(const std::string &filename, bool verbose)
  {
    //-----------------------------------
    //           Initialize
    //-----------------------------------
    if(verbose)
      cout << "Writing mesh ply file: " << filename + ".ply" << endl;
    ofstream file((filename + ".ply").c_str());

    size_t face_count = 4*tets.size();
    size_t vertex_count = 3*face_count;

    //-----------------------------------
    //           Write Header
    //-----------------------------------
    file << "ply" << endl;
    file << "format ascii 1.0" << endl;
    file << "element vertex " << vertex_count << endl;
    file << "property float x " << endl;
    file << "property float y " << endl;
    file << "property float z " << endl;
    file << "element face " << face_count << endl;
    file << "property list uchar int vertex_index" << endl;
    file << "property uchar red" << endl;
    file << "property uchar green" << endl;
    file << "property uchar blue" << endl;
    file << "end_header" << endl;

    //-----------------------------------
    //         Write Vertex List
    //-----------------------------------
    for(size_t t=0; t < tets.size(); t++)
    {
      Tet *tet = this->tets[t];

      for(int f=0; f < 4; f++)
      {
        Vertex *v1 = tet->verts[(f+0)%4];
        Vertex *v2 = tet->verts[(f+1)%4];
        Vertex *v3 = tet->verts[(f+2)%4];

        file << v1->pos().x << " " << v1->pos().y << " " << v1->pos().z << endl;
        file << v2->pos().x << " " << v2->pos().y << " " << v2->pos().z << endl;
        file << v3->pos().x << " " << v3->pos().y << " " << v3->pos().z << endl;
      }
    }



    //-----------------------------------
    //         Write Face List
    //-----------------------------------
    int idx = 0;
    for(size_t t=0; t < tets.size(); t++)
    {
      Tet *tet = this->tets[t];

      for(int f=0; f < 4; f++)
      {
        // output 3 vertices
        file << "3 " << (idx + 0) << " " << (idx + 1) << " " << (idx + 2) << " ";
        idx += 3;

        // output 3 color components
        file << (int)(255*INTERFACE_COLORS[(int)tet->mat_label][0]) << " ";
        file << (int)(255*INTERFACE_COLORS[(int)tet->mat_label][1]) << " ";
        file << (int)(255*INTERFACE_COLORS[(int)tet->mat_label][2]) << endl;
      }
    }

    // end with a single blank line
    file << endl;

    //-----------------------------------
    //          Close  File
    //-----------------------------------
    file.close();
  }

  //===================================================
  // vec3 comparator
  //
  //===================================================
  class vec3_compare {
    public:
      bool operator()(const vec3 &a, const vec3 &b) const {
        if ((a.x < b.x) && (b.x - a.x) > 1e-9) return true;
        else if ((a.x > b.x) && (a.x - b.x) > 1e-9) return false;
        if ((a.y < b.y) && (b.y - a.y) > 1e-9) return true;
        else if ((a.y > b.y) && (a.y - b.y) > 1e-9) return false;
        if ((a.z < b.z) && (b.z - a.z) > 1e-9) return true;
        else if ((a.z > b.z) && (a.z - b.z) > 1e-9) return false;
        return false;
      }
  };
  typedef std::map< const vec3, size_t, vec3_compare > VertMap;

  //===================================================
  // writePly()
  //
  // Public method that writes the surface mesh
  // in the PLY triangle file format.
  //===================================================
  void TetMesh::writePly(const std::string &filename, bool verbose)
  {
    //-----------------------------------
    //           Initialize
    //-----------------------------------
    if(verbose)
      cout << "Writing mesh ply file: " << filename + ".ply" << endl;
    ofstream file((filename + ".ply").c_str());

    std::vector<size_t> interfaces;
    std::vector<size_t> colors;
    std::vector<size_t> keys;

    // determine output faces and vertices vertex counts
    for(size_t f=0; f < faces.size(); f++)
    {
      const int t1_index = static_cast<int>(faces[f]->tets[0]);
      const int t2_index = static_cast<int>(faces[f]->tets[1]);

      if(t1_index < 0 || t2_index < 0){
        continue;
      }

      Tet *t1 = this->tets[t1_index];
      Tet *t2 = this->tets[t2_index];

      if(t1->mat_label != t2->mat_label)
      {
        interfaces.push_back(f);

        const int color_key = (1 << (int)t1->mat_label) + (1 << (int)t2->mat_label);
        int color_index = -1;
        for(size_t k=0; k < keys.size(); k++)
        {
          if(keys[k] == color_key){
            color_index = static_cast<int>(k);
            break;
          }
        }
        if(color_index == -1)
        {
          keys.push_back(color_key);
          color_index = static_cast<int>(keys.size() - 1);
        }

        colors.push_back(color_index);
      }
    }

    //-----------------------------------
    //           Write Header
    //-----------------------------------
    file << "ply" << endl;
    file << "format ascii 1.0" << endl;

    //-----------------------------------
    //         Create Pruned Vertex List
    //-----------------------------------
    VertMap vert_map;
    std::vector<vec3> pruned_verts;
    size_t pruned_pos = 0;
    for(int f=0; f < interfaces.size(); f++)
    {
      Face *face = faces[interfaces[f]];

      Vertex *v1 = this->verts[face->verts[0]];
      Vertex *v2 = this->verts[face->verts[1]];
      Vertex *v3 = this->verts[face->verts[2]];

      vec3 p1 = v1->pos();
      vec3 p2 = v2->pos();
      vec3 p3 = v3->pos();

      if (!vert_map.count(p1)) {
        vert_map.insert(std::pair<vec3,size_t>(p1,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p1);
      }
      if (!vert_map.count(p2)) {
        vert_map.insert(std::pair<vec3,size_t>(p2,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p2);
      }
      if (!vert_map.count(p3)) {
        vert_map.insert(std::pair<vec3,size_t>(p3,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p3);
      }
    }
    file << "element vertex " << pruned_verts.size() << endl;
    file << "property float x " << endl;
    file << "property float y " << endl;
    file << "property float z " << endl;
    file << "element face " << interfaces.size() << endl;
    file << "property list uchar int vertex_index" << endl;
    file << "property uchar red" << endl;
    file << "property uchar green" << endl;
    file << "property uchar blue" << endl;
    file << "end_header" << endl;

    //-----------------------------------
    //         Write Vertex List
    //-----------------------------------
    for(std::vector<vec3>::iterator it = pruned_verts.begin();
        it != pruned_verts.end(); ++it) {
      file << it->x << " " << it->y << " " << it->z << std::endl;
    }

    //-----------------------------------
    //         Write Face List
    //-----------------------------------
    for(int f=0; f < interfaces.size(); f++)
    {
      //Face &face = faces[interfaces[f]];

      Face *face = faces[interfaces[f]];

      Vertex *v1 = this->verts[face->verts[0]];
      Vertex *v2 = this->verts[face->verts[1]];
      Vertex *v3 = this->verts[face->verts[2]];
      size_t i1 = vert_map.find(v1->pos())->second;
      size_t i2 = vert_map.find(v2->pos())->second;
      size_t i3 = vert_map.find(v3->pos())->second;
      // output 3 vertices
      file << "3 " << i1 << " " << i2 << " " << i3 << " ";

      // output 3 color components
      file << (int)(255*INTERFACE_COLORS[colors[f]%12][0]) << " ";
      file << (int)(255*INTERFACE_COLORS[colors[f]%12][1]) << " ";
      file << (int)(255*INTERFACE_COLORS[colors[f]%12][2]) << endl;
    }

    //-----------------------------------
    //          Close  File
    //-----------------------------------
    file.close();
  }

  std::pair<int,int> keyToPair(unsigned int key)
  {
    std::pair<int,int> labels;

    int offset = 0;
    unsigned int m = (key >> offset) & 1;

    while(!m)
    {
      offset++;
      m = (key >> offset) & 1;
    }
    labels.first = offset;

    offset++;
    m = (key >> offset) & 1;
    while(!m)
    {
      offset++;
      m = (key >> offset) & 1;
    }
    labels.second = offset;


    return labels;
  }

  //===================================================
  // writeMultiplePly()
  //
  // Public method that writes the surface mesh
  // into multiple PLY files. One for each material.
  //===================================================
  void TetMesh::writeMultiplePly(const vector<std::string> &inputs, const std::string &filename, bool verbose)
  {
    //-----------------------------------
    //           Initialize
    //-----------------------------------
    std::vector<std::vector<size_t> > meshes;
    std::vector<size_t> interfaces;
    std::vector<size_t> colors;
    std::vector<size_t> keys;

    while(meshes.size() < inputs.size())
      meshes.push_back(vector<size_t>());

    // determine output faces and vertices vertex counts
    for(size_t f=0; f < faces.size(); f++)
    {
      const int t1_index = static_cast<int>(faces[f]->tets[0]);
      const int t2_index = static_cast<int>(faces[f]->tets[1]);

      if(t1_index < 0 || t2_index < 0){
        continue;
      }

      Tet *t1 = this->tets[t1_index];
      Tet *t2 = this->tets[t2_index];

      if(t1->mat_label != t2->mat_label)
      {
        interfaces.push_back(f);

        const int color_key = (1 << (int)t1->mat_label) + (1 << (int)t2->mat_label);
        int color_index = -1;
        for(size_t k=0; k < keys.size(); k++)
        {
          if(keys[k] == color_key){
            color_index = static_cast<int>(k);
            break;
          }
        }
        if(color_index == -1)
        {
          keys.push_back(color_key);
          color_index = static_cast<int>(keys.size() - 1);
        }

        colors.push_back(color_index);


        if(meshes.size() < keys.size()){
          meshes.push_back(vector<size_t>());
        }
        meshes[color_index].push_back(f);

      }
    }


    for(size_t m=0; m < meshes.size(); m++)
    {
      if(meshes[m].empty())
        continue;

      std::pair<int,int> mats = keyToPair(static_cast<unsigned int>(keys[m]));

      const int mat1 = mats.first;
      const int mat2 = mats.second;

      stringstream fns;
      fns << "interface." << mat1 << "-" << mat2 << ".ply";
      string filename = fns.str();
      ofstream file(filename.c_str());

      if(verbose)
        cout << "Writing mesh ply file: " << filename << endl;

      //-----------------------------------
      //           Write Header
      //-----------------------------------
      file << "ply" << endl;
      file << "format ascii 1.0" << endl;
      file << "element vertex " << 3*meshes[m].size() << endl;
      file << "property float x " << endl;
      file << "property float y " << endl;
      file << "property float z " << endl;
      file << "element face " << meshes[m].size() << endl;
      file << "property list uchar int vertex_index" << endl;
      file << "property uchar red" << endl;
      file << "property uchar green" << endl;
      file << "property uchar blue" << endl;
      file << "end_header" << endl;

      //-----------------------------------
      //         Write Vertex List
      //-----------------------------------
      for(size_t f=0; f < meshes[m].size(); f++)
      {
        Face *face = faces[meshes[m][f]];

        Vertex *v1 = this->verts[face->verts[0]];
        Vertex *v2 = this->verts[face->verts[1]];
        Vertex *v3 = this->verts[face->verts[2]];

        file << v1->pos().x << " " << v1->pos().y << " " << v1->pos().z << endl;
        file << v2->pos().x << " " << v2->pos().y << " " << v2->pos().z << endl;
        file << v3->pos().x << " " << v3->pos().y << " " << v3->pos().z << endl;
      }

      //-----------------------------------
      //         Write Face List
      //-----------------------------------
      for(size_t f=0; f < meshes[m].size(); f++)
      {
        // output 3 vertices
        file << "3 " << (3*f + 0) << " " << (3*f + 1) << " " << (3*f + 2) << " ";

        // output 3 color components
        file << (int)(255*INTERFACE_COLORS[m%12][0]) << " ";
        file << (int)(255*INTERFACE_COLORS[m%12][1]) << " ";
        file << (int)(255*INTERFACE_COLORS[m%12][2]) << endl;
      }

      // end with a single blank line
      file << endl;

      //-----------------------------------
      //          Close  File
      //-----------------------------------
      file.close();
    }
  }

  //===================================================
  // writeNodeEle()
  //
  // Public method that writes the mesh
  // in the TetGen node/ele file format.
  //===================================================
  void TetMesh::writeNodeEle(const string &filename, bool verbose, bool include_materials, bool include_parents)
  {
    //-----------------------------------
    //  Determine Attributes to Include
    //-----------------------------------
    int attribute_count = 0;
    if(include_materials)
      attribute_count++;
    if(include_parents)
      attribute_count++;


    //-----------------------------------
    //         Write Node File
    //-----------------------------------
    string node_filename = filename + ".node";
    if(verbose)
      cout << "Writing mesh node file: " << node_filename << endl;
    ofstream node_file(node_filename.c_str());

    //---------------------------------------------------------------------------------------------------------
    //  First line: <# of points> <dimension (must be 3)> <# of attributes> <# of boundary markers (0 or 1)>
    //---------------------------------------------------------------------------------------------------------
    node_file << "# Node count, 3 dim, no attributes, no boundary markers" << endl;
    node_file << this->verts.size() << " 3  0  0" << endl << endl;

    //-------------------------------------------------------------------------------------------
    //  Remaining lines list # of points:  <point #> <x> <y> <z> [attributes] [boundary marker]
    //-------------------------------------------------------------------------------------------
    for(size_t i=0; i < this->verts.size(); i++)
    {
      node_file << i+1 << " " << this->verts[i]->pos().x << " " << this->verts[i]->pos().y << " " << this->verts[i]->pos().z << endl;
    }

    node_file.close();


    //-----------------------------------
    //        Write Element File
    //-----------------------------------
    string elem_filename = filename + ".ele";
    if(verbose)
      cout << "Writing mesh ele file: " << elem_filename << endl;
    ofstream elem_file(elem_filename.c_str());

    //---------------------------------------------------------------------------
    //  First line: <# of tetrahedra> <nodes per tetrahedron> <# of attributes>
    //--------------------------------------------------------------------------
    elem_file << "# Tet count, verts per tet, attribute count" << endl;
    elem_file << this->tets.size() << " 4 " << attribute_count << endl << endl;

    //-----------------------------------------------------------------------------------------------------------
    //  Remaining lines list of # of tetrahedra:  <tetrahedron #> <node> <node> <node> <node> ... [attributes]
    //-----------------------------------------------------------------------------------------------------------
    for(size_t i=0; i < this->tets.size(); i++)
    {
      elem_file << i+1;
      for(int v=0; v < 4; v++)
        elem_file << " " << this->tets[i]->verts[v]->tm_v_index + 1;
      if(include_materials)
        elem_file << " " << this->tets[i]->mat_label + 1;
      if(include_parents)
        elem_file << " " << this->tets[i]->parent + 1;
      elem_file << endl;
    }

    elem_file.close();
  }


  //===================================================
  // writePtsEle()
  //
  // Public method that writes the mesh
  // in the SciRun pts/ele file format.
  //===================================================
  void TetMesh::writePtsEle(const std::string &filename, bool verbose)
  {
    //-----------------------------------
    //         Create Pts File
    //-----------------------------------
    string pts_filename = filename + ".pts";
    if(verbose)
      cout << "Writing mesh pts file: " << pts_filename << endl;
    ofstream pts_file(pts_filename.c_str());

    //-------------------------------------------------------------------------------------------
    //  Write each line of file <x> <y> <z>
    //-------------------------------------------------------------------------------------------
    for(size_t i=0; i < this->verts.size(); i++)
    {
      pts_file << this->verts[i]->pos().x << " " << this->verts[i]->pos().y << " " << this->verts[i]->pos().z << endl;
    }
    pts_file.close();


    //-----------------------------------
    //        Create Element File
    //-----------------------------------
    string elem_filename = filename + ".elem";
    if(verbose)
      cout << "Writing mesh elem file: " << elem_filename << endl;
    ofstream elem_file(elem_filename.c_str());


    //-----------------------------------------------------------------------------------------------------------
    //  Write each line <node> <node> <node> <node>
    //-----------------------------------------------------------------------------------------------------------
    for(size_t i=0; i < this->tets.size(); i++)
    {
      elem_file << this->tets[i]->verts[0]->tm_v_index + 1 << " ";
      elem_file << this->tets[i]->verts[1]->tm_v_index + 1 << " ";
      elem_file << this->tets[i]->verts[2]->tm_v_index + 1 << " ";
      elem_file << this->tets[i]->verts[3]->tm_v_index + 1 << endl;
    }
    elem_file.close();

    //-----------------------------------
    //        Create Material File
    //-----------------------------------
    string mat_filename = filename + ".txt";
    cout << "Writing mesh material file: " << mat_filename << endl;
    ofstream mat_file(mat_filename.c_str());
    for(size_t i=0; i < this->tets.size(); i++)
    {
      mat_file << this->tets[i]->mat_label + 1 << endl;
    }
    mat_file.close();
  }


  //  If create is set to false, no vertex is created if one is missing
  //-----------------------------------------------------------------------------------
  HalfEdge* TetMesh::halfEdgeForVerts(Vertex *v1, Vertex *v2)
  {
    std::pair<int,int> key = std::make_pair(v1->tm_v_index,v2->tm_v_index);
    std::map<std::pair<int,int>, HalfEdge*>::iterator res = halfEdges.find(key);
    HalfEdge *half_edge = nullptr;

    // create new one if necessary
    if (res == halfEdges.end())
    {
      half_edge = new HalfEdge(v1->dual && v2->dual);
      halfEdges[key] = half_edge;
    }
    // or return existing one
    else
    {
      half_edge = res->second;
    }


    return half_edge;
  }

  void TetMesh::constructFaces()
  {
    //---------------------------------------------------------
    // If faces already computed once, clean up old information
    //---------------------------------------------------------
    if(!faces.empty()){

      // Delete the individual faces
      for(size_t f=0; f < faces.size(); f++)
        delete faces[f];

      // Clear the vector
      faces.clear();
    }

    // reset tet face indices
    for(size_t t=0; t < this->tets.size(); t++)
    {
      for(int f=0; f < FACES_PER_TET; f++)
      {
        this->tets[t]->faces[f] = nullptr;
        this->tets[t]->tets[f] = nullptr;
      }
    }

    bool valid = isValid();

    //-----------------------------------
    // Obtain Tet-Tet Adjacency
    //-----------------------------------
    int nFaces = 0;
    for(size_t i=0; i < this->tets.size(); i++)
    {
      // look for a tet sharing three verts opposite vert[j]
      for (int f=0; f < FACES_PER_TET; f++)
      {
        // if information for adjacent tet is null, attempt to fill it in
        if (this->tets[i]->tets[f] == nullptr)
        {
          // first grab the three vertices corresponding to the face[f] opposite vertex j
          Vertex *v0 = this->tets[i]->verts[(f+1) % FACES_PER_TET];
          Vertex *v1 = this->tets[i]->verts[(f+2) % FACES_PER_TET];
          Vertex *v2 = this->tets[i]->verts[(f+3) % FACES_PER_TET];
          bool found_adjacent = false;

          // make sure the 3 vertices are unique
          if(v0 == v1 || v1 == v2 || v2 == v0)
          {
            std::cout << "Degenerate Tet found while building adjacency. Terminating." << std::endl;
            exit(-7);
          }

          // search over adjacent tets touching these verts
          for (size_t j=0; j < v0->tets.size(); j++)
          {
            Tet *tet = v0->tets[j];
            if(tet == nullptr){
              std::cout << "PROBLEM! nullptr TET" << std::endl;
            }

            // Skip self
            if(tet == this->tets[i])
              continue;


            // check if it is adjacent, i.e. has v1 and v2
            int shared_count = 0;
            for (int l=0; l < 4; l++){
              if (tet->verts[l] == v1 || tet->verts[l] == v2 || tet->verts[l] == v0)
                shared_count++;
            }

            // Sanity check (To remove later for optimization)
            if(shared_count == 4){
              std::cout << "HUGE PROBLEM building adjacency. Two tets share all four faces!!" << std::endl;
              exit(9);
            }

            // if match found
            if (shared_count == 3)
            {
              // new shared face found
              found_adjacent = true;
              nFaces++;

              //--------------------
              //  set for this tet
              //--------------------
              this->tets[i]->tets[f] = tet;


              //----------------------
              // set for neighbor tet
              //----------------------
              for (int m=0; m<4; m++){  // J.R.B. 3/25/13  (changed index to m=0 to 4, was previously m=1 to 4, why?

                // If we find it
                if(tet->verts[m] != v0 && tet->verts[m] != v1 && tet->verts[m] != v2)
                {
                  // make sure we're not overwriting a value that's already written
                  if(tet->tets[m] != nullptr)
                  {
                    if(tet->tets[m] == this->tets[i])
                      std::cout << "ALREADY SET! We're Actually SAFE" << std::endl;
                    else
                    {
                      Tet *tet1 = this->tets[i];
                      Tet *tet2 = tet;
                      Tet *tet3 = tet->tets[m];

                      std::cout << "overwriting a tet!! Aborting." << std::endl;
                      std::cout << "The three tets that share this face are: " << std::endl;
                      std::cout << "Tet1 (" << tet1 << "): {"
                        << "v1(" << (int)tet1->verts[0]->order() << "), "
                        << "v2(" << (int)tet1->verts[1]->order() << "), "
                        << "v3(" << (int)tet1->verts[2]->order() << "), "
                        << "v4(" << (int)tet1->verts[3]->order() << ")} "
                        << " parent = " << tet1->parent
                        << std::endl;
                      std::cout << "Tet2 (" << tet2 << "): {"
                        << "v1(" << (int)tet2->verts[0]->order() << "), "
                        << "v2(" << (int)tet2->verts[1]->order() << "), "
                        << "v3(" << (int)tet2->verts[2]->order() << "), "
                        << "v4(" << (int)tet2->verts[3]->order() << ")}"
                        << " parent = " << tet2->parent
                        << std::endl;
                      std::cout << "Tet3 (" << tet3 << "): "
                        << "v1(" << (int)tet3->verts[0]->order() << "), "
                        << "v2(" << (int)tet3->verts[1]->order() << "), "
                        << "v3(" << (int)tet3->verts[2]->order() << "), "
                        << "v4(" << (int)tet3->verts[3]->order() << ")}"
                        << " parent = " << tet3->parent
                        << std::endl;
					  //where Cleaver exits when padding is included
                      exit(0);
                    }
                  }

                  // set it
                  tet->tets[m] = this->tets[i];
                  break;
                }
              }

              // done searching adjacent tets
              break;
            }
          }

          //if there is no face, this is a border face, up the face count
          if (!found_adjacent) {
            nFaces++;
          }
        }
      }
    }

    //----------------------------
    //  Allocate and Fill Faces
    //----------------------------
    bool doneMakingFaces = false;
    //faces = new Face[nFaces];
    faces.resize(nFaces, 0);
    int face_count = 0;

    // Loop over every tet in mesh
    for(size_t i=0; i < this->tets.size(); i++)
    {
      // Loop over face for current tet
      for(int j=0; j < FACES_PER_TET; j++)
      {
        if(face_count >= nFaces){
          std::cout << "PROBLEM with Face Adjacency Construction" << std::endl;
          exit(8);
        }

        Face *face = faces[face_count] = new Face();

        // Face Is Shared?
        if(this->tets[i]->tets[j] && this->tets[i]->faces[j] == nullptr)
        {
          // make a new face
          face->tets[0] = this->tets[i]->tm_index;
          face->face_index[0] = static_cast<int>(j);

          face->verts[0] = this->tets[i]->verts[(j+1)%4]->tm_v_index;
          face->verts[1] = this->tets[i]->verts[(j+2)%4]->tm_v_index;
          face->verts[2] = this->tets[i]->verts[(j+3)%4]->tm_v_index;

          // find the face that had i
          int shared_face = -1;
          for (int k=0; k < 4; k++)  // JRB 3/25/13 changed k=1 to 4 to k=0 to 4, not sure why k=0 skipped before
          {
            if (this->tets[i]->tets[j]->tets[k] == this->tets[i])
              shared_face = k;
          }
          if(shared_face == -1){
            std::cout << "BAD SHARED FACE" << std::endl;
            std::cout << "t1 = " << this->tets[i]->tm_index << std::endl;
            std::cout << "t2 = " << this->tets[i]->tets[j]->tm_index << std::endl;
            std::cout << "t1 verts: ["
              << this->tets[i]->verts[0]->tm_v_index << ", "
              << this->tets[i]->verts[1]->tm_v_index << ", "
              << this->tets[i]->verts[2]->tm_v_index << ", "
              << this->tets[i]->verts[3]->tm_v_index << "]" << std::endl;

            std::cout << "t2 verts: ["
              << this->tets[i]->tets[j]->verts[0]->tm_v_index << ", "
              << this->tets[i]->tets[j]->verts[1]->tm_v_index << ", "
              << this->tets[i]->tets[j]->verts[2]->tm_v_index << ", "
              << this->tets[i]->tets[j]->verts[3]->tm_v_index << "]" << std::endl;
            std::cout << "tets around t1: ["
              << this->tets[i]->tets[0]->tm_index << ", "
              << this->tets[i]->tets[1]->tm_index << ", "
              << this->tets[i]->tets[2]->tm_index << ", "
              << this->tets[i]->tets[3]->tm_index << "]" << std::endl;
            std::cout << "tets around t2: ["
              << this->tets[i]->tets[j]->tets[0]->tm_index << ", "
              << this->tets[i]->tets[j]->tets[1]->tm_index << ", "
              << this->tets[i]->tets[j]->tets[2]->tm_index << ", "
              << this->tets[i]->tets[j]->tets[3]->tm_index << "]" << std::endl;
            std::cout << endl;
            exit(0);
          }

          face->tets[1] = this->tets[i]->tets[j]->tm_index;
          face->face_index[1] = shared_face;

          this->tets[i]->faces[j] = faces[face_count];
          this->tets[i]->tets[j]->faces[shared_face] = faces[face_count];

          face_count++;
        }
        // Boundary Face
        else if(this->tets[i]->tets[j] == nullptr){
          face->tets[0] = static_cast<int>(i);
          face->face_index[0] = static_cast<int>(j);
          face->tets[1] = -1;
          face->verts[0] = this->tets[i]->verts[(j+1)%4]->tm_v_index;
          face->verts[1] = this->tets[i]->verts[(j+2)%4]->tm_v_index;
          face->verts[2] = this->tets[i]->verts[(j+3)%4]->tm_v_index;
          this->tets[i]->faces[j] = faces[face_count];
          face_count++;
        }

        if(face_count == nFaces){
          doneMakingFaces = true;
          break;
        }
      }

      if(doneMakingFaces)
        break;
    }

    if(face_count != nFaces)
      std::cout << "WARNING: face_count(" << face_count << ") differs from nFaces(" << nFaces << ")"  << std::endl;

    //-------------------
    //   Normal Loop
    //-------------------
    for(size_t i=0; i < faces.size(); i++)
    {
      Face *face = faces[i];
      Vertex *v0 = this->verts[face->verts[0]];
      Vertex *v1 = this->verts[face->verts[1]];
      Vertex *v2 = this->verts[face->verts[2]];

      vec3 e10 = v1->pos() - v0->pos();
      vec3 e20 = v2->pos() - v0->pos();
      face->normal = normalize(e10.cross(e20));
      vec3 bary = (1.0/3.0)*(v0->pos() + v1->pos() + v2->pos());

      // flip the normal if it points towards tets[0]
      Tet *tet = this->tets[face->tets[0]];
      Vertex *vert = tet->verts[face->face_index[0]];
      vec3 pos = vert->pos();
      vec3 dir = pos - bary;
      if (dot(dir, face->normal) > 0) {
        //swap normal direction
        face->normal = -1*face->normal;
      }
    }
  }

  void TetMesh::computeAngles()
  {
    double min = 180;
    double max = 0;
    Status status(this->tets.size());
    std::ofstream debug_dump("debug.dump", ofstream::out);
    debug_dump << "{ \"badtets\": [" << std::endl;
    int bad_tets = 0;

    for (size_t i=0; i < this->tets.size(); i++)
    {
      status.printStatus();
      Tet *t = this->tets[i];

      //each tet has 6 dihedral angles between pairs of faces
      //compute the face normals for each face
      vec3 face_normals[4];

      for (int j=0; j<4; j++) {
        vec3 v0 = t->verts[(j+1)%4]->pos();
        vec3 v1 = t->verts[(j+2)%4]->pos();
        vec3 v2 = t->verts[(j+3)%4]->pos();
        vec3 normal = normalize(cross(v1-v0,v2-v0));

        // make sure normal faces 4th (opposite) vertex
        vec3 v3 = t->verts[(j+0)%4]->pos();
        vec3 v3_dir = normalize(v3 - v0);
        if(dot(v3_dir, normal) > 0)
          normal *= -1;

        face_normals[j] = normal;
      }

      double local_min = 180., local_max = 0.;

      //now compute the 6 dihedral angles between each pair of faces
      for (int j=0; j<4; j++) {
        for (int k=j+1; k<4; k++) {
          double dot_product = dot(face_normals[j], face_normals[k]);
          if (dot_product < -1) {
            dot_product = -1;
          } else if (dot_product > 1) {
            dot_product = 1;
          }

          double dihedral_angle = 180.0 - acos(dot_product) * 180.0 / PI;
          if (local_min > dihedral_angle) local_min = dihedral_angle;
          if (local_max < dihedral_angle) local_max = dihedral_angle;
          if (dihedral_angle < min) min = dihedral_angle;
          if (dihedral_angle > max) max = dihedral_angle;
        }
      }

      // DEBUG for flat tets
      if(local_max == 180){
        if (bad_tets > 0)
          debug_dump << "," << std::endl;
        bad_tets++;

        Json::Value tet = tet_to_json(t, this, false);
        tet["parent"] = static_cast<int>(t->parent);
        debug_dump << tet << std::endl;
        t->flagged = true;
        std::cout << "ERROR, TET #: " << i << std::endl;
        std::cout << "bad tet, vert orders { "
          << (int)t->verts[0]->order() << ", "
          << (int)t->verts[1]->order() << ", "
          << (int)t->verts[2]->order() << ", "
          << (int)t->verts[3]->order() << " } " << std::endl;
        std::cout << "\t vertex positions: {"
          << t->verts[0]->pos() << ", "
          << t->verts[1]->pos() << ", "
          << t->verts[2]->pos() << ", "
          << t->verts[3]->pos() << "} " << std::endl;
        std::cout << "\t exterior?: {"
          << t->verts[0]->isExterior << ", "
          << t->verts[1]->isExterior << ", "
          << t->verts[2]->isExterior << ", "
          << t->verts[3]->isExterior << "} " << std::endl;
      }
      // END DEBUG
    }
    debug_dump << "]}" << std::endl;
    debug_dump.close();
    status.done();
    if (bad_tets > 0) {
      std::cout << "Errors: " << bad_tets << " degenerate tets." << std::endl;
    }

    min_angle = min;
    max_angle = max;
  }

  void TetMesh::writeInfo(const string &filename, bool verbose)
  {
    //-----------------------------------
    //         Create Pts File
    //-----------------------------------
    std::string info_filename = filename + ".info";
    if(verbose)
      std::cout << "Writing info file: " << info_filename << std::endl;
    std::ofstream info_file(info_filename.c_str());

    info_file.precision(8);
    info_file << "min_angle = " << min_angle << std::endl;
    info_file << "max_angle = " << max_angle << std::endl;
    info_file << "tet_count = " << tets.size() << std::endl;
    info_file << "vtx_count = " << verts.size() << std::endl;
    info_file << "mesh time = " << time << "s" << std::endl;

    info_file.close();
  }

  //============================================================
  // writeMesh()
  //
  // Public method to write mesh to file using desired
  // mesh format. The appropriate file writer is called.
  //============================================================
  void TetMesh::writeMesh(const std::string &filename, MeshFormat format, bool verbose)
  {

    switch(format) {
    case cleaver::Tetgen:
      writeNodeEle(filename, verbose);
      break;
    case cleaver::Scirun:
      writePtsEle(filename, verbose);
      break;
    case cleaver::Matlab:
      writeMatlab(filename, verbose);
      break;
    case  cleaver::VtkUSG:
      writeVtkUnstructuredGrid(filename, verbose);
      break;
    case  cleaver::VtkPoly:
      writeVtkPolyData(filename, verbose);
      break;
    case  cleaver::PLY:
      writePly(filename, verbose);
      break;
    default: {
               std::cerr << "Unsupported Mesh Format. " << std::endl;
               break;
             }
    }
  }

  void TetMesh::writeVtkPolyData(const std::string &filename, bool verbose)
  {
    std::string path = filename.substr(0,filename.find_last_of("/")+1);
    std::string name = filename.substr(filename.find_last_of("/")+1,filename.size() - 1);
    if (path.empty()) {
      char cCurrentPath[FILENAME_MAX];
      if(GetCurrentDir(cCurrentPath, sizeof(cCurrentPath))){}
      cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
      path = std::string(cCurrentPath) + "/";
    }
    // get the number of files/mats
    std::vector<std::ofstream*> output;
    std::vector<size_t> numTetsPerMat;
    for(size_t i = 0; i < this->tets.size(); i++) {
      size_t label = tets.at(i)->mat_label;
      if (label + 1 > numTetsPerMat.size())
        numTetsPerMat.resize(label+1);
      numTetsPerMat.at(label)++;
    }
    size_t num = 0;
    std::vector<std::string> filenames;
    while (numTetsPerMat.size() != filenames.size()) {
      std::stringstream ss;
      ss << path << name << num++ << ".vtk" ;
      filenames.push_back(ss.str());
      std::cout << "\t" << ss.str() << std:: endl;
    }
    if(verbose) {
      std::cout << "Writing VTK mesh files(tets): \n";
      for(size_t i = 0; i < filenames.size(); i++)
        std::cout << "\t" << filenames.at(i) << std::endl;
    }
    //-----------------------------------
    //         Create Pruned Vertex List
    //-----------------------------------
    std::vector<VertMap> vert_maps;
    vert_maps.resize(numTetsPerMat.size());
    std::vector<std::vector<vec3> > pruned_verts;
    pruned_verts.resize(numTetsPerMat.size());
    std::vector<size_t> pruned_pos;
    pruned_pos.resize(numTetsPerMat.size());
    for(size_t i = 0; i < numTetsPerMat.size();i++)
      pruned_pos[i] = 0;
    for(size_t t=0; t < this->tets.size(); t++) {
      Tet* tet = this->tets[t];
      size_t label = tet->mat_label;

      Vertex *v1 = tet->verts[0];
      Vertex *v2 = tet->verts[1];
      Vertex *v3 = tet->verts[2];
      Vertex *v4 = tet->verts[3];

      vec3 p1 = v1->pos();
      vec3 p2 = v2->pos();
      vec3 p3 = v3->pos();
      vec3 p4 = v4->pos();

      if (!vert_maps[label].count(p1)) {
        vert_maps[label].insert(std::pair<vec3,size_t>(p1,pruned_pos[label]));
        pruned_pos[label]++;
        pruned_verts[label].push_back(p1);
      }
      if (!vert_maps[label].count(p2)) {
        vert_maps[label].insert(std::pair<vec3,size_t>(p2,pruned_pos[label]));
        pruned_pos[label]++;
        pruned_verts[label].push_back(p2);
      }
      if (!vert_maps[label].count(p3)) {
        vert_maps[label].insert(std::pair<vec3,size_t>(p3,pruned_pos[label]));
        pruned_pos[label]++;
        pruned_verts[label].push_back(p3);
      }
      if (!vert_maps[label].count(p4)) {
        vert_maps[label].insert(std::pair<vec3,size_t>(p4,pruned_pos[label]));
        pruned_pos[label]++;
        pruned_verts[label].push_back(p4);
      }
    }
    //-----------------------------------
    //         Write Headers
    //-----------------------------------
    for(size_t i=0; i < numTetsPerMat.size(); i++) {
      output.push_back(new std::ofstream(filenames.at(i).c_str()));
      *output.at(i) << "# vtk DataFile Version 2.0\n";
      *output.at(i) << filenames.at(i) << " Tet Mesh\n";
      *output.at(i) << "ASCII\n";
      *output.at(i) << "DATASET POLYDATA\n";
      *output.at(i) << "POINTS " << pruned_verts[i].size() << " float\n";
    }
    //-----------------------------------
    //         Write Vertex List
    //-----------------------------------
    for(size_t f=0; f < numTetsPerMat.size(); f++) {
      for(size_t i=0; i < pruned_verts[f].size(); i++)
      {
        *output.at(f) << pruned_verts[f][i].x << " "
          << pruned_verts[f][i].y << " "
          << pruned_verts[f][i].z << std::endl;
      }
      size_t num_tets = numTetsPerMat.at(f);
      *output.at(f) << "POLYGONS " << num_tets*4 << " "
        << (num_tets*16) <<"\n";
    }
    //-----------------------------------
    //         Write Cell/Face List
    //-----------------------------------
    for(size_t f=0; f < this->tets.size(); f++)
    {
      Tet* t = this->tets.at(f);

      Vertex *v1 = t->verts[0];
      Vertex *v2 = t->verts[1];
      Vertex *v3 = t->verts[2];
      Vertex *v4 = t->verts[3];
      size_t i1 = vert_maps[t->mat_label].find(v1->pos())->second;
      size_t i2 = vert_maps[t->mat_label].find(v2->pos())->second;
      size_t i3 = vert_maps[t->mat_label].find(v3->pos())->second;
      size_t i4 = vert_maps[t->mat_label].find(v4->pos())->second;

      *output.at(t->mat_label) << 3 << " " << i1 <<  " " << i2 << " " << i3 << "\n";
      *output.at(t->mat_label) << 3 << " " << i2 <<  " " << i3 << " " << i4 << "\n";
      *output.at(t->mat_label) << 3 << " " << i3 <<  " " << i4 << " " << i1 << "\n";
      *output.at(t->mat_label) << 3 << " " << i4 <<  " " << i1 << " " << i2 << "\n";
    }
    //CLOSE
    for(size_t i=0; i < numTetsPerMat.size(); i++) {
      (*output.at(i)).close();
      delete output.at(i);
    }
  }

  void TetMesh::writeVtkUnstructuredGrid(const std::string &filename, bool verbose)
  {
    std::string filepath = filename + ".vtk";
    std::ofstream output(filepath.c_str(), std::ios::out | std::ios::binary);
    if(verbose)
      std::cout << "Writing mesh vtk file: " << filepath << std::endl;
    //-----------------------------------
    //         Create Pruned Vertex List
    //-----------------------------------
    VertMap vert_map;
    std::vector<vec3> pruned_verts;
    size_t pruned_pos = 0;
    for(size_t t=0; t < this->tets.size(); t++) {
      Tet* tet = this->tets[t];

      Vertex *v1 = tet->verts[0];
      Vertex *v2 = tet->verts[1];
      Vertex *v3 = tet->verts[2];
      Vertex *v4 = tet->verts[3];

      vec3 p1 = v1->pos();
      vec3 p2 = v2->pos();
      vec3 p3 = v3->pos();
      vec3 p4 = v4->pos();

      if (!vert_map.count(p1)) {
        vert_map.insert(std::pair<vec3,size_t>(p1,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p1);
      }
      if (!vert_map.count(p2)) {
        vert_map.insert(std::pair<vec3,size_t>(p2,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p2);
      }
      if (!vert_map.count(p3)) {
        vert_map.insert(std::pair<vec3,size_t>(p3,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p3);
      }
      if (!vert_map.count(p4)) {
        vert_map.insert(std::pair<vec3,size_t>(p4,pruned_pos));
        pruned_pos++;
        pruned_verts.push_back(p4);
      }
    }

    //-----------------------------------
    //         Write Header
    //-----------------------------------
    output << "# vtk DataFile Version 2.0\n";
    output << filepath << " Tet Mesh\n";
    output << "ASCII\n";
    output << "DATASET UNSTRUCTURED_GRID\n";
    output << "POINTS " << pruned_verts.size() << " float\n";
    //-----------------------------------
    //         Write Vertex List
    //-----------------------------------
    for(size_t i=0; i < pruned_verts.size(); i++)
    {
      output << pruned_verts[i].x << " "
        << pruned_verts[i].y << " "
        << pruned_verts[i].z << std::endl;
    }

    //-----------------------------------
    //         Write Cell/Face List
    //-----------------------------------
    // \todo make writing background optional
    output << "CELLS " << this->tets.size() << " " << this->tets.size()*5 << "\n";
    for(size_t f=0; f < this->tets.size(); f++)
    {
      Tet* t = this->tets.at(f);

      Vertex* v1 = t->verts[0];
      Vertex* v2 = t->verts[1];
      Vertex* v3 = t->verts[2];
      Vertex* v4 = t->verts[3];
      size_t i1 = vert_map.find(v1->pos())->second;
      size_t i2 = vert_map.find(v2->pos())->second;
      size_t i3 = vert_map.find(v3->pos())->second;
      size_t i4 = vert_map.find(v4->pos())->second;
      output << 4 << " " << i1 <<  " " << i2 << " " << i3 << " " << i4 << "\n";
    }

    output << "CELL_TYPES " << this->tets.size() << "\n";
    for(size_t f=0; f < this->tets.size(); f++)
    {
      Tet* t = this->tets.at(f);
      output << 10 << "\n";
    }

    //-----------------------------------
    //         Write Labels
    //-----------------------------------
    output << "CELL_DATA " << this->tets.size() << "\n";
    output << "SCALARS labels int 1\n";
    output << "LOOKUP_TABLE default\n";
    for(size_t f=0; f < this->tets.size(); f++)
    {
      Tet* t = this->tets.at(f);
      output << (int)t->mat_label << "\n";
    }

    //CLOSE
    output.close();
  }

  //==================================================================
  // writeMatlab()
  //
  // Public method that writes the mesh
  // in the SCIRun-Matlab file format.
  //==================================================================
  void TetMesh::writeMatlab(const std::string &filename, bool verbose)
  {
#ifdef _WIN32
#define float_t float
#endif
    //-------------------------------
    //         Create File
    //-------------------------------
    std::ofstream file((filename + ".mat").c_str(), std::ios::out | std::ios::binary);
    if(verbose)
      std::cout << "Writing mesh matlab file: " << (filename + ".mat").c_str() << std::endl;

    if(!file.is_open())
    {
      std::cerr << "Failed to create file." << std::endl;
      return;
    }
    Status status(verts.size() + 2*tets.size());

    //--------------------------------------------------------------
    //        Write Header (128 bytes)
    //
    //      Bytes   1 - 116  : Descriptive Text (116 bytes)
    //      Bytes 117 - 124  : Subsystem Offset (8 bytes)
    //      Bytes 125 - 126  : Matlab Version   ( 2 bytes)
    //      Bytes 127 - 128  : Endian Indicator ( 2 bytes)
    //--------------------------------------------------------------

    // write description
    std::string description = "MATLAB 5.0 MAT-file, SCIRun-TetMesh Created using Cleaver. SCI/Utah | http://www.sci.utah.edu";
    description.resize(116, ' ');
    file.write((char*)description.c_str(), description.length());

    // write offset
    char zeros[32] = {0};
    file.write(zeros, 8);

    // write version
    int16_t version = 0x0100;
    file.write((char*)&version, sizeof(int16_t));

    // write endian
    char endian[2] = {'I','M'};
    file.write(endian, sizeof(int16_t));

    //----------------------------------------------------------
    //  Write Containing Structure
    //  8 byte Tag for Matrix Element
    //  6 Structure SubElements as Data
    //          1 - Array Flags         (8 bytes)
    //          2 - Dimensions Array    numberOfDimensions * sizeOfDataType
    //          3 - Array Name          numberOfCharacters * sizeOfDataType
    //          4 - Field Name Length   (4 bytes)
    //          5 - Field Names         numberOfFields * FieldNameLength;
    //          6 - Node Field
    //          7 - Cell Field
    //          8 - Field Field
    //----------------------------------------------------------
    int32_t mainType  = miMATRIX;
    int32_t totalSize = 0;

    // save location, when total size known, come back and fill it in
    long totalSizeAddress = (long)file.tellp();
    totalSizeAddress += sizeof(int32_t);


    file.write((char*)&mainType, sizeof(int32_t));
    file.write((char*)&totalSize, sizeof(int32_t));

    //---------------------------------------------
    //       Write Array Flags SubElement
    //
    //   bytes 1 - 2  : undefined (2 bytes)
    //   byte  3      : flags  (1 byte)
    //   byte  4      : class  (1 byte)
    //   bytes 5 - 8  : undefined (4 bytes)
    //---------------------------------------------

    int32_t flagsType = miUINT32;
    int32_t flagsSize = 8;

    file.write((char*)&flagsType, sizeof(int32_t));
    file.write((char*)&flagsSize, sizeof(int32_t));

    int8_t flagsByte = 0;
    int8_t classByte = mxSTRUCT_CLASS;

    file.write((char*)&classByte, sizeof(int8_t));
    file.write((char*)&flagsByte, sizeof(int8_t));
    file.write(zeros, 2);
    file.write(zeros, 4);

    //---------------------------------------------
    //     Write Dimensions Array SubElement
    //
    //---------------------------------------------
    int32_t dimensionsType = miINT32;
    int32_t dimensionsSize = 8;
    int32_t dimension = 1;

    file.write((char*)&dimensionsType, sizeof(int32_t));
    file.write((char*)&dimensionsSize, sizeof(int32_t));
    file.write((char*)&dimension, sizeof(int32_t));
    file.write((char*)&dimension, sizeof(int32_t));

    //---------------------------------------------
    //     Write Array Name SubElement
    //---------------------------------------------
    int8_t  arrayName[8] = {'t','e','t','m','e','s','h','\0'};
    int32_t arrayNameType = miINT8;
    int32_t arrayNameSize = 7;

    file.write((char*)&arrayNameType, sizeof(int32_t));
    file.write((char*)&arrayNameSize, sizeof(int32_t));
    file.write((char*)arrayName,    8*sizeof(int8_t));


    //---------------------------------------------
    //  Write Field Name Length SubElement
    //---------------------------------------------
    int16_t fieldNameLengthSize = sizeof(int32_t);
    int16_t fieldNameLengthType = miINT32;
    int32_t fieldNameLengthData = 8;

    file.write((char*)&fieldNameLengthType, sizeof(int16_t));
    file.write((char*)&fieldNameLengthSize, sizeof(int16_t));
    file.write((char*)&fieldNameLengthData, sizeof(int32_t));

    //---------------------------------------------
    //  Write Field Names
    //---------------------------------------------
    int32_t fieldNamesType = miINT8;
    int32_t fieldNamesSize = 8*3;

    file.write((char*)&fieldNamesType, sizeof(int32_t));
    file.write((char*)&fieldNamesSize, sizeof(int32_t));

    strcpy(zeros, "node");
    file.write(zeros, 8*sizeof(char));
    memset(zeros, 0, 32);

    strcpy(zeros, "cell");
    file.write(zeros, 8*sizeof(char));
    memset(zeros, 0, 32);

    strcpy(zeros, "field");
    file.write(zeros, 8*sizeof(char));
    memset(zeros, 0, 32);

    //---------------------------------------------
    //  Write Field Cells
    //---------------------------------------------

    //-------------------------------
    //         Write .node
    //-------------------------------
    int32_t nodeType = miMATRIX;
    int32_t nodeSize = 0;

    long nodeSizeAddress = (long)file.tellp();
    nodeSizeAddress += sizeof(int32_t);

    file.write((char*)&nodeType, sizeof(int32_t));
    file.write((char*)&nodeSize, sizeof(int32_t));

    //--------------------------------
    //  Write Node Array flags
    //
    //   bytes 1 - 2  : undefined (2 bytes)
    //   byte  3      : flags  (1 byte)
    //   byte  4      : class  (1 byte)
    //   bytes 5 - 8  : undefined (4 bytes)
    //--------------------------------
    int32_t nodeFlagsType = miUINT32;
    int32_t nodeFlagsSize = 8;

    file.write((char*)&nodeFlagsType, sizeof(int32_t));
    file.write((char*)&nodeFlagsSize, sizeof(int32_t));

    int8_t nodeFlagsByte = 0;
    int8_t nodeClassByte = mxSINGLE_CLASS;

    file.write((char*)&nodeClassByte, sizeof(int8_t));
    file.write((char*)&nodeFlagsByte, sizeof(int8_t));
    file.write(zeros, 2);
    file.write(zeros, 4);

    //---------------------------------------------
    //   Write Node Dimensions Array
    //---------------------------------------------
    int32_t nodeDimensionType = miINT32;
    int32_t nodeDimensionSize = 8;
    int32_t nodeDimensionRows = 3;
    int32_t nodeDimensionCols = static_cast<int32_t>(verts.size());

    file.write((char*)&nodeDimensionType, sizeof(int32_t));
    file.write((char*)&nodeDimensionSize, sizeof(int32_t));
    file.write((char*)&nodeDimensionRows, sizeof(int32_t));
    file.write((char*)&nodeDimensionCols, sizeof(int32_t));

    //---------------------------------------------
    //     Write Node Array Name SubElement
    //---------------------------------------------
    int32_t nodeArrayNameType = miINT8;
    int32_t nodeArrayNameSize = 0;

    file.write((char*)&nodeArrayNameType, sizeof(int32_t));
    file.write((char*)&nodeArrayNameSize, sizeof(int32_t));

    //----------------------------------------------
    //     Write Node Pr Array Data SubElement
    //----------------------------------------------
    int32_t nodeDataType = miSINGLE;
    int32_t nodeDataSize = nodeDimensionRows*nodeDimensionCols*sizeof(float_t);
    int32_t nodePadding = (8 - (nodeDataSize % 8)) % 8;

    file.write((char*)&nodeDataType, sizeof(int32_t));
    file.write((char*)&nodeDataSize, sizeof(int32_t));

    for(size_t i=0; i < verts.size(); i++)
    {
      if (verbose) status.printStatus();
      float_t x = (float_t)verts[i]->pos().x;
      float_t y = (float_t)verts[i]->pos().y;
      float_t z = (float_t)verts[i]->pos().z;

      file.write((char*)&x, sizeof(float_t));
      file.write((char*)&y, sizeof(float_t));
      file.write((char*)&z, sizeof(float_t));
    }
    if(nodePadding)
      file.write((char*)zeros, nodePadding);
    long nodeEndAddress = (long)file.tellp();

    //-------------------------------
    //         Write .cell
    //-------------------------------
    int32_t cellType = miMATRIX;
    int32_t cellSize = 0;

    long cellSizeAddress = (long)file.tellp();
    cellSizeAddress += sizeof(int32_t);

    file.write((char*)&cellType, sizeof(int32_t));
    file.write((char*)&cellSize, sizeof(int32_t));

    //--------------------------------
    //  Write Cell Array flags
    //
    //   bytes 1 - 2  : undefined (2 bytes)
    //   byte  3      : flags  (1 byte)
    //   byte  4      : class  (1 byte)
    //   bytes 5 - 8  : undefined (4 bytes)
    //--------------------------------
    int32_t cellFlagsType = miUINT32;
    int32_t cellFlagsSize = 8;

    file.write((char*)&cellFlagsType, sizeof(int32_t));
    file.write((char*)&cellFlagsSize, sizeof(int32_t));

    int8_t cellFlagsByte = 0;
    int8_t cellClassByte = mxINT32_CLASS;

    file.write((char*)&cellClassByte, sizeof(int8_t));
    file.write((char*)&cellFlagsByte, sizeof(int8_t));
    file.write(zeros, 2);
    file.write(zeros, 4);

    //---------------------------------------------
    //   Write Cell Dimensions Array
    //---------------------------------------------
    int32_t cellDimensionType = miINT32;
    int32_t cellDimensionSize = 8;
    int32_t cellDimensionRows = 4;
    int32_t cellDimensionCols = static_cast<int32_t>(tets.size());

    file.write((char*)&cellDimensionType, sizeof(int32_t));
    file.write((char*)&cellDimensionSize, sizeof(int32_t));
    file.write((char*)&cellDimensionRows, sizeof(int32_t));
    file.write((char*)&cellDimensionCols, sizeof(int32_t));

    //---------------------------------------------
    //     Write Cell Array Name SubElement
    //---------------------------------------------
    int32_t cellArrayNameType = miINT8;
    int32_t cellArrayNameSize = 0;

    file.write((char*)&cellArrayNameType, sizeof(int32_t));
    file.write((char*)&cellArrayNameSize, sizeof(int32_t));

    //----------------------------------------------
    //     Write Cell Pr Array Data SubElement
    //----------------------------------------------
    int32_t cellDataType = miINT32;
    int32_t cellDataSize = cellDimensionRows*cellDimensionCols*sizeof(int32_t);
    int32_t cellPadding  = (8 - (cellDataSize % 8)) % 8;

    file.write((char*)&cellDataType, sizeof(int32_t));
    file.write((char*)&cellDataSize, sizeof(int32_t));

    for(size_t i=0; i < tets.size(); i++)
    {
      if (verbose) status.printStatus();
      for(int v=0; v < 4; v++){
        int32_t index = tets[i]->verts[v]->tm_v_index + 1;
        file.write((char*)&index, sizeof(int32_t));
      }
    }
    if(cellPadding)
      file.write((char*)zeros, cellPadding);
    long cellEndAddress = (long)file.tellp();

    //----------------------------------
    //         Write .field
    //----------------------------------
    int32_t fieldType = miMATRIX;
    int32_t fieldSize = 0;

    long fieldSizeAddress = (long)file.tellp();
    fieldSizeAddress += sizeof(int32_t);

    file.write((char*)&fieldType, sizeof(int32_t));
    file.write((char*)&fieldSize, sizeof(int32_t));

    //--------------------------------
    //  Write Field Array flags
    //
    //   bytes 1 - 2  : undefined (2 bytes)
    //   byte  3      : flags  (1 byte)
    //   byte  4      : class  (1 byte)
    //   bytes 5 - 8  : undefined (4 bytes)
    //--------------------------------
    int32_t fieldFlagsType = miUINT32;
    int32_t fieldFlagsSize = 8;

    file.write((char*)&fieldFlagsType, sizeof(int32_t));
    file.write((char*)&fieldFlagsSize, sizeof(int32_t));

    int8_t fieldFlagsByte = 0;
    int8_t fieldClassByte = mxUINT8_CLASS;

    file.write((char*)&fieldClassByte, sizeof(int8_t));
    file.write((char*)&fieldFlagsByte, sizeof(int8_t));
    file.write(zeros, 2);
    file.write(zeros, 4);

    //---------------------------------------------
    //   Write Field Dimensions Array
    //---------------------------------------------
    int32_t fieldDimensionType = miINT32;
    int32_t fieldDimensionSize = 8;
    int32_t fieldDimensionRows = 1;
    int32_t fieldDimensionCols = static_cast<int32_t>(tets.size());

    file.write((char*)&fieldDimensionType, sizeof(int32_t));
    file.write((char*)&fieldDimensionSize, sizeof(int32_t));
    file.write((char*)&fieldDimensionRows, sizeof(int32_t));
    file.write((char*)&fieldDimensionCols, sizeof(int32_t));

    //---------------------------------------------
    //     Write Field Array Name SubElement
    //---------------------------------------------
    int32_t fieldArrayNameType = miINT8;
    int32_t fieldArrayNameSize = 0;

    file.write((char*)&fieldArrayNameType, sizeof(int32_t));
    file.write((char*)&fieldArrayNameSize, sizeof(int32_t));

    //----------------------------------------------
    //     Write Field Pr Array Data SubElement
    //----------------------------------------------
    int32_t fieldDataType = miUINT8;
    int32_t fieldDataSize = fieldDimensionRows*fieldDimensionCols*sizeof(int8_t);
    int32_t fieldPadding  = (8 - (fieldDataSize % 8)) % 8;

    file.write((char*)&fieldDataType, sizeof(int32_t));
    file.write((char*)&fieldDataSize, sizeof(int32_t));

    for(size_t i=0; i < tets.size(); i++)
    {
      if (verbose) status.printStatus();
      unsigned char m = tets[i]->mat_label;
      file.write((char*)&m, sizeof(int8_t));
    }
    if(fieldPadding)
      file.write(zeros, fieldPadding);
    long fieldEndAddress = (long)file.tellp();
    long fileEndAddress = (long)file.tellp();

    //-------------------------------
    //  Finally, Compute Sizes and
    //         Write Them
    //-------------------------------
    totalSize = fileEndAddress -  (totalSizeAddress + sizeof(int32_t));
    nodeSize  = nodeEndAddress -  (nodeSizeAddress  + sizeof(int32_t));
    cellSize  = cellEndAddress -  (cellSizeAddress  + sizeof(int32_t));
    fieldSize = fieldEndAddress - (fieldSizeAddress + sizeof(int32_t));

    file.seekp(totalSizeAddress);
    file.write((char*)&totalSize, sizeof(int32_t));

    file.seekp(nodeSizeAddress);
    file.write((char*)&nodeSize,  sizeof(int32_t));

    file.seekp(cellSizeAddress);
    file.write((char*)&cellSize,  sizeof(int32_t));

    file.seekp(fieldSizeAddress);
    file.write((char*)&fieldSize, sizeof(int32_t));

    //-------------------------------
    //   Done
    //-------------------------------
    file.flush();
    file.close();
    if (verbose) status.done();
  }

  //===================================================================================
  // - createTet()
  //
  //  Since creating an output Tet for the mesh always involves the same procedure,
  // it is helpful to have a function dedicated to this task. The calling code will
  // pass in the 4 vertices making up the output Tet, add them to the global lists
  // if necessary, and copy adjacency information appropriately.
  //===================================================================================
  Tet* TetMesh::createTet(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, int material)
  {
    // debugging check
    if(v1 == v2 || v1 == v3 || v1 == v4 || v2 == v3 || v2 == v4 || v3 == v4)
      std::cout << "PROBLEM! Creating nullptr Tet" << std::endl;
    else if(v1 == 0 || v2 == 0 || v3 == 0 || v4 == 0)
      std::cout << "PROBLEM! Creating nullptr Tet" << std::endl;

    //----------------------------
    //  Create Tet + Add to List
    //----------------------------

    Tet *tet = new Tet(v1, v2, v3, v4, material);
    tet->tm_index = static_cast<int>(tets.size());
    tets.push_back(tet);

    //------------------------
    //   Add Verts To List
    //------------------------
    if(v1->tm_v_index < 0){
      v1->tm_v_index = static_cast<int>(verts.size());
      verts.push_back(v1);
    }
    if(v2->tm_v_index < 0){
      v2->tm_v_index = static_cast<int>(verts.size());
      verts.push_back(v2);
    }
    if(v3->tm_v_index < 0){
      v3->tm_v_index = static_cast<int>(verts.size());
      verts.push_back(v3);
    }
    if(v4->tm_v_index < 0){
      v4->tm_v_index = static_cast<int>(verts.size());
      verts.push_back(v4);
    }

    //---------------
    // Update Bounds
    //---------------
    updateBounds(v1);
    updateBounds(v2);
    updateBounds(v3);
    updateBounds(v4);

    return tet;
  }


  void TetMesh::stripMaterial(char material, bool verbose)
  {
    std::vector<Vertex*> candidate_verts;
    std::set<Vertex*>    delete_list;

    // clear old vertex and adjacency information
    for(size_t v=0; v < verts.size(); v++) {
      verts[v]->tets.clear();
      verts[v]->tm_v_index = -1;
    }
    verts.clear();

    // go through mesh and check for exterior tets
    int tet_count = 0;

    for(size_t t=0; t < tets.size(); t++) {
      Tet *tet = tets[t];

      // if not material we're stripping
      if(tet->mat_label != material) {
        for(int v=0; v < 4; v++) {
          // add this tet to the vertex
          tet->verts[v]->tets.push_back(tet);

          // add to verts list if havn't already
          if(tet->verts[v]->tm_v_index < 0){
            tet->verts[v]->tm_v_index = static_cast<int>(verts.size());
            verts.push_back(tet->verts[v]);
          }
        }

        // move tet past any deleted tets
        tet->tm_index = tet_count;
        tets[tet_count] = tet;
        tet_count++;
      }
      // if it is the material we strip
      else {
        // add vertices to list to check
        for (int v=0; v < 4; v++) {
          candidate_verts.push_back(tet->verts[v]);
        }

        // then free the tet
        delete tet;
        tets[t] = nullptr;
      }
    }
    // prepare to free unused vertices
    for(size_t v=0; v < candidate_verts.size(); v++) {
      Vertex *vertex = candidate_verts[v];
      if(vertex->tm_v_index < 0){
        delete_list.insert(vertex);
      }
    }

    // free the vertices, once
    std::set<Vertex*>::iterator it;
    for (it = delete_list.begin(); it != delete_list.end(); ++it) {
      delete *it;
    }

    // resize tet list
    tets.resize(tet_count);

    size_t stripped_verts_count = delete_list.size();
    ptrdiff_t stripped_tets_count = tets.size() - tet_count;

    if(verbose) {
      std::cout << "Stripped " << stripped_tets_count << " tets from mesh exterior." << std::endl;
      std::cout << "Stripped " << stripped_verts_count << " verts from mesh exterior." << std::endl;
    }
  }

  //=======================
  //  Adjacency Queries
  //=======================

  //==========================================================================================
  //  return all adjacency lists for the given
  //  tet vertex_i is opposite edge_i on face
  //==========================================================================================
  void TetMesh::getAdjacencyListsForFace(HalfFace *face, Vertex *verts[3], HalfEdge *edges[3])
  {
    edges[0] = face->halfEdges[0];
    edges[1] = face->halfEdges[1];
    edges[2] = face->halfEdges[2];

    verts[0] = edges[1]->vertex;
    verts[1] = edges[2]->vertex;
    verts[2] = edges[0]->vertex;
  }

  //=======================================================================================================
  //  return all adjacency lists for the given tet
  //  - this is possibly a place to enforce an ordering
  //=======================================================================================================
  void TetMesh::getAdjacencyListsForTet(Tet *tet, Vertex *verts[4], HalfEdge *edges[6], HalfFace *faces[4])
  {
    verts[0] = tet->verts[0];
    verts[1] = tet->verts[1];
    verts[2] = tet->verts[2];
    verts[3] = tet->verts[3];

    faces[0] = &halfFaces[(4*tet->tm_index)+0];
    faces[1] = &halfFaces[(4*tet->tm_index)+1];
    faces[2] = &halfFaces[(4*tet->tm_index)+2];
    faces[3] = &halfFaces[(4*tet->tm_index)+3];

    edges[0] = faces[2]->halfEdges[1];  // edge 0-1
    edges[1] = faces[3]->halfEdges[0];  // edge 0-2
    edges[2] = faces[1]->halfEdges[1];  // edge 0-3
    edges[3] = faces[0]->halfEdges[0];  // edge 1-2
    edges[4] = faces[2]->halfEdges[2];  // edge 1-3
    edges[5] = faces[0]->halfEdges[1];  // edge 2-3
  }

  //=================================================================
  // - getRightHandedVertexList()
  //
  // This is the list used to lookup stencil information.
  //=================================================================
  void TetMesh::getRightHandedVertexList(Tet *tet, Vertex *verts[15])
  {
    Vertex *v[4];
    HalfEdge *e[6];
    HalfFace *f[4];

    getAdjacencyListsForTet(tet, v, e, f);

    verts[0]  = v[0];
    verts[1]  = v[1];
    verts[2]  = v[2];
    verts[3]  = v[3];
    verts[4]  = e[0]->cut;
    verts[5]  = e[1]->cut;
    verts[6]  = e[2]->cut;
    verts[7]  = e[3]->cut;
    verts[8]  = e[4]->cut;
    verts[9]  = e[5]->cut;
    verts[10] = f[0]->triple;
    verts[11] = f[1]->triple;
    verts[12] = f[2]->triple;
    verts[13] = f[3]->triple;
    verts[14] = tet->quadruple;
  }

  //==========================================================
  //  return edges incident to vertex v (visually verified)
  //==========================================================
  std::vector<HalfEdge*> TetMesh::edgesAroundVertex(Vertex *v)
  {
    return v->halfEdges;
  }

  bool less(const vec3 &a, const vec3 &b)
  {
    return ((a.x < b.x) ||
        (a.x == b.x && a.y < b.y) ||
        (a.x == b.x && a.y == b.y && a.z < b.z));
  }

  //==========================================================
  // returns half faces incident to vertex v (visually verified)
  //==========================================================
  std::vector<HalfFace*> TetMesh::facesAroundVertex(Vertex *v)
  {
    std::vector<HalfFace*> facelist;

    for(size_t e=0; e < v->halfEdges.size(); e++)
    {
      HalfEdge *he = v->halfEdges[e];
      for(size_t f=0; f < he->halfFaces.size(); f++){

        // if no mate (border) always add
        if(he->halfFaces[f]->mate == nullptr){
          facelist.push_back(he->halfFaces[f]);
        }
        // otherwise only add face if positively oriented (to avoid mate duplicates)
        else{
          vec3 n1 = he->halfFaces[f]->normal();
          vec3 n2 = -1*n1;
          if(less(n2, n1))
            facelist.push_back(he->halfFaces[f]);
        }
      }
    }

    return facelist;
  }

  //=====================================================
  // return tets incident to vertex v (must work)
  //=====================================================
  std::vector<Tet*>  TetMesh::tetsAroundVertex(Vertex *v)
  {
    return v->tets;
  }

  //==========================================================
  //  returns faces incident to edge e - (visually verified)
  //==========================================================
  std::vector<HalfFace*> TetMesh::facesAroundEdge(HalfEdge *e)
  {
    std::vector<HalfFace*> facelist;

    for(size_t f=0; f < e->halfFaces.size(); f++)
      facelist.push_back(e->halfFaces[f]);

    // now look at mate edge, add any incident faces that don't have face-mates
    HalfEdge *me = e->mate;
    for(size_t f=0; f < me->halfFaces.size(); f++){
      if(me->halfFaces[f]->mate == nullptr){
        facelist.push_back(me->halfFaces[f]);
      }
    }

    return facelist;
  }

  //=====================================================
  // returns tets incident to edge e - (visually verified)
  //=====================================================
  std::vector<Tet*>  TetMesh::tetsAroundEdge(HalfEdge *e)
  {
    std::vector<Tet*> tetlist;

    for(size_t f=0; f < e->halfFaces.size(); f++){
      size_t index = (e->halfFaces[f] - &this->halfFaces[0]) / 4;
      tetlist.push_back(tets[index]);
    }

    // now look at mate edge, add any incident faces that don't have face-mates
    HalfEdge *me = e->mate;
    for(size_t f=0; f < me->halfFaces.size(); f++){
      if(me->halfFaces[f]->mate == nullptr){
        size_t index = (me->halfFaces[f] - &this->halfFaces[0]) / 4;
        tetlist.push_back(tets[index]);
      }
    }

    return tetlist;
  }

  //---------------------------------------------------------
  // returns tets incident to face - (visually verified)
  //---------------------------------------------------------
  std::vector<Tet*>  TetMesh::tetsAroundFace(HalfFace *f)
  {
    std::vector<Tet*> tetlist;

    size_t index1 = (f - &this->halfFaces[0]) / 4;
    tetlist.push_back(tets[index1]);

    if(f->mate){
      size_t index2 = (f->mate - &this->halfFaces[0]) / 4;
      tetlist.push_back(tets[index2]);
    }

    return tetlist;
  }

  //========================================================
  // returns vertices incident to face
  //========================================================
  std::vector<Vertex*> TetMesh::vertsAroundFace(HalfFace *f)
  {
    std::vector<Vertex*> vertlist;

    vertlist.push_back(f->halfEdges[0]->vertex);
    vertlist.push_back(f->halfEdges[1]->vertex);
    vertlist.push_back(f->halfEdges[2]->vertex);

    return vertlist;
  }

  //==================================================
  // returns vertices incident to tet
  //==================================================
  std::vector<Vertex*> TetMesh::vertsAroundTet(Tet *t)
  {
    std::vector<Vertex*> vertlist;

    vertlist.push_back(t->verts[0]);
    vertlist.push_back(t->verts[1]);
    vertlist.push_back(t->verts[2]);
    vertlist.push_back(t->verts[3]);

    return vertlist;
  }

  //====================================================
  // returns faces incident to tet
  //====================================================
  std::vector<HalfFace*> TetMesh::facesAroundTet(Tet *t)
  {
    std::vector<HalfFace*> facelist;

    facelist.push_back(&halfFaces[4*t->tm_index + 0]);
    facelist.push_back(&halfFaces[4*t->tm_index + 1]);
    facelist.push_back(&halfFaces[4*t->tm_index + 2]);
    facelist.push_back(&halfFaces[4*t->tm_index + 3]);

    return facelist;
  }

  //====================================================
  // returns edges incident to tet
  //====================================================
  std::vector<HalfEdge*> TetMesh::edgesAroundTet(Tet *t)
  {
    HalfFace *faces[4];
    faces[0] = &halfFaces[(4*t->tm_index)+0];
    faces[1] = &halfFaces[(4*t->tm_index)+1];
    faces[2] = &halfFaces[(4*t->tm_index)+2];
    faces[3] = &halfFaces[(4*t->tm_index)+3];

    std::vector<HalfEdge*> edgelist;

    edgelist.push_back(faces[2]->halfEdges[1]);  // edge 0-1
    edgelist.push_back(faces[3]->halfEdges[0]);  // edge 0-2
    edgelist.push_back(faces[1]->halfEdges[1]);  // edge 0-3
    edgelist.push_back(faces[0]->halfEdges[0]);  // edge 1-2
    edgelist.push_back(faces[2]->halfEdges[2]);  // edge 1-3
    edgelist.push_back(faces[0]->halfEdges[1]);  // edge 2-3

    return edgelist;
  }

  std::vector<HalfFace*> TetMesh::facesIncidentToBothTetAndEdge(Tet *tet, HalfEdge *edge)
  {
    std::vector<HalfFace*> facelist;
    std::vector<HalfFace*> tet_faces = facesAroundTet(tet);

    for(size_t f=0; f < 4; f++)
    {
      HalfFace *face = tet_faces[f];

      for(int e=0; e < 3; e++)
      {
        if(face->halfEdges[e] == edge || face->halfEdges[e]->mate == edge)
        {
          facelist.push_back(face);
          break;
        }
      }
    }

    return facelist;
  }

  Tet* TetMesh::oppositeTetAcrossFace(Tet *tet, HalfFace *face)
  {
    std::vector<Tet*> tets = tetsAroundFace(face);

    if(tets[0] == tet){
      if(tets.size() > 1)
        return tets[1];
      else
        return nullptr;
    }
    else
      return tets[0];
  }

  //------------------------------------------------------
  // This method checks that the given tetrahedral mesh
  // is a valid mesh. It does so by ensuring that no more
  // than two tets share any given face.
  //------------------------------------------------------
  bool TetMesh::isValid()
  {
    std::map<std::string, int> face_count;

    for(size_t t=0; t < tets.size(); t++)
    {
      for(int f=0; f < FACES_PER_TET; f++)
      {
        vector<int> index_list(3);
        index_list[0] = this->tets[t]->verts[(f+1)%4]->tm_v_index;
        index_list[1] = this->tets[t]->verts[(f+2)%4]->tm_v_index;
        index_list[2] = this->tets[t]->verts[(f+3)%4]->tm_v_index;

        sort(index_list.begin(), index_list.end());

        std::stringstream ss;
        ss << index_list[0] << " " << index_list[1] << " " << index_list[2];
        std::string key = ss.str();

        // if not in the map, add it with count one
        if(face_count.count(key) > 0){
          face_count[key] = face_count[key] + 1;
        }
        else
          face_count[key] = 1;
      }
    }

    int one_count = 0;
    int two_count = 0;
    int more_count = 0;

    for(size_t t=0; t < tets.size(); t++)
    {
      for(int f=0; f < FACES_PER_TET; f++)
      {
        vector<int> index_list(3);
        index_list[0] = this->tets[t]->verts[(f+1)%4]->tm_v_index;
        index_list[1] = this->tets[t]->verts[(f+2)%4]->tm_v_index;
        index_list[2] = this->tets[t]->verts[(f+3)%4]->tm_v_index;

        sort(index_list.begin(), index_list.end());

        std::stringstream ss;
        ss << index_list[0] << " " << index_list[1] << " " << index_list[2];
        std::string key = ss.str();

        int count = face_count[key];
        if(count == 1)
          one_count++;
        else if(count == 2)
          two_count++;
        else
          more_count++;
      }
    }

    if(more_count == 0){
      return true;
    }
    else
      return false;
  }

  void TetMesh::constructBottomUpIncidences(bool verbose)
  {
    if(verbose)
      std::cout << "constructing bottom up incidences" << std::endl;


    //-----------------------------------
    //  First Obtain Tet-Tet Adjacency
    //-----------------------------------
    for(size_t i=0; i < this->tets.size(); i++)
    {
      // look for a tet sharing three verts opposite vert[j]
      for (int j=0; j < 4; j++)
      {
        if (this->tets[i]->tets[j] == nullptr)
        {
          // grab three vertices to compare against
          Vertex *v0 = this->tets[i]->verts[(j+1)%4];
          Vertex *v1 = this->tets[i]->verts[(j+2)%4];
          Vertex *v2 = this->tets[i]->verts[(j+3)%4];
          //bool found_adjacent = false;

          // search over adjacent tets touching these verts
          for (size_t k=0; k < v0->tets.size(); k++)
          {
            Tet* tet = v0->tets[k];
            if(tet != this->tets[i])
            {
              // check if it is adjacent, i.e. has v2 and v3
              int shared_count = 0;
              for (int l=0; l < 4; l++){
                if (tet->verts[l] == v1 || tet->verts[l] == v2)
                  shared_count++;
              }

              // if match found
              if (shared_count == 2)
              {
                //--------------------
                //  set for this tet
                //--------------------
                this->tets[i]->tets[j] = tet;

                //----------------------
                // set for neighbor tet
                //----------------------
                // first figure out reversed face
                int shared_face = 0;
                for (int m=0; m<4; m++){
                  if(tet->verts[m] != v0 && tet->verts[m] != v1 && tet->verts[m] != v2){
                    shared_face = m;
                  }
                }
                // then set it
                tet->tets[shared_face] = this->tets[i];

                // done searching adjacent tets
                break;
              }
            }
          }
        }
      }
    }
    // allocate sufficient space
    halfFaces = std::vector<HalfFace>(4*tets.size());
    halfEdges.clear();

    std::queue<Tet*> tq;

    tq.push(tets[0]);

    while(!tq.empty())
    {
      Tet *tet = tq.front();

      // pop and continue if this tet was already done
      // can tell by checking if halfEdges set yet
      if(halfFaces[4*tet->tm_index].halfEdges[0])
      {
        tq.pop();
        continue;
      }

      // for each face in untouched tet
      for(int f=0; f < 4; f++)
      {
        // get pointer to half face
        HalfFace *half_face = &halfFaces[4*tet->tm_index+f];

        // grab vertices that are in face
        Vertex *v[3];
        v[0] = tet->verts[VERT_FACE_LIST[f][0]];
        v[1] = tet->verts[VERT_FACE_LIST[f][1]];
        v[2] = tet->verts[VERT_FACE_LIST[f][2]];

        // for each half-edge, ordered pair
        for(int e=0; e < 3; e++)
        {
          Vertex *v1 = v[e];
          Vertex *v2 = v[(e+1)%3];

          HalfEdge *half_edge = halfEdgeForVerts(v1, v2);
          HalfEdge *pair_edge = halfEdgeForVerts(v2, v1);
          half_face->halfEdges[e] = half_edge;

          // check if this edge has been assigned yet
          if(half_edge->vertex == nullptr){
            half_edge->vertex = v2;
            v1->halfEdges.push_back(half_edge);
          }
          if(pair_edge->vertex == nullptr){
            pair_edge->vertex = v1;
            v2->halfEdges.push_back(pair_edge);
          }

          // we only touch this half face once, can add safely without duplicates
          half_edge->halfFaces.push_back(half_face);

          // set mates, always correct
          half_edge->mate = pair_edge;
          pair_edge->mate = half_edge;
        }

        // set up face mate pointer if it exists
        if(tet->tets[f]){
          for(int ff=0; ff < 4; ff++)
          {
            Vertex *opv = tet->tets[f]->verts[ff];
            if(opv != v[0] && opv != v[1] && opv != v[2]){
              half_face->mate = &halfFaces[4*tet->tets[f]->tm_index+ff];
              break;
            }
          }
        }
      }

      // done with each face, pop the tet
      tq.pop();

      // push neighbors
      for(int j=0; j < 4; j++){
        if(tet->tets[j])
          tq.push(tet->tets[j]);
      }

    }

    //------- Begin DEBUGGING CODE FOR VIS ---------/
    // go through all faces and see if any were disconnected from mesh
    // if they were, temporarily give them edges so we can visualize them.

    for(size_t f=0; f < 4*tets.size(); f++)
    {
      // get pointer to half face
      HalfFace *half_face = &halfFaces[f];

      if(half_face->halfEdges[0] == nullptr)
      {
        std::cerr << "Found disconnected face! f=" << f << std::endl;

      }
    }
  }

  std::string GetLineSkipComments(std::ifstream &stream)
  {
    std::string line;
    std::getline(stream, line);

    //------ skip any commented or blank lines before header -----//
    bool comment_or_blank = false;
    do{
      comment_or_blank = false;
      std::string whitespaces ("\t\f\v\n\r");
      std::size_t found = line.find_first_not_of(whitespaces);

      // trim leading whitespace
      if (found != std::string::npos)
        line = line.substr(found);

      if(line.empty() || line[0] == '#'){
        comment_or_blank = true;
        line.clear();
        std::getline(stream, line);
        if(!stream){
          std::cout << "failure" << std::endl;
          return std::string();
        }
      }
    }while(comment_or_blank);
    //----- Should now be at first non comment or blank line ----//

    return line;
  }

  TetMesh* TetMesh::createFromNodeElePair(const std::string &nodeFileName, const std::string &elemFileName, bool verbose)
  {
    std::ifstream nodestream(nodeFileName.c_str());
    std::ifstream elemstream(elemFileName.c_str());

    if(!nodestream.is_open() || !elemstream.is_open())
    {
      std::cerr << "node or ele file open failed!" << std::endl;
      exit(0);
    }

    int nv = 0;
    int tmp,tmp2,tmp3;

    std::string line = GetLineSkipComments(nodestream);

    std::istringstream parser(line);
    parser >> nv >> tmp >> tmp2 >> tmp3;

    if (verbose) {
      std::cout << "Imput mesh contains " << nv << " nodes" << std::endl;
    }

    double xmin = 1.0e16, xmax = -1.0e16,
           ymin = 1.0e16, ymax = -1.0e16,
           zmin = 1.0e16, zmax = -1.0e16;
    vector< Vertex*> verts(nv);

    for(int i = 0; i < nv; i++)
    {
      float x, y, z;

      line = GetLineSkipComments(nodestream);
      parser.str(line);
      parser.clear();
      parser >> tmp >> x >> y >> z;

      if(x < xmin) xmin = x;
      if(x > xmax) xmax = x;

      if(y < ymin) ymin = y;
      if(y > ymax) ymax = y;

      if(z < zmin) zmin = z;
      if(z > zmax) zmax = z;

      verts[i] = new Vertex();
      verts[i]->pos() = vec3(x,y,z);
      verts[i]->tm_v_index = i;
    }

    TetMesh *mesh = new TetMesh(verts,vector< Tet*>());

    mesh->bounds = BoundingBox(xmin, ymin, zmin, xmax - xmin, ymax - ymin, zmax - zmin);


    if (verbose) {
      printf("xbound = (%f, %f, %f), ybound = (%f, %f %f), zbound = (%f, %f, %f)\n",
          xmin, xmax, mesh->bounds.size.x, ymin, ymax,
          mesh->bounds.size.y, zmin, zmax, mesh->bounds.size.z);
    }


    int ne = 0;
    int attrCount = 0;
    int tetidx[4];
    std::set<int> mats;
    mats.insert(0);
    int material;
    int parent;

    line = GetLineSkipComments(elemstream);
    parser.str(line);
    parser.clear();
    parser >> ne >> tmp >> attrCount;

    if (verbose)
      std::cout << "Imput mesh contains " << ne << " elements." << std::endl;
    //in case our indices start at zero
    bool zero_based = false;
    for(int i=0; i < ne; i++)
    {
      line = GetLineSkipComments(elemstream);
      parser.str(line);
      parser.clear();
      parser >> tmp >> tetidx[0] >> tetidx[1] >> tetidx[2] >> tetidx[3];
      //bring the indexing down 1 to be zero based?
      if (i == 0) {
        zero_based = tmp == 0;
      }
      material = 0;
      if (attrCount > 0) {
        parser >> material;
        mats.insert(material);
      }
      if (attrCount > 1) parser >> parent;
      Tet * tet = mesh->createTet(
          verts[tetidx[0]-(zero_based?0:1)],
          verts[tetidx[1]-(zero_based?0:1)],
          verts[tetidx[2]-(zero_based?0:1)],
          verts[tetidx[3]-(zero_based?0:1)],
          material);
      if (attrCount > 1) tet->parent = parent;
    }

    mesh->material_count = static_cast<int>(mats.size());
    if (verbose)
      std::cout << "Number of materials: " << mats.size() << std::endl;

    // close input files
    nodestream.close();
    elemstream.close();

    return mesh;
  }

  //-----------------------------------
  // -removeExternalTets()
  //
  // This method removes all tets from
  // the mesh that have 4 vertices
  // each of which is marked 'external'
  //------------------------------------
  void TetMesh::removeExternalTets()
  {
    // loop over all tets in the mesh
    std::vector<Tet*>::iterator iter = tets.begin();
    while(iter != tets.end())
    {
      Tet *tet = *iter;

      // erase only if all 4 vertices are exterior
      if(tet->verts[0]->isExterior && tet->verts[1]->isExterior &&
          tet->verts[2]->isExterior && tet->verts[3]->isExterior)
      {
        iter = removeTet(iter);
      }
      else
        iter++;
    }

    constructFaces();
    constructBottomUpIncidences();
  }

  //------------------------------
  // -removeLockedTets()
  //
  // This method removes all tets from
  // the mesh that have 4 vertices
  // which cannot be moved due to
  // snaps and warps
  //------------------------------
  void TetMesh::removeLockedTets()
  {
    // loop over all tets in the mesh
    std::vector<Tet*>::iterator iter = tets.begin();
    while(iter != tets.end())
    {
      Tet *tet = *iter;

      // check tets incident to the 4 vertices
      bool safe = true;
      for(int v=0; v < 4; v++)
      {
        std::vector<HalfEdge*> incEdges = edgesAroundVertex(tet->verts[v]);

        for(size_t e=0; e < incEdges.size(); e++)
        {
          HalfEdge *edge = incEdges[e];

          if(edge->cut && edge->cut->order() == Order::CUT)
          {
            safe = false;
            break;
          }
        }

        if(!safe)
          break;
      }

      // if no cuts incident, safe to delete
      if(safe)
        iter = removeTet(iter);
      else
        iter++;
    }
    // fix tm indices
    for(size_t t=0; t < tets.size(); t++)
    {
      tets[t]->tm_index = static_cast<int>(t);
    }

    constructFaces();
    constructBottomUpIncidences();
  }

  //------------------------------
  // -removeMaterial()
  //
  // This method removes all tets from
  // the mesh that have the material
  // label m.
  //------------------------------
  void TetMesh::removeMaterial(int m)
  {
    // loop over all tets in the mesh
    std::vector<Tet*>::iterator iter = tets.begin();
    while(iter != tets.end())
    {
      Tet *tet = *iter;

      // erase only if all 4 vertices are exterior
      if(tet->mat_label == m)
        iter = removeTet(iter);
      else
        iter++;
    }

    constructFaces();
    constructBottomUpIncidences();
  }

  //------------------------------
  // -removeOutsideBox()
  //
  // This method removes all tets from
  // the mesh that are outside the
  // provided bounding box
  // label m.
  //------------------------------
  void TetMesh::removeOutsideBox(BoundingBox &box)
  {
    // loop over all tets in the mesh
    std::vector<Tet*>::iterator iter = tets.begin();
    while(iter != tets.end())
    {
      Tet *tet = *iter;

      bool outside = true;
      for(int v=0; v < 4; v++)
      {
        if(box.contains(tet->verts[v]->pos()))
        {
          outside = false;
        }
      }

      if(outside)
        iter = removeTet(iter);
      else
        iter++;
    }

    constructFaces();
    constructBottomUpIncidences(true);
  }

  //----------------------------------------------------------------
  // - removeTet()
  //
  // This method takes care of all the dirty work required to
  // safely remove a tet from the mesh.
  //----------------------------------------------------------------
  std::vector<Tet*>::iterator TetMesh::removeTet(std::vector<Tet*>::iterator iter)
  {
    Tet *tet = *iter;

    //---------------------------------------
    // Get Rid of Old Adjacency Information
    //---------------------------------------
    for(int v=0; v < 4; v++)
    {
      for(size_t j=0; j < tet->verts[v]->tets.size(); j++)
      {
        // remove this tet from the list of all its vertices
        if(tet->verts[v]->tets[j] == tet){
          tet->verts[v]->tets.erase(tet->verts[v]->tets.begin() + j);
          break;
        }
      }
    }


    tets.erase(iter);

    return iter;
  }

  //-----------------------------
  // - removeTet()
  //
  // This method takes care of all
  // the dirty work required to
  // safely remove a tet from the
  // mesh.
  //-----------------------------
  void TetMesh::removeTet(int t)
  {
    int i = 0;
    std::vector<Tet*>::iterator iter = tets.begin();
    while(iter != tets.end() && i < t) { i++; ++iter; }
    if (iter != tets.end())
      removeTet(iter);

  }

}
