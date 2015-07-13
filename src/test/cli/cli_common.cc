//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//
// -- Cleaver-CLI Tests
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
#include <stdint.h>
#include "cli_common.h"
std::string diff = std::string("diff -wB ");
std::string data_dir = std::string(TEST_DATA_DIR);
std::string command = std::string(BINARY_DIR) + _CLI + " -v ";
std::string name = _NAME + std::string("output");
std::string path = _PATH + data_dir;
std::string seg_input = _FIELDS +
data_dir + "input/mickey.nrrd " ;
std::string input = _FIELDS +
data_dir + "input/spheres1.nrrd " +
data_dir + "input/spheres2.nrrd " +
data_dir + "input/spheres3.nrrd " +
data_dir + "input/spheres4.nrrd " ;
std::string scirun = std::string(SCIRun4_DIR) + " ";
std::string files[num_files] = {
  "sizing_field.nrrd",
  "boundary_field.nrrd",
  "boundary.nrrd",
  "feature_field.nrrd",
  "medial.nrrd"};

void system_execute(const std::string cmmd, const std::string args) {
	std::string new_args;
#if WIN32
	for(size_t i = 0; i < args.size(); i++) 
		if (args[i] == '/') new_args = new_args + "\\";
		else new_args = new_args + args[i];
#else
	new_args = args;
#endif
	std::system((cmmd + new_args).c_str());
}


void compareEleFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  size_t count0, dim0, att0;
  size_t count1, dim1, att1;
  char line[512];
  test.getline(line,512);
  ans.getline(line,512);
  test >> count0 >> dim0 >> att0;
  ans  >> count1 >> dim1 >> att1;
  ASSERT_EQ(count0,count1);
  ASSERT_EQ(dim0,dim1);
  ASSERT_EQ(att0,att1);
  test.getline(line,512);
  ans.getline(line,512);
  for (size_t i = 0; i < count0; i++) {
    size_t tmp = 0, tmp2 = 0;
    size_t num0 = 0, num1 = 0;
    test >> num0; ans >> num1;
    ASSERT_EQ(num0,num1);
    for(size_t j = 0; j < dim0; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_EQ(tmp,tmp2);
    }
    for(size_t j = 0; j < att0; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_EQ(tmp,tmp2);
    }
  }
  test.close();
  ans.close();
}

void compareNodeFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  size_t count0, dim0, att0, bnd0;
  size_t count1, dim1, att1, bnd1;
  char line[512];
  test.getline(line,512);
  ans.getline(line,512);
  test >> count0 >> dim0 >> att0 >> bnd0;
  ans  >> count1 >> dim1 >> att1 >> bnd1;
  ASSERT_EQ(count0,count1);
  ASSERT_EQ(dim0,dim1);
  ASSERT_EQ(att0,att1);
  ASSERT_EQ(bnd0,bnd1);
  test.getline(line,512);
  ans.getline(line,512);
  for (size_t i = 0; i < count0; i++) {
    float tmp = 0.f, tmp2 = 0.f;
    size_t num0 = 0, num1 = 0;
    test >> num0; ans >> num1;
    ASSERT_EQ(num0,num1);
    for(size_t j = 0; j < dim0; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_FLOAT_EQ(tmp,tmp2);
    }
    for(size_t j = 0; j < att0; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_FLOAT_EQ(tmp,tmp2);
    }
    for(size_t j = 0; j < bnd0; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_FLOAT_EQ(tmp,tmp2);
    }
  }
  test.close();
  ans.close();
}

void compareVTKFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  int point_count0, poly_count0;
  int point_count1, poly_count1;
  char line[512];
  for(int i = 0; i < 4; i++) {
    test.getline(line,512);
    ans.getline(line,512);
  }
  test.getline(line,512);
  sscanf(line,"POINTS %d float",&point_count0);
  ans.getline(line,512);
  sscanf(line,"POINTS %d float",&point_count1);
  ASSERT_EQ(point_count0,point_count1);
  for (int i = 0; i < point_count0; i++) {
    float tmp = 0.f, tmp2 = 0.f;
    for(size_t j = 0; j < 3; j++) {
      test >> tmp; ans >> tmp2;
      ASSERT_FLOAT_EQ(tmp,tmp2);
    }
  }
  test.getline(line,512);
  ans.getline(line,512);
  test.getline(line,512);
  sscanf(line,"POLYGONS %d %d",&point_count0,&poly_count0);
  ans.getline(line,512);
  sscanf(line,"POLYGONS %d %d",&point_count1,&poly_count1);
  ASSERT_EQ(point_count0,point_count1);
  ASSERT_EQ(poly_count0,poly_count1);
  for (int i = 0; i < poly_count0; i++) {
    size_t num0 = 0, num1 = 0;
    test >> num0; ans >> num1;
    ASSERT_EQ(num0,num1);
  }
  test.close();
  ans.close();
}

void comparePtsFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  float tmp1, tmp2;
  while(!test.eof() && !ans.eof()) {
    test >> tmp1 ; ans >> tmp2;
    ASSERT_FLOAT_EQ(tmp1,tmp2);
  }
  test.close();
  ans.close();
}

void compareElemFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  int tmp1, tmp2;
  while(!test.eof() && !ans.eof()) {
    test >> tmp1 ; ans >> tmp2;
    ASSERT_EQ(tmp1,tmp2);
  }
  test.close();
  ans.close();
}

template<typename T>
void testValue(std::ifstream &a, std::ifstream &b, T &value) {
  T test,ans;
  a.read((char*)&test,sizeof(T));
  b.read((char*)&ans,sizeof(T));
  ASSERT_EQ(test,ans);
  value = ans;
}
#ifdef _WIN32
#define float_t float
#endif

void testValueFloat(std::ifstream &a, std::ifstream &b) {
  float_t test,ans;     
  a.read((char*)&test,sizeof(float_t));
  b.read((char*)&ans,sizeof(float_t));
  ASSERT_FLOAT_EQ(test,ans);
}

void compareMatFiles(const std::string a, const std::string b) {
  ASSERT_FALSE(a == b);
  std::ifstream test(b.c_str(),std::ifstream::in),
    ans(a.c_str(),std::ifstream::in);
  //consume the descrip buffer. don't care.
  char desc[116];
  test.read(desc,116); ans.read(desc,116);
  //check the offset
  char my_byte;
  for (size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //check the version
  int16_t my_short;
  testValue(test,ans,my_short);
  //check the endianness
  testValue(test,ans,my_short);
  //get the maintype and totalsize
  int32_t totalSize;
  testValue(test,ans,totalSize);
  testValue(test,ans,totalSize);
  //get the flags
  int32_t my_word;
  testValue(test,ans,my_word);
  testValue(test,ans,my_word);
  //get the next eight bytes
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //get the next six words
  for(size_t i = 0; i < 6; i++)
    testValue(test,ans,my_word);
  //get the next eight bytes
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //get the next two shorts
  for(size_t i = 0; i < 2; i++)
    testValue(test,ans,my_short);
  //get the next three words
  for(size_t i = 0; i < 3; i++)
    testValue(test,ans,my_word);
  //get the next 24 bytes
  for(size_t i = 0; i < 24; i++)
    testValue(test,ans,my_byte);
  //the node info reading starts here.
  //get the next 4 words
  for(size_t i = 0; i < 4; i++)
    testValue(test,ans,my_word);
  //get the next eight bytes
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //get the next 3 words
  for(size_t i = 0; i < 3; i++)
    testValue(test,ans,my_word);
  //now we're actually starting the data reading
  int32_t num_verts;
  testValue(test,ans,num_verts);
  //get the next 4 words
  for(size_t i = 0; i < 4; i++)
    testValue(test,ans,my_word);
  //get the node values xyz * node_size
  for(int32_t i = 0; i < num_verts * 3; i++) {
    testValueFloat(test,ans);
  }
  //possible padding
  int32_t data_size = 3 * num_verts * sizeof(float_t);
  int32_t padding = (8 - (data_size % 8)) % 8;
  for(int32_t i = 0; i < padding; i++)
    testValue(test,ans,my_byte);
  //now the cell data
  //get the next 4 words
  for(size_t i = 0; i < 4; i++)
    testValue(test,ans,my_word);
  //get the next eight bytes
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //get the next 3 words
  for(size_t i = 0; i < 3; i++)
    testValue(test,ans,my_word);
  //now we're actually starting the data reading
  int32_t num_cells;
  testValue(test,ans, num_cells);
  //get the next 4 words
  for(size_t i = 0; i < 4; i++)
    testValue(test,ans,my_word);
  //get the cell values ... 4 indicies
  for(int32_t i = 0; i < num_cells * 4; i++)
    testValue(test,ans,my_word);
  //possible padding
  data_size = 4 * num_cells * sizeof(int32_t);
  padding = (8 - (data_size % 8)) % 8;
  for(int32_t i = 0; i < padding; i++)
    testValue(test,ans,my_byte);
  //now we're at the field field...
  //get the next 4 words
  for(size_t i = 0; i < 4; i++)
    testValue(test,ans,my_word);
  //get the next eight bytes
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_byte);
  //get the next 8 words
  for(size_t i = 0; i < 8; i++)
    testValue(test,ans,my_word);
  //now we're actually starting the data reading
  //get the cell field mat values ...
  for(int32_t i = 0; i < num_cells; i++)
    testValue(test,ans,my_byte);
  //possible padding
  data_size = num_cells * sizeof(int8_t);
  padding = (8 - (data_size % 8)) % 8;
  for(int32_t i = 0; i < padding; i++)
    testValue(test,ans,my_byte);
  //that's all folks!
  test.close();
  ans.close();
}
