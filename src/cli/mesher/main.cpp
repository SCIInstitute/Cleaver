

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//  - Command Line Program
//
// Primary Author: Jonathan Bronson (bronson@sci.utah.edu)
//
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
//
//-------------------------------------------------------------------
//
//  Copyright (C) 2014, Jonathan Bronson
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
//
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#include <Cleaver/Cleaver.h>
#include <Cleaver/CleaverMesher.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/SizingFieldCreator.h>
#include <Cleaver/Timer.h>
#include <nrrd2cleaver/nrrd2cleaver.h>

#include <boost/program_options.hpp>

// STL Includes
#include <exception>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>

#ifdef WIN32
#define NOMINMAX
#endif

const std::string scirun = "scirun";
const std::string tetgen = "tetgen";
const std::string matlab = "matlab";
const std::string vtk = "vtk";

const std::string kDefaultOutputPath   = "./";
const std::string kDefaultOutputName   = "output";
const cleaver::MeshFormat kDefaultOutputFormat = cleaver::Tetgen;


const double kDefaultAlpha = 0.4;
const double kDefaultAlphaLong = 0.357;
const double kDefaultAlphaShort = 0.203;
const double kDefaultScale = 1.0;
const double kDefaultLipschitz = 0.2;
const double kDefaultMultiplier = 1.0;
const int    kDefaultPadding = 0;
const int    kDefaultMaxIterations = 1000;

namespace po = boost::program_options;

