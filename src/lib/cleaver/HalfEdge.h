//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- HalfEdge Class
//
// Author: Jonathan Bronson (bronson@sci.utah.edu)
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

#ifndef HALFEDGE_H_
#define HALFEDGE_H_

#include "Vertex.h"

namespace cleaver
{

class HalfEdge : public Geometry
{
public:
    HalfEdge() : vertex(nullptr), mate(nullptr), cut(nullptr), alpha(0.2f), evaluated(false), parity(false){}
    HalfEdge(bool long_edge) : vertex(nullptr), mate(nullptr), cut(nullptr),
        alpha(0.2f), evaluated(false), parity(false), m_long_edge(long_edge){}
    Vertex *vertex;
    HalfEdge *mate;
    std::vector<HalfFace*> halfFaces;
    Vertex *cut;
    float alpha;
    float alpha_length;
    bool evaluated:1;
    bool parity:1;
    bool m_long_edge:1;

    bool sameAs(HalfEdge *e){
        return(e == this || e->mate == this);
    }

    // helper functions
    bool incidentToVertex(Vertex *v) {
        return(vertex == v || mate->vertex == v);
    }

    float alphaForVertex(Vertex *v) {
        if(vertex == v)
            return mate->alpha;
        else if(mate->vertex == v)
            return alpha;
        else
            return 0.0f;
    }

    float edgeLength() {
        return static_cast<float>(length(vertex->pos() - mate->vertex->pos()));
    }

    float alphaLengthForVertex(Vertex *v) {
        float alpha = alphaForVertex(v);
        return alpha*edgeLength();
    }

    void setAlphaLengthForVertex(Vertex *v, float alpha_length) {

        float alpha = alpha_length / edgeLength();

        if(vertex == v)
            mate->alpha = alpha;
        else if(mate->vertex == v)
            this->alpha = alpha;
    }
};

} // namespace cleaver

#endif // HALFEDGE_H_
