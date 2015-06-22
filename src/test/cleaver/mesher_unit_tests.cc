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

class ViolationTest : public ::testing::Test {
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

TEST_F(ViolationTest, CutViolatesVertices)
{
    cleaver::HalfEdge *edge = edges[0];
    cleaver::Vertex *cut = new cleaver::Vertex();
    cleaver::Vertex *v1 = edge->vertex;
    cleaver::Vertex *v2 = edge->mate->vertex;
    float alpha = 0.25;
    edge->alpha = alpha;   edge->mate->alpha = alpha;
    edge->cut = cut;       edge->mate->cut = cut;

    edge->cut->pos() = 0.1*v1->pos() + 0.9*v2->pos();
    mesher->checkIfCutViolatesVertices(edge);
    ASSERT_TRUE(cut->violating);
    ASSERT_TRUE(cut->closestGeometry == v2);

    edge->cut->pos() = 0.9*v1->pos() + 0.1*v2->pos();
    mesher->checkIfCutViolatesVertices(edge);
    ASSERT_TRUE(cut->violating);
    ASSERT_TRUE(cut->closestGeometry == v1);

    edge->cut->pos() = 0.5*v1->pos() + 0.5*v2->pos();
    mesher->checkIfCutViolatesVertices(edge);
    ASSERT_FALSE(cut->violating);

    delete cut;
}

TEST_F(ViolationTest, TripleViolatesVertices)
{
    cleaver::HalfFace *face = faces[0];
    cleaver::Vertex *triple = new cleaver::Vertex();
    face->triple = triple;

    cleaver::Vertex   *face_verts[VERTS_PER_FACE];
    cleaver::HalfEdge *face_edges[EDGES_PER_FACE];
    mesh->getAdjacencyListsForFace(face, face_verts, face_edges);

    cleaver::vec3 v0 = face_verts[0]->pos();
    cleaver::vec3 v1 = face_verts[1]->pos();
    cleaver::vec3 v2 = face_verts[2]->pos();

    float alpha = 0.25;
    for(int i=0; i < 3; i++) {
        face_edges[i]->alpha = alpha;
        face_edges[i]->mate->alpha = alpha;
    }

    // violate vertex 1
    face->triple->pos() = (3/3.0*v0) + (0/3.0*v1) + (0/3.0*v2);
    mesher->checkIfTripleViolatesVertices(face);
    ASSERT_TRUE(triple->violating);
    ASSERT_TRUE(triple->closestGeometry == face_verts[0]);

    // violate vertex 2
    face->triple->pos() = (0/3.0*v0) + (3/3.0*v1) + (0/3.0*v2);
    mesher->checkIfTripleViolatesVertices(face);
    ASSERT_TRUE(triple->closestGeometry == face_verts[1]);

    // violate vertex 3
    face->triple->pos() = (0/3.0*v0) + (0/3.0*v1) + (3/3.0*v2);
    mesher->checkIfTripleViolatesVertices(face);
    ASSERT_TRUE(triple->closestGeometry == face_verts[2]);

    // safe at barycenter
    face->triple->pos() = (1/3.0*v0) + (1/3.0*v1) + (1/3.0*v2);
    mesher->checkIfTripleViolatesVertices(face);
    ASSERT_FALSE(triple->violating);

    delete triple;
}

TEST_F(ViolationTest, TripleViolatesEdges)
{
    cleaver::HalfFace *face = faces[0];
    cleaver::Vertex *triple = new cleaver::Vertex();
    face->triple = triple;
    face->triple->order() = TRIP;

    cleaver::Vertex   *face_verts[VERTS_PER_FACE];
    cleaver::HalfEdge *face_edges[EDGES_PER_FACE];
    mesh->getAdjacencyListsForFace(face, face_verts, face_edges);

    cleaver::vec3 v0 = face_verts[0]->pos();
    cleaver::vec3 v1 = face_verts[1]->pos();
    cleaver::vec3 v2 = face_verts[2]->pos();

    float alpha = 0.25;
    for(int i=0; i < 3; i++) {
        face_edges[i]->alpha = alpha;
        face_edges[i]->mate->alpha = alpha;
    }

    // violate edge 1
    face->triple->pos() = 0.5*v0 + 0.5*v1;
    mesher->checkIfTripleViolatesEdges(face);
    ASSERT_TRUE(triple->violating);
    ASSERT_TRUE(triple->closestGeometry == face_edges[2]);

    // violate edge 2
    face->triple->pos() = 0.5*v1 + 0.5*v2;
    mesher->checkIfTripleViolatesEdges(face);
    ASSERT_TRUE(triple->closestGeometry == face_edges[0]);

    // violate edge 3
    face->triple->pos() = 0.5*v2 + 0.5*v0;
    mesher->checkIfTripleViolatesEdges(face);
    ASSERT_TRUE(triple->closestGeometry == face_edges[1]);

    // safe at barycenter
    face->triple->pos() = (1/3.0*v0) + (1/3.0*v1) + (1/3.0*v2);
    mesher->checkIfTripleViolatesEdges(face);
    ASSERT_FALSE(triple->violating);

    delete triple;
}

TEST_F(ViolationTest, QuadrupleViolatesVertices)
{
    cleaver::Vertex *quadruple = new cleaver::Vertex();
    tet->quadruple = quadruple;
    tet->quadruple->order() = QUAD;
    tet->quadruple->violating = false;

    float alpha = 0.1f;
    for (int e = 0; e < EDGES_PER_FACE; e++) {
        edges[e]->alpha = alpha;
        edges[e]->mate->alpha = alpha;
    }

    cleaver::vec3 v0 = verts[0]->pos();
    cleaver::vec3 v1 = verts[1]->pos();
    cleaver::vec3 v2 = verts[2]->pos();
    cleaver::vec3 v3 = verts[3]->pos();

    // violate vertex 1
    tet->quadruple->pos() = v0;
    mesher->checkIfQuadrupleViolatesVertices(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(verts[0], quadruple->closestGeometry);

    // violate vertex 2
    tet->quadruple->pos() = v1;
    mesher->checkIfQuadrupleViolatesVertices(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(verts[1], quadruple->closestGeometry);

    // violate vertex 3
    tet->quadruple->pos() = v2;
    mesher->checkIfQuadrupleViolatesVertices(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(verts[2], quadruple->closestGeometry);

    // violate vertex 4
    tet->quadruple->pos() = v3;
    mesher->checkIfQuadrupleViolatesVertices(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(verts[3], quadruple->closestGeometry);

    // safe at barycenter
    tet->quadruple->pos() = 0.25*v0 + 0.25*v1 + 0.25*v2 + 0.25*v3;
    mesher->checkIfQuadrupleViolatesVertices(tet);
    EXPECT_FALSE(quadruple->violating);

    delete quadruple;
}

TEST_F(ViolationTest, QuadrupleViolatesEdges)
{
    cleaver::Vertex *quadruple = new cleaver::Vertex();
    tet->quadruple = quadruple;
    tet->quadruple->order() = QUAD;
    tet->quadruple->violating = false;

    float alpha = 0.1f;
    for (int e = 0; e < EDGES_PER_FACE; e++) {
        edges[e]->alpha = alpha;
        edges[e]->mate->alpha = alpha;
    }

    cleaver::vec3 v0 = verts[0]->pos();
    cleaver::vec3 v1 = verts[1]->pos();
    cleaver::vec3 v2 = verts[2]->pos();
    cleaver::vec3 v3 = verts[3]->pos();

    // violate edge 1
    tet->quadruple->pos() = 0.5*v0 + 0.5*v1;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[0], quadruple->closestGeometry);

    // violate edge 2
    tet->quadruple->pos() = 0.5*v0 + 0.5*v2;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[1], quadruple->closestGeometry);

    // violate edge 3
    tet->quadruple->pos() = 0.5*v0 + 0.5*v3;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[2], quadruple->closestGeometry);

    // violate edge 4
    tet->quadruple->pos() = 0.5*v1 + 0.5*v2;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[3], quadruple->closestGeometry);

    // violate edge 5
    tet->quadruple->pos() = 0.5*v1 + 0.5*v3;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[4], quadruple->closestGeometry);

