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
  v1.pos() = vec3(0.,0.,sqrt(2./3.) - .5/sqrt(6.));
  v2.pos() = vec3(-.5/sqrt(3.),-.5,-.5/sqrt(6.));
  v3.pos() = vec3(-.5/sqrt(3.),.5,-.5/sqrt(6.));
  v4.pos() = vec3(1./sqrt(3.),0.,-.5/sqrt(6.));
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288, tri.minAngle());
  ASSERT_FLOAT_EQ(70.5288, tri.maxAngle());
  //regular 2
  float xcord = 1./tan(70.5288);
  v1.pos() = vec3( 1., 0.,-1./sqrt(2));
  v2.pos() = vec3(-1., 0.,-1./sqrt(2));
  v3.pos() = vec3( 0., 1., 1./sqrt(2));
  v4.pos() = vec3( 0.,-1., 1./sqrt(2));
  Tet tri2(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(70.5288, tri2.minAngle());
  ASSERT_FLOAT_EQ(70.5288, tri2.maxAngle());
}
//Tests a right angled tet
TEST(AngleTests,Right) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.,0.,1.);
  v2.pos() = vec3(1.,0.,0.);
  v3.pos() = vec3(0.,1.,0.);
  v4.pos() = vec3(0.,0.,0.);
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(54.7356103, tri.minAngle());
  ASSERT_FLOAT_EQ(90.0, tri.maxAngle());
}
//Tests a flat tet
TEST(AngleTests,Flat) {
  Vertex v1,v2,v3,v4;
  v1.pos() = vec3(0.25,0.25,.0);
  v2.pos() = vec3(1.,0.,0.);
  v3.pos() = vec3(0.,1.,0.);
  v4.pos() = vec3(0.,0.,0.);
  Tet tri(&v1,&v2,&v3,&v4,0);
  ASSERT_FLOAT_EQ(0., tri.minAngle());
  ASSERT_FLOAT_EQ(180., tri.maxAngle());
  v1.pos() = vec3(0.,0.,0.);
  v2.pos() = vec3(0.,0.,0.);
  v3.pos() = vec3(0.,0.,0.);
  v4.pos() = vec3(0.,0.,0.);
  Tet tri2(&v1,&v2,&v3,&v4,0);
  ASSERT_TRUE(tri2.minAngle() == 0. || tri2.minAngle() == 180.);
  ASSERT_TRUE(tri2.maxAngle() == 0. || tri2.maxAngle() == 180.);
}