// Entry Point
int main(int argc,  char* argv[])
{
  bool verbose = false;
  bool fix_tets = false;
  std::vector<std::string> material_fields;
  std::string sizing_field;
  std::string background_mesh;
  std::string output_path = kDefaultOutputPath;
  std::string output_name = kDefaultOutputName;
  double alpha = kDefaultAlpha;
  double alpha_long = kDefaultAlphaLong;
  double alpha_short = kDefaultAlphaShort;
  double scale = kDefaultScale;
  double lipschitz = kDefaultLipschitz;
  double multiplier = kDefaultMultiplier;
  int padding = kDefaultPadding;
  bool have_sizing_field = false;
  bool have_background_mesh = false;
  bool write_background_mesh = false;
  bool strict = false;
  bool strip_exterior = false;
  enum cleaver::MeshType mesh_mode = cleaver::Structured;
  cleaver::MeshFormat output_format = kDefaultOutputFormat;

  double sizing_field_time = 0;
  double   background_time = 0;
  double     cleaving_time = 0;


  //-------------------------------
  //  Parse Command Line Params
  //-------------------------------
  try{
    po::options_description description("Command line flags");
    description.add_options()
      ("help,h", "display help message")
      ("verbose,v", "enable verbose output")
      ("version", "display version information")
      ("material_fields,i", po::value<std::vector<std::string> >()->multitoken(), "material field paths")
      ("background_mesh,b", po::value<std::string>(), "input background mesh")
      ("mesh_mode,m", po::value<std::string>(), "background mesh mode (structured [default], regular)")
      ("alpha,a", po::value<double>(), "initial alpha value")
      ("alpha_short,s", po::value<double>(), "alpha short value for regular mesh_mode")
      ("alpha_long,l", po::value<double>(), "alpha long value for regular mesh_mode")
      ("sizing_field,z", po::value<std::string>(), "sizing field path")
      ("grading,g", po::value<double>(), "sizing field grading")
      ("multiplier,x", po::value<double>(), "sizing field multiplier")
      ("scale,c", po::value<double>(), "sizing field scale")
      ("padding,p", po::value<int>(), "volume padding")
      ("write_background_mesh,w", "write background mesh")
      ("fix_tet_windup,j", "Ensure positive Jacobians with proper vertex wind-up.")
      ("strip_exterior,e", "strip exterior tetrahedra")
      ("output_path,o", po::value<std::string>(), "output path prefix")
      ("output_name,n", po::value<std::string>(), "output mesh name [default 'output']")
      ("output_format,f", po::value<std::string>(), "output mesh format (tetgen [default], scirun, matlab, vtk)")
      ("strict,t", "warnings become errors")
      ;

    boost::program_options::variables_map variables_map;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
    boost::program_options::notify(variables_map);

    // print help
    if (variables_map.count("help") || (argc == 1)) {
      std::cout << description << std::endl;
      return 0;
    }

    // print version info
    if (variables_map.count("version")) {
      std::cout << cleaver::Version << std::endl;
      return 0;
    }

    // enable verbose mode
    if (variables_map.count("verbose")) {
      verbose = true;
    }

    if (variables_map.count("strict")) {
      strict = true;
    }

    // parse the material field input file names
    if (variables_map.count("material_fields")) {
      material_fields = variables_map["material_fields"].as<std::vector<std::string> >();
    }
    else{
      std::cout << "Error: At least one material field file must be specified." << std::endl;
      return 1;
    }

    //-----------------------------------------
    // parse the sizing field input file name
    // and check for conflicting parameters
    //----------------------------------------
    if (variables_map.count("sizing_field")) {
      have_sizing_field = true;
      sizing_field = variables_map["sizing_field"].as<std::string>();

      if (variables_map.count("grading")){
        if (!strict)
          std::cerr << "Warning: sizing field provided, grading will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and grading parameter provided." << std::endl;
          return 2;
        }
      }
      if (variables_map.count("multiplier")) {
        if (!strict)
          std::cerr << "Warning: sizing field provided, multiplier will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and multiplier parameter provided." << std::endl;
          return 3;
        }
      }
      if (variables_map.count("scale")) {
        if (!strict)
          std::cerr << "Warning: sizing field provided, scale will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and scale parameter provided." << std::endl;
          return 4;
        }
      }
    }

    // parse sizing field parameters
    if (variables_map.count("grading")) {
      lipschitz = variables_map["grading"].as<double>();
    }
    if (variables_map.count("multiplier")) {
      multiplier = variables_map["multiplier"].as<double>();
    }
    if (variables_map.count("scale")) {
      scale = variables_map["scale"].as<double>();
    }
    if (variables_map.count("padding")) {
      padding = variables_map["padding"].as<int>();
    }
    fix_tets = variables_map.count("fix_tet_windup")==0?false:true;

    if (variables_map.count("alpha")) {
      alpha = variables_map["alpha"].as<double>();
    }
    if (variables_map.count("alpha_short")) {
      alpha_short = variables_map["alpha_short"].as<double>();
    }
    if (variables_map.count("alpha_long")) {
      alpha_long = variables_map["alpha_long"].as<double>();
    }

    if (variables_map.count("background_mesh")) {
      have_background_mesh = true;
      background_mesh = variables_map["background_mesh"].as<std::string>();

      if (variables_map.count("sizing_field")) {
        if (!strict)
          std::cerr << "Warning: background mesh provided, sizing field will be ignored." << std::endl;
        else {
          std::cerr << "Error: both background mesh and sizing field provided." << std::endl;
          return 5;
        }
      }
    }


    // parse the background mesh mode
    if (variables_map.count("mesh_mode")) {
      std::string mesh_mode_string = variables_map["mesh_mode"].as<std::string>();
      if(mesh_mode_string.compare("regular") == 0) {
        mesh_mode = cleaver::Regular;
      }
      else if(mesh_mode_string.compare("structured") == 0) {
        mesh_mode = cleaver::Structured;
      } else {
        std::cerr << "Error: invalid background mesh mode: " << mesh_mode_string << std::endl;
        std::cerr << "Valid Modes: [regular] [structured] " << std::endl;
        return 6;
      }
    }

    // does user want background mesh improvement?
    //if (variables_map.count("mesh_improve")) {
    //  improve_mesh = true;
    //}

    // strip exterior tetra after mesh generation
    if (variables_map.count("strip_exterior")) {
      strip_exterior = true;
    }

    // enable writing background mesh if request
    if (variables_map.count("write_background_mesh")) {
      write_background_mesh = true;
    }

    // set the proper output mesh format
    if (variables_map.count("output_format")) {
      std::string format_string = variables_map["output_format"].as<std::string>();
      if(format_string.compare(tetgen) == 0){
        output_format = cleaver::Tetgen;
      }
      else if(format_string.compare(scirun) == 0){
        output_format = cleaver::Scirun;
      }
      else if(format_string.compare(matlab) == 0){
        output_format = cleaver::Matlab;
      }
      else if(format_string.compare(vtk) == 0){
        output_format = cleaver::VTK;
      }
      else{
        std::cerr << "Error: unsupported output format: " << format_string << std::endl;
        return 7;
      }
    }

    // set output path
    if (variables_map.count("output_path")) {
      output_path = variables_map["output_path"].as<std::string>();
    }

    // set output mesh name
    if (variables_map.count("output_name")) {
      output_name = variables_map["output_name"].as<std::string>();
    }

  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 8;
  }
  catch(...) {
    std::cerr << "Unhandled exception caught. Terminating." << std::endl;
    return 9;
  }

  //-----------------------------------
  //  Load Data & Construct Volume
  //-----------------------------------
  cleaver::Timer total_timer;
  total_timer.start();
  bool add_inverse = false;

  // get the current executable directory. 
  std::string exe_path;
  char str[512];
  int sz = sizeof(str);
#ifdef WIN32
#include <Winbase.h>
  GetModuleFileName(NULL,str,sz);
#endif
#ifdef LINUX
#include <unistd.h>
  readlink("/proc/self/exe",str,sz);
