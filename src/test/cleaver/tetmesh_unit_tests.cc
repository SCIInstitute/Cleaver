//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- TetMesh Tests
//
// Primary Author: Brig Bagley (brig@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2014, Brig Bagley
//  Scientific Computing & Imaging Institute
//
//  University of Utah
//  //
//  //  Permission is  hereby  granted, free  of charge, to any person
//  //  obtaining a copy of this software and associated documentation
//  //  files  ( the "Software" ),  to  deal in  the  Software without
//  //  restriction, including  without limitation the rights to  use,
//  //  copy, modify,  merge, publish, distribute, sublicense,  and/or
//  //  sell copies of the Software, and to permit persons to whom the
//  //  Software is  furnished  to do  so,  subject  to  the following
//  //  conditions:
//  //
//  //  The above  copyright notice  and  this permission notice shall
//  //  be included  in  all copies  or  substantial  portions  of the
//  //  Software.
//  //
//  //  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
//  //  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
//  //  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
//  //  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
//  //  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
//  //  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  //  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//  //  USE OR OTHER DEALINGS IN THE SOFTWARE.
//  //-------------------------------------------------------------------
//  //-------------------------------------------------------------------
#include "TetMesh.h"
#include "gtest/gtest.h"
#include <cmath>
using namespace cleaver;
// Tests min/max Angles of regular tet
TEST(AngleTests, Regular) {
  //regular 1
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.f,0.f,sqrt(2.f/3.f) - .5f/sqrt(6.f));
  v2.pos() = vec3(-.5f/sqrt(3.f),-.5f,-.5f/sqrt(6.f));
  v3.pos() = vec3(-.5f/sqrt(3.f),.5f,-.5f/sqrt(6.f));
  v4.pos() = vec3(1.f/sqrt(3.f),0.f,-.5f/sqrt(6.f));
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288f, tri.minAngle());
  ASSERT_FLOAT_EQ(70.5288f, tri.maxAngle());
  //regular 2
  v1.pos() = vec3( 1.f, 0.f,-1.f/sqrt(2.f));
  v2.pos() = vec3(-1.f, 0.f,-1.f/sqrt(2.f));
  v3.pos() = vec3( 0.f, 1.f, 1.f/sqrt(2.f));
  v4.pos() = vec3( 0.f,-1.f, 1.f/sqrt(2.f));
  Tet tri2(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288f, tri2.minAngle());
  ASSERT_FLOAT_EQ(70.5288f, tri2.maxAngle());
}
//Tests a right angled tet
TEST(AngleTests,Right) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.f,0.f,1.f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(54.7356103f, tri.minAngle());
  ASSERT_FLOAT_EQ(90.0f, tri.maxAngle());
}
//Tests a flat tet
TEST(AngleTests,Flat) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.25f,0.25f,.0f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(0.f, tri.minAngle());
  ASSERT_FLOAT_EQ(180.f, tri.maxAngle());
  v1.pos() = vec3(0.f,0.f,0.f);
  v2.pos() = vec3(0.f,0.f,0.f);
  v3.pos() = vec3(0.f,0.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tri2(&v1,&v2,&v3,&v4,0);
  ASSERT_TRUE(tri2.minAngle() == 0.f || tri2.minAngle() == 180.f);
  ASSERT_TRUE(tri2.maxAngle() == 0.f || tri2.maxAngle() == 180.f);
}
//test determinant
TEST(Tetmesh, Determinant) {
  //get determinant of a right tet
  TetMesh t;
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3( 1.f, 0.f,-1.f/sqrt(2.f));
  v2.pos() = vec3(-1.f, 0.f,-1.f/sqrt(2.f));
  v3.pos() = vec3( 0.f, 1.f, 1.f/sqrt(2.f));
  v4.pos() = vec3( 0.f,-1.f, 1.f/sqrt(2.f));
  Tet *tet = new Tet(&v1,&v2,&v3,&v4,0);
  t.tets.push_back(tet);
  float m[16] = {1,1,1,1,1, -1, 0,0, 0,0, 1,-1, 
    -1/sqrt(2.f),-1/sqrt(2.f),1/sqrt(2.f),1/sqrt(2.f)};
  ASSERT_FLOAT_EQ(t.getDeterminant(m),-5.65685424f);
  ASSERT_FLOAT_EQ(t.getDeterminant(m)/6.f,t.getJacobian(tet));
}
//test jacobian
TEST(Tetmesh, Jacobian) {
  //flat will be zero
  TetMesh t;
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.25f,0.25f,.0f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet *tet = new Tet(&v1,&v2,&v3,&v4,0);
  t.tets.push_back(tet);
  ASSERT_FLOAT_EQ(t.getJacobian(tet),0);
  //right will be negative
  v1.pos() = vec3(0.f,0.f,1.f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);

  delete tet;
  tet = new Tet(&v1,&v2,&v3,&v4,0);
  t.tets.clear();
  t.tets.push_back(tet);
  ASSERT_TRUE(t.getJacobian(tet) < 0);
  //flipped right will be positive
  v1.pos() = vec3(0.f,0.f,1.f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v4.pos() = vec3(0.f,1.f,0.f);
  v3.pos() = vec3(0.f,0.f,0.f);

  delete tet;
  tet = new Tet(&v1,&v2,&v3,&v4,0);
  t.tets.clear();
  t.tets.push_back(tet);
  ASSERT_TRUE(t.getJacobian(tet) > 0);
}
//tests remove flat tets.
TEST(Tetmesh, RemoveFlat) {
  TetMesh t;
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.25f,0.25f,.0f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet* tet = new Tet(&v1,&v2,&v3,&v4,0);
  ASSERT_EQ(t.removeFlatTetsAndFixJacobians(true),0);
  Vertex v5,v6,v7,v8;
  v5.pos() = vec3(0.f,0.f,1.f);
  v6.pos() = vec3(1.f,0.f,0.f);
  v7.pos() = vec3(0.f,1.f,0.f);
  v8.pos() = vec3(0.f,0.f,0.f);
  Tet* tet2 = new Tet(&v5,&v6,&v7,&v8,0);
  t.tets.push_back(tet2);
  t.tets.push_back(tet);
  ASSERT_EQ(t.removeFlatTetsAndFixJacobians(true),1);
  ASSERT_EQ(t.tets.size(),1);
}
