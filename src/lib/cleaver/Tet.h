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

#ifndef TET_H
#define TET_H

#include "Vertex.h"
#include "Face.h"
#include "Tet.h"
#include "vec3.h"

namespace cleaver
{

class Tet
{
public:
    Tet(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, int material);
    Tet();
    ~Tet();

    double volume() const;

    Vertex *quadruple;
    Vertex *verts[4];
    Tet* tets[4];         // tet[i] is incident to face[i]
    Face* faces[4];       // face[i] is opposite vertex[i]
    int tm_index;
    int parent;
    char mat_label;
    unsigned char key;
    bool evaluated:1;
    bool output:1;
    bool flagged:1;  // for debugging

    float minAngle();
    float maxAngle();

    bool contains(Vertex *v) const;
};

}

#endif // TET_H