#endif
#ifdef DARWIN
#include <mach-o/dyld.h>
  _NSGetExecutablePath(str,&sz);
#endif
  exe_path = std::string(str);
  exe_path = exe_path.substr(0,exe_path.find_last_of("/"));

  if(material_fields.empty()){
    std::cerr << "No material fields or segmentation files provided. Terminating." 
      << std::endl;
    return 10;
  }
  else if(material_fields.size() == 1) {
    if(USE_BIOMESH_SEGMENTATION) {
      std::system((exe_path + "/unu minmax " +
            material_fields.at(0) + " > tmp").c_str());
      std::ifstream in("tmp");
      int first, second;
      std::string tmp;
      in >> tmp >> first >> tmp >> second;
      in.close();
      std::ifstream nrrd(material_fields.at(0).c_str());
      nrrd >> tmp;
      nrrd.close();
      if (tmp == "NRRD0001" || (second - first) == 0)
        add_inverse = true;
      else if (tmp == "NRRD0004") {
        int total_mats = second - first + 1;
        std::cout << "This is a segmentation file. " << std::endl;
        //create an output folder by this file for the new fields.
        std::string output_dir = material_fields[0].substr(0,
            material_fields[0].find_last_of("/")) + "/material_fields" ;
        //std::system(("mkdir " + output_dir).c_str());
        //create the python file that the scripts need to create the fields.
        //copy template file
        std::system(("cat " + exe_path + "/ConfigTemplate.py > " +
              exe_path + "/ConfigUse.py ").c_str());
        //add "mats" "mat_names" "model_output_path" "model_input_file"
        std::ofstream out((exe_path +
              "/ConfigUse.py").c_str(),std::ios::app);
        out << "model_input_file=\"" << material_fields[0] << "\"" << std::endl;
        out << "model_output_path=\"" << output_dir << "\"" << std::endl;
        out << "mats = (";
        for(int i = 0; i < total_mats; i++)
          out << i << ((i+1)==total_mats?")\n":", ");
        out << "mat_names = (";
        for(int i = 0; i < total_mats; i++)
          out << "'" << ((char)('a' + i)) << "'" << ((i+1)==total_mats?")\n":", ");
        out.close();
        //call the python script to make the fields
        std::string cmmd = "python " + exe_path +
          "/BuildMesh.py -s1:2 --binary-path " + exe_path
          + " " + exe_path + "/ConfigUse.py";
        std::cout << "Calling BuildMesh.py: " << cmmd << std::endl;
        std::system(cmmd.c_str());
        //delete all of the unneeded files.
        std::system(("rm " + output_dir + "/*.log").c_str());
        std::system(("rm " + output_dir + "/*.txt").c_str());
        std::system(("rm " + output_dir + "/*.fld").c_str());
        std::system(("rm " + output_dir + "/*.raw").c_str());
        std::system(("rm " + output_dir + "/*.tight.nrrd").c_str());
        std::system(("rm " + output_dir + "/*corrected.nrrd").c_str());
        std::system(("rm " + output_dir + "/*solo.nrrd").c_str());
        std::system(("rm " + output_dir + "/*lut.nrrd").c_str());
        std::system(("rm " + output_dir + "/*unorient.nrrd").c_str());
        std::system(("rm " + output_dir + "/*.tf").c_str());
        std::system(("rm " + output_dir + "/*labelmap.nrrd").c_str());
        material_fields.clear();
        for(int i = 0; i < total_mats; i++)
          material_fields.push_back(output_dir + std::string("/")
              + ((char)('a' + i)) +
              std::string(".tight_transformed.nrrd"));

      }
      else {
        std::cerr << "Cleaver cannot mesh this volume file." << std::endl;
        return 1;
      }
    } else {
      add_inverse = true;
    }
  }

  if(verbose) {
    std::cout << " Loading input fields:" << std::endl;
    for (size_t i=0; i < material_fields.size(); i++) {
      std::cout << " - " << material_fields[i] << std::endl;
    }
  }

  std::vector<cleaver::AbstractScalarField*> fields =
    loadNRRDFiles(material_fields, verbose);
  if (fields.empty()) {
    std::cerr << "Failed to load image data. Terminating." << std::endl;
    return 10;
  } else if (add_inverse)
    fields.push_back(new cleaver::InverseScalarField(fields[0]));

  cleaver::Volume *volume = new cleaver::Volume(fields);
  cleaver::CleaverMesher mesher(volume);
  mesher.setAlphaInit(alpha);

  //-----------------------------------
  // Load background mesh if provided
  //-----------------------------------
  cleaver::TetMesh *bgMesh = NULL;
  if(have_background_mesh) {
    std::string nodeFileName = background_mesh + ".node";
    std::string eleFileName = background_mesh + ".ele";
    if (verbose) {
      std::cout << "Loading background mesh: \n\t" << nodeFileName
        << "\n\t" << eleFileName << std::endl;
    }
    bgMesh =  cleaver::TetMesh::createFromNodeElePair(nodeFileName, eleFileName);
    mesher.setBackgroundMesh(bgMesh);
  }
  //-----------------------------------
  // otherwise take steps to compute one
  //-----------------------------------
  else {

    //------------------------------------------------------------
    // Load or Construct Sizing Field
    //------------------------------------------------------------
    cleaver::AbstractScalarField *sizingField = NULL;
    if (have_sizing_field) {
      std::cout << "Loading sizing field: " << sizing_field << std::endl;
      sizingField = loadNRRDFile(sizing_field, verbose);
      // todo(jon): add error handling
    }
    else {
      cleaver::Timer sizing_field_timer;
      sizing_field_timer.start();
      sizingField = cleaver::SizingFieldCreator::createSizingFieldFromVolume(
          volume,
          (float)(1.0/lipschitz),
          (float)scale,
          (float)multiplier,
          (int)padding,
          (mesh_mode==cleaver::Regular?false:true),
          verbose);
      sizing_field_timer.stop();
      sizing_field_time = sizing_field_timer.time();
    }

    //------------------------------------------------------------
    // Set Sizing Field on Volume
    //------------------------------------------------------------
    volume->setSizingField(sizingField);


    //-----------------------------------------------------------
    // Construct Background Mesh
    //-----------------------------------------------------------
    cleaver::Timer background_timer;
    background_timer.start();
    if(verbose)
      std::cout << "Creating Octree Mesh..." << std::endl;
    switch(mesh_mode) {

    case cleaver::Regular:
      mesher.setAlphas(alpha_long,alpha_short);
      mesher.setRegular(true);
      bgMesh = mesher.createBackgroundMesh(verbose);
      break;
    default:
    case cleaver::Structured:
      mesher.setRegular(false);
      bgMesh = mesher.createBackgroundMesh(verbose);
      break;
    }
    background_timer.stop();
    background_time = background_timer.time();
    mesher.setBackgroundTime(background_time);
  }


  //-----------------------------------------------------------
  // Write Background Mesh if requested
  //-----------------------------------------------------------
  if (bgMesh && write_background_mesh) {
    bgMesh->writeNodeEle(output_path + "bgmesh", false, false, false);
  }

  //-----------------------------------------------------------
  // Apply Mesh Cleaving
  //-----------------------------------------------------------
  cleaver::Timer cleaving_timer;
  cleaving_timer.start();
  mesher.buildAdjacency(verbose);
  mesher.sampleVolume(verbose);
  mesher.computeAlphas(verbose);
  mesher.computeInterfaces(verbose);
  mesher.generalizeTets(verbose);
  mesher.snapsAndWarp(verbose);
  mesher.stencilTets(verbose);
  cleaving_timer.stop();
  cleaving_time = cleaving_timer.time();

  cleaver::TetMesh *mesh = mesher.getTetMesh();

  //-----------------------------------------------------------
  // Strip Exterior Tets
  //-----------------------------------------------------------
  if(strip_exterior){
    cleaver::stripExteriorTets(mesh, volume, verbose);
  }

  //-----------------------------------------------------------
  // Compute Quality If Havn't Already
  //-----------------------------------------------------------
  mesh->computeAngles();
  //-----------------------------------------------------------
  // Fix jacobians if requested.
  //-----------------------------------------------------------
  if (fix_tets) mesh->fixVertexWindup(verbose);

  //-----------------------------------------------------------
  // Write Mesh To File
  //-----------------------------------------------------------
  mesh->writeMesh(output_path + output_name, output_format, verbose);
  mesh->writeInfo(output_path + output_name, verbose);
  //-----------------------------------------------------------
  // Write Experiment Info to file
  //-----------------------------------------------------------
  total_timer.stop();
  double total_time = total_timer.time();

  if(verbose) {
    std::cout << "Output Info" << std::endl;
    std::cout << "Size: " << volume->size().toString() << std::endl;
    std::cout << "Materials: " << volume->numberOfMaterials() << std::endl;
    std::cout << "Min Dihedral: " << mesh->min_angle << std::endl;
    std::cout << "Max Dihedral: " << mesh->max_angle << std::endl;
    std::cout << "Total Time: " << total_time << " seconds" << std::endl;
    std::cout << "Sizing Field Time: " << sizing_field_time << " seconds" << std::endl;
    std::cout << "Backound Mesh Time: " << background_time << " seconds" << std::endl;
    std::cout << "Cleaving Time: " << cleaving_time << " seconds" << std::endl;
  }
  return 0;
}
