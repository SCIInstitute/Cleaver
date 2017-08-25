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

  std::string idForFace(HalfFace *face) {
    Vertex *v1 = face->halfEdges[0]->vertex;
    Vertex *v2 = face->halfEdges[1]->vertex;
    Vertex *v3 = face->halfEdges[2]->vertex;
    std::vector<int> ids = { v1->tm_v_index, v2->tm_v_index, v3->tm_v_index };
    std::sort(ids.begin(), ids.end());
    return catIds(ids);
  }

  std::string idForTet(Tet *tet) {
    return catIds({tet->tm_index});
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
    Vertex *vertex, const vec3 &warp_point, 
    std::vector<HalfEdge*> violating_cuts,  std::vector<HalfEdge*> projected_cuts,
    std::vector<HalfFace*> violating_trips, std::vector<HalfFace*> projected_trips,
    std::vector<Tet*>      violating_quads, std::vector<Tet*>      projected_quads){

    Json::Value root(Json::objectValue);
    root["name"] = "SNAP_VERTEX";
    root["vertex"] = vertex->tm_v_index;
    root["warp_point"] = Json::Value(Json::objectValue);
    root["warp_point"]["x"] = warp_point.x;
    root["warp_point"]["y"] = warp_point.y;
    root["warp_point"]["z"] = warp_point.z;

    // add involved cuts
    root["violating_cuts"] = Json::Value(Json::arrayValue);
    for (HalfEdge *edge : violating_cuts) {
      root["violating_cuts"].append(idForEdge(edge).c_str());
    }
    root["projected_cuts"] = Json::Value(Json::arrayValue);    
    for (HalfEdge *edge : projected_cuts) {
      Json::Value cut = Json::Value(Json::objectValue);
      cut["id"] = idForEdge(edge).c_str();
      cut["position"] = Json::Value(Json::objectValue);      
      cut["position"]["x"] = edge->cut->pos_next().x;
      cut["position"]["y"] = edge->cut->pos_next().y;
      cut["position"]["z"] = edge->cut->pos_next().z;      
      root["projected_cuts"].append(cut);
    }

    // add involved triples
    root["violating_triples"] = Json::Value(Json::arrayValue);
    for (HalfFace *face : violating_trips) {
      root["violating_triples"].append(idForFace(face).c_str());      
    }
    root["projected_triples"] = Json::Value(Json::arrayValue);
    for (HalfFace *face : projected_trips) {
      Json::Value triple = Json::Value(Json::objectValue);
      triple["id"] = idForFace(face).c_str();
      triple["position"] = Json::Value(Json::objectValue);
      triple["position"]["x"] = face->triple->pos_next().x;
      triple["position"]["y"] = face->triple->pos_next().y;
      triple["position"]["z"] = face->triple->pos_next().z;      
      root["projected_triples"].append(triple);
    }

    // add involved quadruples
    root["violating_quadruples"] = Json::Value(Json::arrayValue);
    for (Tet *tet : violating_quads) {
      root["violating_quadruples"].append(idForTet(tet).c_str());
    }

    root["projected_quadruples"] = Json::Value(Json::arrayValue);
    for (Tet *tet : projected_quads) {
      Json::Value quadruple = Json::Value(Json::objectValue);
      quadruple["id"] = idForTet(tet).c_str();
      quadruple["position"] = Json::Value(Json::objectValue);
      quadruple["position"]["x"] = tet->quadruple->pos_next().x;
      quadruple["position"]["y"] = tet->quadruple->pos_next().y;
      quadruple["position"]["z"] = tet->quadruple->pos_next().z;
      root["projected_quadruples"].append(quadruple);      
    }
    
    return root;
  }

} // namespace cleaver
