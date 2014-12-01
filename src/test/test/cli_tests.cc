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
#include "gtest/gtest.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#define _FIELDS " --material_fields "
#define _FILES  "sphere*.nrrd "
#define _NAME   " --output_name "
#define _PATH   " --output_path "
#define _CLI    "cleaver-cli "
#define _DIFF   "cmake -E compare_files "
static std::string data_dir = std::string(TEST_DATA_DIR);
static std::string command = std::string(BINARY_DIR) + _CLI + " -v ";
static std::string name = _NAME + std::string("output");
static std::string path = _PATH + data_dir;
static std::string input = _FIELDS + data_dir + "input/" +_FILES;
static std::vector<std::string> files;

// Tests basic IO for CLI
TEST(CLIRegressionTests, Basic) {
  //make sure there is a command interpreter
  ASSERT_EQ(0,(int)!(std::system(NULL)));
  //add the common files to the list
  files.push_back("background.node");
  files.push_back("background.ele");
  files.push_back("sizing_field.nrrd");
  files.push_back("boundary_field.nrrd");
  files.push_back("boundary.nrrd");
  files.push_back("feature_field.nrrd");
  files.push_back("medial.nrrd");
  files.push_back("output.info");
  //setup the line that calls the command line interface
  files.push_back("output.node");
  files.push_back("output.ele");
  std::string log = "basic_output.txt";
  files.push_back(log);
  std::string output = " > " + data_dir + log + " 2>&1";
  std::string line = (command + name + path + input + output);
  //make sure there was no error from the command line
  ASSERT_EQ(0, std::system(line.c_str()));
  //move the other generated files in the current dir to the test dir
  for(size_t i = 0; i < 7; i++) {
    std::system(("mv " + files[i] + " " + data_dir).c_str());
  }
  //compare all of the files
  std::string diff(_DIFF);
  for(size_t i = 0; i < files.size() - 1; i++) {
    ASSERT_EQ(0, std::system((diff + data_dir +
            files[i] + " " + data_dir +
            "basic/" + files[i]).c_str()));
  }
  //delete the output files from this test
  for(size_t i = 0; i < files.size(); i++) {
    std::system(("rm " + data_dir + files[i]).c_str());
  }
  //put the list of files back to the common ones
  while (files.size() > 8) files.pop_back();
}
// Tests scaling IO for CLI
TEST(CLIRegressionTests, Scaling) {
  //setup the line that calls the command line interface
  files.push_back("output.node");
  files.push_back("output.ele");
  std::string log = "scaling_output.txt";
  files.push_back(log);
  std::string output = " > " + data_dir + log + " 2>&1";
  std::string scale = " --scale 1.0 ";
  std::string line = (command + name + path + scale + input + output);
  //make sure there was no error from the command line
  ASSERT_EQ(0, std::system(line.c_str()));
  //move the other generated files in the current dir to the test dir
  for(size_t i = 0; i < 7; i++) {
    std::system(("mv " + files[i] + " " + data_dir).c_str());
  }
  //compare all of the files
  std::string diff(_DIFF);
  for(size_t i = 0; i < files.size() - 1; i++) {
    ASSERT_EQ(0, std::system((diff + data_dir +
            files[i] + " " + data_dir +
            "scaling/" + files[i]).c_str()));
  }
  //delete the output files from this test
  for(size_t i = 0; i < files.size(); i++) {
  //  std::system(("rm " + data_dir + files[i]).c_str());
  }
  //put the list of files back to the common ones
  while (files.size() > 8) files.pop_back();
}
