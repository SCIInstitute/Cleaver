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
#include "cli_common.h"

//OSX and Linux return opposite byte order shorts after a process completes.
#ifdef _WIN32
#define _MUL 1
#else
#define _MUL 256
#endif

// Tests scaling IO for CLI
TEST(CLIRegressionTests, RegularStrict) {
  //make sure there is a command interpreter
  ASSERT_EQ(0,(int)!(std::system(NULL)));
  //setup the line that calls the command line interface
  std::string log = "strict_output.txt";
  //check for no material fields error.
  std::string output = " >> " + data_dir + log + " 2>&1";
  std::string option = " --strict 2>&1 ";
  std::string line = (command + option + output);
  EXPECT_EQ(1 * _MUL , std::system(line.c_str()));
  //check for sizing field grading error
  option = " --strict --grading 0.2 --sizing_field " 
    + data_dir + "basic/sizing_field.nrrd " + input;
  line = (command + option + output);
  EXPECT_EQ(2 * _MUL , std::system(line.c_str()));
  //check for sizing field multiplier error
  option = " --strict --multiplier 0.2 --sizing_field " 
    + data_dir + "basic/sizing_field.nrrd " + input;
  line = (command + option + output);
  EXPECT_EQ(3 * _MUL , std::system(line.c_str()));
  //check for sizing field scale error
  option = " --strict --scale 0.2 --sizing_field " 
    + data_dir + "basic/sizing_field.nrrd " + input;
  line = (command + option + output);
  EXPECT_EQ(4 * _MUL , std::system(line.c_str()));
  //check for sizing field background error
  option = " --strict --background_mesh " + 
    data_dir + "basic/background  --sizing_field " 
    + data_dir + "basic/sizing_field.nrrd " + input;
  line = (command + option + output);
  EXPECT_EQ(5 * _MUL , std::system(line.c_str()));
  //check for wrong mesh mode error
  option = " --strict --mesh_mode unknown " + input;
  line = (command + option + output);
  EXPECT_EQ(6 * _MUL , std::system(line.c_str()));
  //check for unsupported output format error
  option = " --strict --output_format unknown " + input;
  line = (command + option + output);
  EXPECT_EQ(7 * _MUL , std::system(line.c_str()));
  //check for exception error (bad option)
  option = " --strict --badthing " + input;
  line = (command + option + output);
  EXPECT_EQ(8 * _MUL , std::system(line.c_str()));
  //check for bad input material field
  option = " --strict --input_files " + data_dir + 
    "basic/basic_output.log " + input;
  line = (command + option + output);
  EXPECT_EQ(10 * _MUL , std::system(line.c_str()));
  system_execute(RM_CMMD,data_dir + log);
}
