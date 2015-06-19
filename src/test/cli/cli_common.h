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
#include <cmath>
#define _FIELDS " --material_fields "
#define _NAME   " --output_name "
#define _PATH   " --output_path "
#define _CLI    "cleaver-cli "
#define num_files 5
#if WIN32
#define RM_CMMD " del /q "
#define RMDIR_CMMD " rmdir /s /q "
#define CAT_CMMD " type "
#define MV_CMMD " move "
#else
#define RM_CMMD " rm "
#define RMDIR_CMMD " rm -rf "
#define CAT_CMMD " cat "
#define MV_CMMD " mv "
#endif
extern std::string diff; 
extern std::string data_dir; 
extern std::string command; 
extern std::string name; 
extern std::string path; 
extern std::string input; 
extern std::string seg_input; 
extern std::string files[num_files]; 

void system_execute(const std::string cmmd, const std::string args);
void compareEleFiles(const std::string a, const std::string b);
void compareNodeFiles(const std::string a, const std::string b);
void compareVTKFiles(const std::string a, const std::string b) ;
void comparePtsFiles(const std::string a, const std::string b) ;
void compareElemFiles(const std::string a, const std::string b) ;
void compareMatFiles(const std::string a, const std::string b) ;
