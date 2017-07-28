//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Debug Tools
//
// Author: Jonathan Bronson (bronson@sci.utah.ed)
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

#ifndef DEBUG_H
#define DEBUG_H

#include "TetMesh.h"
#include "vec3.h"
#include "Vertex.h"
#include "HalfEdge.h"
#include <jsoncpp/json.h>

#include <string>
#include <exception>


namespace cleaver
{
  std::string idForEdge(HalfEdge *edge);
	Json::Value createVertexOperation(Vertex *vertex);
	Json::Value createEdgeOperation(HalfEdge *edge);
	Json::Value createFaceOperation(HalfFace *face);
	std::vector<Json::Value> createTetOperations(Tet *tet, TetMesh *mesh, bool debug=false);
	std::vector<Json::Value> createTetSet(Tet *tet, TetMesh *mesh);
  Json::Value createVertexSnapOperation(Vertex *vertex,  const vec3 &warp_point, 
      std::vector<HalfEdge*> violating_cuts,  std::vector<HalfEdge*> projected_cuts);
}

#endif // DEBUG_H