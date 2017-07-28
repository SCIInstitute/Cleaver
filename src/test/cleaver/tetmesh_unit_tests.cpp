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
  Tet tet(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288f, tet.minAngle());
  ASSERT_FLOAT_EQ(70.5288f, tet.maxAngle());
  //regular 2
  v1.pos() = vec3( 1.f, 0.f,-1.f/sqrt(2.f));
  v2.pos() = vec3(-1.f, 0.f,-1.f/sqrt(2.f));
  v3.pos() = vec3( 0.f, 1.f, 1.f/sqrt(2.f));
  v4.pos() = vec3( 0.f,-1.f, 1.f/sqrt(2.f));
  Tet tet2(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288f, tet2.minAngle());
  ASSERT_FLOAT_EQ(70.5288f, tet2.maxAngle());
}
//Tests a right angled tet
TEST(AngleTests,Right) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.f,0.f,1.f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tet(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(54.7356103f, tet.minAngle());
  ASSERT_FLOAT_EQ(90.0f, tet.maxAngle());
}
//Tests a flat tet
TEST(AngleTests,Flat) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.25f,0.25f,.0f);
  v2.pos() = vec3(1.f,0.f,0.f);
  v3.pos() = vec3(0.f,1.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tet(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(0.f, tet.minAngle());
  ASSERT_FLOAT_EQ(180.f,tet.maxAngle());
  v1.pos() = vec3(0.f,0.f,0.f);
  v2.pos() = vec3(0.f,0.f,0.f);
  v3.pos() = vec3(0.f,0.f,0.f);
  v4.pos() = vec3(0.f,0.f,0.f);
  Tet tet2(&v1,&v2,&v3,&v4,0);
  ASSERT_TRUE(tet2.minAngle() == 0.f || tet2.minAngle() == 180.f);
  ASSERT_TRUE(tet2.maxAngle() == 0.f || tet2.maxAngle() == 180.f);
}
