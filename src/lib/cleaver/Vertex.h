//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Vertex Class
//
// Author: Jonathan Bronson (bronson@sci.utah.ed)
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

#ifndef VERTEX_H
#define VERTEX_H

#include <vector>
#include <cstring>
#include "vec3.h"
#include "Geometry.h"

namespace cleaver
{

enum class Order : std::int8_t {
    VERT = 0,
    CUT  = 1,
    TRIP = 2,
    QUAD = 3
};

class OTCell;
class HalfEdge;
class HalfFace;
class Face;
class Tet;

class Vertex : public Geometry{

public:
    Vertex(int materials) : parent(nullptr), conformedVertex(nullptr), conformedEdge(nullptr), conformedFace(nullptr),
        isExterior(false), violating(false), warped(false), tm_v_index(-1), lbls(new bool[materials+1]), dual(false),
        m_order(Order::VERT), m_pos(vec3::zero), m_pos_next(vec3::zero)
    {
        // increases lbls by 1 to account for background feb 20
        memset(lbls, 0, (materials+1)*sizeof(bool));
    }
    Vertex() : parent(nullptr), conformedVertex(nullptr), conformedEdge(nullptr), conformedFace(nullptr),
        isExterior(false), violating(false), warped(false), tm_v_index(-1), lbls(0), dual(false),
        m_order(Order::VERT), m_pos(vec3::zero),m_pos_next(vec3::zero){ }
    ~Vertex();

    inline vec3& pos(){
        Vertex* ptr = this;
        while(ptr->parent)
            ptr = ptr->parent;
        return ptr->m_pos;
    }
    inline vec3& pos_next(){
        Vertex* ptr = this;
        while(ptr->parent)
            ptr = ptr->parent;
        return ptr->m_pos_next;
    }
    inline Order& order(){
        Vertex *ptr = this;
        while(ptr->parent)
            ptr = ptr->parent;
        return ptr->m_order;
    }

    const Order original_order(){
        return m_order;
    }

    inline Vertex* root() {
        Vertex *ptr = this;
        while(ptr->parent)
            ptr = ptr->parent;
        return ptr;
    }

    inline bool isEqualTo(Vertex* vert)
    {
        return (this->root() == vert->root());
    }


    Vertex   *parent;
    Vertex   *conformedVertex;
    HalfEdge *conformedEdge;
    HalfFace *conformedFace;
    Geometry *closestGeometry;
    std::vector<HalfEdge*> halfEdges;
    std::vector<Tet*> tets;
    std::vector<Face*> faces;
    bool isExterior:1;             // is not part of domain
    bool violating:1;            // is this cut violating
    bool warped:1;               // has this edge been warped
    int tm_v_index;
    unsigned char label;       // single label (for generating texture image)
    bool  *lbls;               // material labels
    bool dual;
    bool phantom;

private:
    Order m_order;      // vertex order
    vec3 m_pos;         // current position
    vec3 m_pos_next;    // next position

};

//typedef Vertex Vertex;

}

#endif // VERTEX_H
