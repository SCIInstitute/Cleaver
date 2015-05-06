//-------------------------------------------------------------------
//
//  Copyright (C) 2015
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

#include <SegmentationTools.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#if WIN32
#include <Winbase.h>
#elif LINUX
#include <unistd.h>
#elif DARWIN
#include <mach-o/dyld.h>
#endif

namespace SegmentationTools {

  std::string getExecutablePath() {
    // get the current executable directory.
    std::string exe_path;
    char str[512];
    int sz = sizeof(str);
#if WIN32
    GetModuleFileName(NULL,str,sz);
#elif LINUX
    readlink("/proc/self/exe",str,sz);
#elif DARWIN
    _NSGetExecutablePath(str,&sz);
#endif
    exe_path = std::string(str);
    return exe_path.substr(0,exe_path.find_last_of("/"));
  }

  std::string getNRRDType(std::string file){
    std::ifstream nrrd(file.c_str());
    std::string tmp;
    nrrd >> tmp;
    nrrd.close();
    return tmp;
  }

  int getNumMats(std::string file) {
    std::string exe_path = getExecutablePath();
    std::string unu_cmmd = 
#if WIN32
      "/unu.exe minmax ";
#else
    "/unu minmax ";
#endif
    std::system((exe_path + unu_cmmd + file + " > tmp").c_str());
    std::ifstream in("tmp");
    int first, second;
    std::string tmp;
    in >> tmp >> first >> tmp >> second;
    in.close();
    return second - first + 1;
  }

  void createIndicatorFunctions(std::vector<std::string> &files) {
    std::string exe_path = getExecutablePath();
    int total_mats = SegmentationTools::getNumMats(files[0]);
    std::string vol_name = files[0].substr(0,files[0].find_last_of("."));
    vol_name = vol_name.substr(vol_name.find_last_of("/")+1,std::string::npos);
    //create an output folder by this file for the new fields.
    std::string output_dir = files[0].substr(0,
        files[0].find_last_of("/")) + "/" + vol_name + "_material_fields" ;
    //create the python file that the scripts need to create the fields.
    //copy template file
    std::system(("cat " + exe_path + "/ConfigTemplate.py > " +
          exe_path + "/ConfigUse.py ").c_str());
    //add "mats" "mat_names" "model_output_path" "model_input_file"
    std::ofstream out((exe_path +
          "/ConfigUse.py").c_str(),std::ios::app);
    out << "model_input_file=\"" << files[0] << "\"" << std::endl;
    out << "model_output_path=\"" << output_dir << "\"" << std::endl;
    out << "mats = (";
    for(int i = 0; i < total_mats; i++)
      out << i << ((i+1)==total_mats?")\n":", ");
    out << "mat_names = (";
    for(int i = 0; i < total_mats; i++)
      out << "'" << vol_name << i << "'" << ((i+1)==total_mats?")\n":", ");
    out.close();
    //call the python script to make the fields
    std::string cmmd = "python " + exe_path +
      "/BuildMesh.py -s1:2 --binary-path " + exe_path
      + " " + exe_path + "/ConfigUse.py";
    std::system(cmmd.c_str());
    //delete all of the unneeded files.
    std::system(("rm -f " + output_dir + "/*.log").c_str());
    std::system(("rm -f " + output_dir + "/*.txt").c_str());
    std::system(("rm -f " + output_dir + "/*.fld").c_str());
    std::system(("rm -f " + output_dir + "/*.raw").c_str());
    std::system(("rm -f " + output_dir + "/*.tight.nrrd").c_str());
    std::system(("rm -f " + output_dir + "/*corrected.nrrd").c_str());
    std::system(("rm -f " + output_dir + "/*solo.nrrd").c_str());
    std::system(("rm -f " + output_dir + "/*lut.nrrd").c_str());
    std::system(("rm -f " + output_dir + "/*unorient.nrrd").c_str());
    std::system(("rm -f " + output_dir + "/*.tf").c_str());
    std::system(("rm -f " + output_dir + "/*labelmap.nrrd").c_str());
    std::system("rm -f tmp");
    files.clear();
    for(int i = 0; i < total_mats; i++)
      files.push_back(output_dir + std::string("/")
          + vol_name + ((char)('0' + i)) +
          std::string(".tight_transformed.nrrd"));
  }
}
