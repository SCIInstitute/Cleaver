//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- HalfFace Class
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

#ifndef HALFFACE_H_
#define HALFFACE_H_

#include "Vertex.h"
#include "HalfEdge.h"
#include "vec3.h"

namespace cleaver
{

class HalfFace : public Geometry
{
public:
    HalfFace() : mate(nullptr), triple(nullptr), evaluated(false) { memset(halfEdges, 0, 3*sizeof(HalfEdge*)); }

    HalfEdge *halfEdges[3];
    HalfFace *mate;
    Vertex *triple;
    bool evaluated:1;

    bool sameAs(HalfFace *f){
      return(f == this || f->mate == this);
    }

    // helper functions
    bool incidentToVertex(Vertex *v){
      return(halfEdges[0]->vertex == v || halfEdges[1]->vertex == v || halfEdges[2]->vertex == v);
    }

    vec3 normal() const
    {
      vec3 v1 = this->halfEdges[0]->vertex->pos();
      vec3 v2 = this->halfEdges[1]->vertex->pos();
      vec3 v3 = this->halfEdges[2]->vertex->pos();

      return normalize(cross(v2 - v1, v3 - v1));
    }
};

}

#endif // HALFFACE_H_
