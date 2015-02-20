//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- vec3 Unit Tests
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
#include "vec3.h"
#include <cmath>

using namespace cleaver;

TEST(vec3, addition) {
    cleaver::vec3 a(1, 2, 3);
    cleaver::vec3 b(4, 5, -6);
    cleaver::vec3 result = a + b;
    cleaver::vec3 expected(5, 7, -3);
    ASSERT_EQ(expected, result);
}

TEST(vec3, subtraction) {
    cleaver::vec3 a(6, 1, 8);
    cleaver::vec3 b(8, 0, 8);
    cleaver::vec3 result = a - b;
    cleaver::vec3 expected(-2, 1, 0);
    ASSERT_EQ(expected, result);
}

TEST(vec3, scalarMultiplicationPrefixed) {
    double a = 2;
    cleaver::vec3 b(8, 1.2, 4);
    cleaver::vec3 result = a * b;
    cleaver::vec3 expected(16, 2.4, 8);
    ASSERT_EQ(expected, result);
}

TEST(vec3, scalarMultiplicationPostfixed) {
    cleaver::vec3 a(8, 1.2, 4);
    double b = 2;
    cleaver::vec3 result = a * b;
    cleaver::vec3 expected(16, 2.4, 8);
    ASSERT_EQ(expected, result);
}

TEST(vec3, scalarDivision) {
    cleaver::vec3 a(8, 1.2, 4);
    double b = 2;
    cleaver::vec3 result = a / b;
    cleaver::vec3 expected(4, 0.6, 2);
    ASSERT_EQ(expected, result);
}

TEST(vec3, toString) {
    cleaver::vec3 x(2, -3, 4.2);
    std::string result = x.toString();
    std::string expected = "[2, -3, 4.2]";
    ASSERT_EQ(expected, result);
}

TEST(vec3, cross) {
    cleaver::vec3 a(1, 0, 0);
    cleaver::vec3 b(0, 1, 0);
    cleaver::vec3 result = a.cross(b);
    cleaver::vec3 expected(0, 0, 1);
    ASSERT_EQ(expected, result);
}

TEST(vec3, dot) {
    cleaver::vec3 a(1, .3, 0.8);
    cleaver::vec3 b(0.2, 0.2, 0.4);
    double result = a.dot(b);
    double expected = 0.58;
    ASSERT_FLOAT_EQ(expected, result);
}

