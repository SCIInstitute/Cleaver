//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- CleaverMesher Unit Tests
//
// Author: Jonathan Bronson (bronson@sci.utah.ed)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2015, Jonathan Bronson
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

#include "gtest/gtest.h"
#include "TetMesh.h"
#include "CleaverMesherImpl.h"

class MesherTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        this->mesh = createTetMesh();
        this->mesher = new cleaver::CleaverMesherImp();
        mesher->setBackgroundMesh(mesh);
        mesher->buildAdjacency();
        ASSERT_EQ(1, mesh->tets.size());

        // get adjacency info
        tet = mesh->tets[0];
        mesh->getAdjacencyListsForTet(tet, verts, edges, faces);

        // make tet regular
        verts[0]->pos() = cleaver::vec3( 1, 1, 1);
        verts[1]->pos() = cleaver::vec3( 1,-1,-1);
        verts[2]->pos() = cleaver::vec3(-1, 1,-1);
        verts[3]->pos() = cleaver::vec3(-1,-1, 1);

        // ensure volume is what we expect
        ASSERT_NEAR(2.666, tet->volume(), 1E-3);
    }

    virtual void TearDown() {
        delete mesh;
        delete mesher;
    }

    cleaver::TetMesh* createTetMesh()
    {
        cleaver::TetMesh *mesh = new cleaver::TetMesh();
        cleaver::Vertex *v1 = new cleaver::Vertex();
        cleaver::Vertex *v2 = new cleaver::Vertex();
        cleaver::Vertex *v3 = new cleaver::Vertex();
        cleaver::Vertex *v4 = new cleaver::Vertex();
        mesh->createTet(v1, v2, v3, v4, 0);
        return mesh;
    }

    cleaver::TetMesh *mesh;
    cleaver::CleaverMesherImp *mesher;

    cleaver::Tet      *tet;
    cleaver::Vertex   *verts[VERTS_PER_TET];
    cleaver::HalfEdge *edges[EDGES_PER_TET];
    cleaver::HalfFace *faces[FACES_PER_TET];
};


TEST_F(MesherTest, BasicTest)
{
    ASSERT_TRUE(true);
}