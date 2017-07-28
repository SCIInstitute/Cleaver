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

#include "Debug.h"
#include <vector>
#include <algorithm>

using namespace std;

std::string catIds(std::vector<int> ids) {
  std::string result;
  for (auto id : ids) {
    if (!result.empty()) {
      result += ",";
    }
    result += std::to_string(id);
  }
  return result;
}

namespace cleaver
{
	//
  Json::Value createVertexOperation(Vertex *vertex, int id) {
    Json::Value root(Json::objectValue);
    root["name"] = "CREATE_VERTEX";
    root["id"] = id,
    root["material"] = vertex->label;
    root["position"] = Json::Value(Json::objectValue);
    root["position"]["x"] = vertex->pos().x;
    root["position"]["y"] = vertex->pos().y;
    root["position"]["z"] = vertex->pos().z;
    return root;
  }

  //
  Json::Value createVertexOperation(Vertex *vertex) {
    return createVertexOperation(vertex, vertex->tm_v_index);
  }

  std::string idForEdge(HalfEdge *edge) {
    Vertex *v1 = edge->vertex;
    Vertex *v2 = edge->mate->vertex;    
    if (v1->tm_v_index > v2->tm_v_index) {
      std::swap(v1, v2);      
    }
    return catIds({ v1->tm_v_index, v2->tm_v_index});
  }

	//
	Json::Value createEdgeOperation(HalfEdge *edge) {
    Vertex *v1 = edge->vertex;
    Vertex *v2 = edge->mate->vertex;
    double alpha1 = edge->alpha;
    double alpha2 = edge->mate->alpha;    
    if (v1->tm_v_index > v2->tm_v_index) {
      std::swap(v1, v2);
      std::swap(alpha1, alpha2);      
    }

    Json::Value root(Json::objectValue);
    root["name"] = "CREATE_EDGE";
    root["id"] = catIds({ v1->tm_v_index, v2->tm_v_index}).c_str();
    root["v1"] = v1->tm_v_index;
    root["v2"] = v2->tm_v_index;
    root["alpha1"] = alpha1;
    root["alpha2"] = alpha2;

    if (edge->cut && edge->cut->order() == Order::CUT) {
      // TODO(jonbronson): set a UID and add operation to create separately
      root["cut"] = createVertexOperation(edge->cut);  
      root["cut"]["violating"] = edge->cut->violating;
    }

    return root;
	}


	//
	Json::Value createFaceOperation(HalfFace *face) {
    Vertex *v1 = face->halfEdges[0]->vertex;
    Vertex *v2 = face->halfEdges[1]->vertex;
    Vertex *v3 = face->halfEdges[2]->vertex;

    std::vector<Vertex*> vertexList = {v1, v2, v3};
    std::sort(vertexList.begin(), vertexList.end(), [](Vertex* a, Vertex* b) -> bool 
      { 
        return (a->tm_v_index > b->tm_v_index);
      });
    v1 = vertexList[0];
    v2 = vertexList[1];
    v3 = vertexList[2];

    Json::Value root(Json::objectValue);
    root["name"] = "CREATE_FACE";
    root["id"] = catIds({ v1->tm_v_index, v2->tm_v_index, v3->tm_v_index}).c_str();
    root["v1"] = v1->tm_v_index;
    root["v2"] = v2->tm_v_index;
    root["v3"] = v3->tm_v_index;

    if (face->triple && face->triple->order() == Order::TRIP) {
      root["triple"] = createVertexOperation(face->triple);   // TODO(jonbronson): set a UID and add operation to create separately
    }

    return root;
	}

	//
	std::vector<Json::Value> createTetOperations(Tet *tet, TetMesh *mesh, bool debug) {
    Vertex *verts[4];
    HalfEdge *edges[6];
    HalfFace *faces[4];
    mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

    std::vector<Json::Value> operations;
    for (int v = 0; v < VERTS_PER_TET; v++) {
      operations.push_back(createVertexOperation(verts[v]));
    }

    for (int e = 0; e < EDGES_PER_TET; e++) {
      operations.push_back(createEdgeOperation(edges[e]));
    }

    for (int f = 0; f < FACES_PER_TET; f++) {
      operations.push_back(createFaceOperation(faces[f]));
    }

    Json::Value root(Json::objectValue);
    root["name"] = "CREATE_TET";
    root["id"] = tet->tm_index;
    root["verts"] = Json::Value(Json::arrayValue);
    for (int v = 0; v < VERTS_PER_TET; v++) {
      root["verts"].append(verts[v]->tm_v_index);
    }

    if (tet->quadruple && tet->quadruple->order() == Order::QUAD) {
      // TODO(jonbronson): set a UID and add operation to create separately
      root["quadruple"] = createVertexOperation(tet->quadruple);   
    }

    operations.push_back(root);
    return operations;
	}

	// To watch the cleaver algorithm on a tet, we need the set of
	// primitives that can affect the output of this particular tet.
	// Redundancy in the creation can be handled when replaying the
	// operations.
	std::vector<Json::Value> createTetSet(Tet *tet, TetMesh *mesh) {
		Vertex *verts[4];
		HalfEdge *edges[6];
		HalfFace *faces[4];
		mesh->getAdjacencyListsForTet(tet, verts, edges, faces);
		
		// create this tet - marked as debug tet
		auto operations = createTetOperations(tet, mesh, true /* debug */);

		// also create tets incident to each of the 4 vertices. 
		for (unsigned int v = 0; v < VERTS_PER_TET; v++) {
			auto tets = mesh->tetsAroundVertex(verts[v]);
			for (auto incidentTet : tets) {
				if (incidentTet != tet) {
          auto tetOperations = createTetOperations(incidentTet, mesh);
          operations.insert(operations.end(), tetOperations.begin(), tetOperations.end());					
				}
			}
		}

    return operations;
	}

  Json::Value createVertexSnapOperation(
    Vertex *vertex, const vec3 &warp_point, std::vector<HalfEdge*> violating_cuts,
    std::vector<HalfEdge*> projected_cuts) {

    Json::Value root(Json::objectValue);
    root["name"] = "SNAP_VERTEX";
    root["vertex"] = vertex->tm_v_index;
    root["warp_point"] = Json::Value(Json::objectValue);
    root["warp_point"]["x"] = warp_point.x;
    root["warp_point"]["y"] = warp_point.y;
    root["warp_point"]["z"] = warp_point.z;
    root["violating_cuts"] = Json::Value(Json::arrayValue);
    for (size_t v = 0; v < violating_cuts.size(); v++) {
      std::string id = idForEdge(violating_cuts[v]);
      root["violating_cuts"].append(id.c_str());
    }
    root["projected_cuts"] = Json::Value(Json::arrayValue);    
    for (size_t v = 0; v < projected_cuts.size(); v++) {
      std::string id = idForEdge(projected_cuts[v]);
      Json::Value cut = Json::Value(Json::objectValue);
      cut["id"] = id.c_str();
      Json::Value position = Json::Value(Json::objectValue);      
      position["x"] = projected_cuts[v]->cut->pos_next().x;
      position["y"] = projected_cuts[v]->cut->pos_next().y;
      position["z"] = projected_cuts[v]->cut->pos_next().z;
      cut["position"] = position;
      root["projected_cuts"].append(cut);
    }
    
    return root;
  }

} // namespace cleaver
