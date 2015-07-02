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
#include <stdint.h>
#if WIN32
#include <Windows.h>
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
    uint32_t sz = sizeof(str);
#if WIN32
    HMODULE hModule = GetModuleHandleW(NULL);
    GetModuleFileName(hModule,str,sz);
    for(size_t i = 0; i < sz; i++)
      if (str[i] == '\\') str[i] = '/';
#elif LINUX
    readlink("/proc/self/exe",str,sz);
#elif DARWIN
    _NSGetExecutablePath(str,&sz);
#endif
    std::string tmp(str);
	tmp = tmp.substr(0,tmp.find_last_of("/"));
	if (tmp.rfind("/Release") != std::string::npos || tmp.rfind("/Debug") != std::string::npos)
		tmp = tmp.substr(0,tmp.find_last_of("/"));
	return tmp;
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
      "/unu.exe";
#else
    "/unu";
#endif
    std::string unu_tmp = unu_cmmd;
    //if the file doesn't exist, try release/debug
    std::ifstream tmp1((exe_path + unu_tmp).c_str());
    if (!tmp1.good()) {
      unu_tmp = "/Release" + unu_cmmd;
      std::ifstream tmp2((exe_path + unu_tmp).c_str());
      if (!tmp2.good()) {
        unu_tmp = "/Debug" + unu_cmmd;
        std::ifstream tmp3((exe_path + unu_tmp).c_str());
        //as a last resort, choose bin/unu
        if (!tmp3.good()) {
          unu_tmp = unu_cmmd;
        }
        tmp3.close();
      }
      tmp2.close(); 
    }
    tmp1.close();
    std::string cmmd = "\"" + exe_path + unu_tmp + " minmax " + file + "> tmp\"";
	std::cout << cmmd << std::endl;
    std::system(cmmd.c_str());
    std::ifstream in("tmp");
    int first, second;
    std::string temp;
    in >> temp >> first >> temp >> second;
    in.close();
    return second - first + 1;
  }

  void createIndicatorFunctions(std::vector<std::string> &files, std::string scirun, std::string python) {
	if (files[0].find(" ") != std::string::npos) {
		for (size_t i = 0; i < files[0].size(); i++)
		  if (files[0][i] == ' ' && files[0][i - 1] != '\\')
			files[0].insert(i,"\\");
	}
    //ensure we are using the right slashes for this section.
    for (size_t i = 0; i < files[0].size(); i++)
      if (files[0][i] == '\\' && files[0][i+1] != ' ')
        files[0][i] = '/';
    std::string exe_path = getExecutablePath();
    int total_mats = SegmentationTools::getNumMats(files[0]);
    std::string vol_name = files[0].substr(0,files[0].find_last_of("."));
    vol_name = vol_name.substr(vol_name.find_last_of("/")+1,std::string::npos);
    //create an output folder by this file for the new fields.
    std::string output_dir = files[0].substr(0,
        files[0].find_last_of("/")) + "/" + vol_name + "_material_fields" ;
    //create the python file that the scripts need to create the fields.
    //copy template file
	std::ifstream templat((exe_path + "/ConfigTemplate.py").c_str());
	std::ofstream out((exe_path + "/ConfigUse.py").c_str());
	out << templat.rdbuf();
	templat.close();
    //add "mats" "mat_names" "model_output_path" "model_input_file"
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
    std::string cmmd = "\"" + python + " " + exe_path +
      "/BuildMesh.py -s1:2 --binary-path " + scirun + " "
      + exe_path + "/ConfigUse.py \"";
	std::cout << cmmd << std::endl;
    std::system(cmmd.c_str());
    files.clear();
    for(int i = 0; i < total_mats; i++) {
		//remove unneeded files.
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".tight.nrrd")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".raw")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".lut.nrrd")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".lut.raw")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".solo.nrrd")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".tight.fld")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".tight_transformed.fld")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".tight-corrected.nrrd")).c_str());
		std::remove((output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string("_isosurface.ts.fld")).c_str());
		std::remove((output_dir + "/" +
          vol_name + "_pad_transform.tf").c_str());
		std::remove((output_dir + "/" +
          vol_name + "_pad_unorient.nrrd").c_str());
		std::remove((output_dir + "/" +
          "mesh_state.txt").c_str());
		std::remove((output_dir + "/" +
          "dominant_labelmap.nrrd").c_str());
		std::remove((output_dir + "/" +
          "medial_axis_param_file.txt").c_str());
		std::remove((output_dir + "/" +
          "make-solo-nrrd-runtime.txt").c_str());
		std::remove((output_dir + "/" +
          "compute-material-boundary-runtime.txt").c_str());
		std::remove((output_dir + "/" +
          "labelmap.txt").c_str());
		std::remove((output_dir + "/" +
          "isosurface-all.ts.fld").c_str());
		std::remove((output_dir + "/" +
          "running_stage_1.txt").c_str());
		std::remove((output_dir + "/" +
          "running_stage_2.txt").c_str());
		std::remove((output_dir + "/" +
          "pid_1.txt").c_str());
		std::remove((output_dir + "/" +
          "pid_2.txt").c_str());
		std::remove((output_dir + "/" +
          "completed_stage_1.txt").c_str());
		std::remove((output_dir + "/" +
          "completed_stage_2.txt").c_str());
		//push back the wanted one 
		files.push_back(output_dir + "/" +
          vol_name + ((char)('0' + i)) +
          std::string(".tight_transformed.nrrd"));
	}
  }
}
