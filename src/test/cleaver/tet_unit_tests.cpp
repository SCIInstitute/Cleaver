//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Tet Unit Tests
//
// Author: Jonathan Bronson (bronson@sci.utah.ed)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2017, Jonathan Bronson
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
#include "Tet.h"
#include <cmath>

using namespace cleaver;

TEST(tet, volume) {
  Vertex v1,v2,v3,v4;  
  v1.pos() = vec3( 1.f, 0.f,-1.f/sqrt(2.f));
  v2.pos() = vec3(-1.f, 0.f,-1.f/sqrt(2.f));
  v3.pos() = vec3( 0.f, 1.f, 1.f/sqrt(2.f));
  v4.pos() = vec3( 0.f,-1.f, 1.f/sqrt(2.f));
  Tet tet(&v1, &v2, &v3, &v4, 0);
  
  ASSERT_NEAR(0.942809f, tet.volume(), 1E-6);
}

TEST(tet, contains) {
  Vertex v1,v2,v3,v4;  
  Tet tet(&v1, &v2, &v3, &v4, 0);
  Vertex v5,v6;
  
  ASSERT_TRUE(tet.contains(&v1));
  ASSERT_TRUE(tet.contains(&v2));
  ASSERT_TRUE(tet.contains(&v3));
  ASSERT_TRUE(tet.contains(&v4));

  ASSERT_FALSE(tet.contains(&v5));
  ASSERT_FALSE(tet.contains(&v6));
}