    // violate edge 6
    tet->quadruple->pos() = 0.5*v2 + 0.5*v3;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(edges[5], quadruple->closestGeometry);

    // safe at barycenter
    tet->quadruple->pos() = 0.25*v0 + 0.25*v1 + 0.25*v2 + 0.25*v3;
    mesher->checkIfQuadrupleViolatesEdges(tet);
    EXPECT_FALSE(quadruple->violating);

    delete quadruple;
}

TEST_F(ViolationTest, QuadrupleViolatesFaces)
{
    cleaver::Vertex *quadruple = new cleaver::Vertex();
    tet->quadruple = quadruple;
    tet->quadruple->order() = QUAD;
    tet->quadruple->violating = false;

    float alpha = 0.1f;
    for (int e = 0; e < EDGES_PER_FACE; e++) {
        edges[e]->alpha = alpha;
        edges[e]->mate->alpha = alpha;
    }

    cleaver::vec3 v0 = verts[0]->pos();
    cleaver::vec3 v1 = verts[1]->pos();
    cleaver::vec3 v2 = verts[2]->pos();
    cleaver::vec3 v3 = verts[3]->pos();

    // violate face 1
    tet->quadruple->pos() = (1/3.0)*v0 + (1/3.0)*v1 + (1/3.0)*v2;
    mesher->checkIfQuadrupleViolatesFaces(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(faces[0], quadruple->closestGeometry);

    // violate face 2
    tet->quadruple->pos() = (1/3.0)*v0 + (1/3.0)*v2 + (1/3.0)*v3;
    mesher->checkIfQuadrupleViolatesFaces(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(faces[1], quadruple->closestGeometry);

    // violate face 3
    tet->quadruple->pos() = (1/3.0)*v0 + (1/3.0)*v1 + (1/3.0)*v3;
    mesher->checkIfQuadrupleViolatesFaces(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(faces[2], quadruple->closestGeometry);

    // violate face 4
    tet->quadruple->pos() = (1/3.0)*v1 + (1/3.0)*v2 + (1/3.0)*v3;
    mesher->checkIfQuadrupleViolatesFaces(tet);
    EXPECT_TRUE(quadruple->violating);
    EXPECT_EQ(faces[3], quadruple->closestGeometry);

    // safe at barycenter
    tet->quadruple->pos() = 0.25*v0 + 0.25*v1 + 0.25*v2 + 0.25*v3;
    mesher->checkIfQuadrupleViolatesFaces(tet);
    EXPECT_FALSE(quadruple->violating);

    delete quadruple;
}
