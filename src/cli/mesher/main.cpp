

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
#include <cleaver/Cleaver.h>
#include <cleaver/CleaverMesher.h>
#include <cleaver/InverseField.h>
#include <cleaver/SizingFieldCreator.h>
#include <cleaver/Timer.h>
#include <NRRDTools.h>

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
const std::string vtkPoly = "vtkPoly";
const std::string vtkUSG = "vtkUSG";
const std::string ply = "ply";

const std::string kDefaultOutputPath = "./";
const std::string kDefaultOutputName = "output";
const cleaver::MeshFormat kDefaultOutputFormat = cleaver::Tetgen;


const double kDefaultAlpha = 0.4;
const double kDefaultAlphaLong = 0.357;
const double kDefaultAlphaShort = 0.203;
const double kDefaultScale = 1.0;
const double kDefaultLipschitz = 0.2;
const double kDefaultMultiplier = 1.0;
const int    kDefaultPadding = 0;
const int    kDefaultMaxIterations = 1000;
const double kDefaultSigma = 1.;

namespace po = boost::program_options;

// Entry Point
int main(int argc, char* argv[])
{
  bool verbose = false;
  bool fix_tets = false;
  bool segmentation = false;
  bool simple = false;
  std::vector<std::string> material_fields;
  std::string sizing_field;
  std::string background_mesh;
  std::string output_path = kDefaultOutputPath;
  std::string output_name = kDefaultOutputName;
  double alpha = kDefaultAlpha;
  double alpha_long = kDefaultAlphaLong;
  double alpha_short = kDefaultAlphaShort;
  double lipschitz = kDefaultLipschitz;
  double multiplier = kDefaultMultiplier;
  double scaling = kDefaultScale;
  int padding = kDefaultPadding;
  bool have_sizing_field = false;
  bool have_background_mesh = false;
  bool write_background_mesh = false;
  bool record_operations = false;
  std::string recording_input;
  bool strict = false;
  bool strip_exterior = false;
  enum cleaver::MeshType mesh_mode = cleaver::Structured;
  cleaver::MeshFormat output_format = kDefaultOutputFormat;
  double sigma = kDefaultSigma;

  double sizing_field_time = 0;
  double   background_time = 0;
  double     cleaving_time = 0;


  //-------------------------------
  //  Parse Command Line Params
  //-------------------------------
  try {
    po::options_description description("Command line flags");
    description.add_options()
      ("alpha,a", po::value<double>(), "initial alpha value")
      ("alpha_short,s", po::value<double>(), "alpha short value for regular mesh_mode")
      ("alpha_long,l", po::value<double>(), "alpha long value for regular mesh_mode")
      ("background_mesh,b", po::value<std::string>(), "input background mesh")
      ("blend_sigma,B", po::value<double>(), "blending sigma for input(s) to remove alias artifacts.")
      ("fix_tet_windup,j", "Ensure positive Jacobians with proper vertex wind-up.")
      ("grading,g", po::value<double>(), "sizing field grading")
      ("help,h", "display help message")
      ("input_files,i", po::value<std::vector<std::string> >()->multitoken(), "material field paths or segmentation path")
      ("mesh_mode,m", po::value<std::string>(), "background mesh mode (structured [default], regular)")
      ("multiplier,x", po::value<double>(), "sizing field multiplier")
      ("output_path,o", po::value<std::string>(), "output path prefix")
      ("output_name,n", po::value<std::string>(), "output mesh name [default 'output']")
      ("output_format,f", po::value<std::string>(), "output mesh format (tetgen [default], scirun, matlab, vtkUSG, vtkPoly, ply [Surface mesh only])")
      ("padding,p", po::value<int>(), "volume padding")
      ("record,r", po::value<std::string>(), "record operations on tets from input file.")
      ("scale,c", po::value<double>(), "sizing field scale factor")
      ("segmentation,S", "The input file is a segmentation file.")
      ("simple", "Use simple interface approximation.")
      ("sizing_field,z", po::value<std::string>(), "sizing field path")
      ("strict,t", "warnings become errors")
      ("strip_exterior,e", "strip exterior tetrahedra")
      ("write_background_mesh,w", "write background mesh")
      ("verbose,v", "enable verbose output")
      ("version,V", "display version information");

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

    // enable simple interfaces
    if (variables_map.count("simple")) {
      simple = true;
    }

    // enable segmentation
    if (variables_map.count("segmentation")) {
      segmentation = true;
    }
    if (variables_map.count("strict")) {
      strict = true;
    }

    // parse the material field input file names
    if (variables_map.count("input_files")) {
      material_fields = variables_map["input_files"].as<std::vector<std::string> >();
    } else {
      std::cerr << "Error: At least one material field file must be specified." << std::endl;
      return 1;
    }

    //-----------------------------------------
    // parse the sizing field input file name
    // and check for conflicting parameters
    //----------------------------------------
    if (variables_map.count("sizing_field")) {
      have_sizing_field = true;
      sizing_field = variables_map["sizing_field"].as<std::string>();

      if (variables_map.count("grading")) {
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
    if (variables_map.count("scale")) {
      scaling = variables_map["scale"].as<double>();
    }
    if (variables_map.count("multiplier")) {
      multiplier = variables_map["multiplier"].as<double>();
    }
    if (variables_map.count("padding")) {
      padding = variables_map["padding"].as<int>();
    }
    fix_tets = variables_map.count("fix_tet_windup") == 0 ? false : true;

    if (variables_map.count("alpha")) {
      alpha = variables_map["alpha"].as<double>();
    }
    if (variables_map.count("alpha_short")) {
      alpha_short = variables_map["alpha_short"].as<double>();
    }
    if (variables_map.count("alpha_long")) {
      alpha_long = variables_map["alpha_long"].as<double>();
    }
    if (variables_map.count("blend_sigma")) {
      sigma = variables_map["blend_sigma"].as<double>();
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
      if (mesh_mode_string.compare("regular") == 0) {
        mesh_mode = cleaver::Regular;
      } else if (mesh_mode_string.compare("structured") == 0) {
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

    if (variables_map.count("record")) {
      record_operations = true;
      recording_input = variables_map["record"].as<std::string>();
    }

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
      if (format_string.compare(tetgen) == 0) {
        output_format = cleaver::Tetgen;
      } else if (format_string.compare(scirun) == 0) {
        output_format = cleaver::Scirun;
      } else if (format_string.compare(matlab) == 0) {
        output_format = cleaver::Matlab;
      } else if (format_string.compare(vtkPoly) == 0) {
        output_format = cleaver::VtkPoly;
      } else if (format_string.compare(vtkUSG) == 0) {
        output_format = cleaver::VtkUSG;
      } else if (format_string.compare(ply) == 0) {
        output_format = cleaver::PLY;
      } else {
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

  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 8;
  } catch (...) {
    std::cerr << "Unhandled exception caught. Terminating." << std::endl;
    return 9;
  }

  //-----------------------------------
  //  Load Data & Construct Volume
  //-----------------------------------
  cleaver::Timer total_timer;
  total_timer.start();
  bool add_inverse = false;
  std::vector<cleaver::AbstractScalarField*> fields;
  if (material_fields.empty()) {
    std::cerr << "No material fields or segmentation files provided. Terminating."
      << std::endl;
    return 10;
  }
  if (segmentation && material_fields.size() == 1) {
    fields = NRRDTools::segmentationToIndicatorFunctions(material_fields[0], sigma);
  } else {
    if (segmentation && material_fields.size() > 1) {
      std::cerr << "Warning: More than 1 input provided for segmentation." << std::endl
                << "This will be assumed to be indicator functions." << std::endl;
    }
    if (material_fields.size() == 1) {
      add_inverse = true;
    }
    if (verbose) {
      std::cout << " Loading input fields:" << std::endl;
      for (size_t i = 0; i < material_fields.size(); i++) {
        std::cout << " - " << material_fields[i] << std::endl;
      }
    }
    fields = NRRDTools::loadNRRDFiles(material_fields,sigma);
    if (fields.empty()) {
      std::cerr << "Failed to load image data. Terminating." << std::endl;
      return 10;
    } else if (add_inverse)
      fields.push_back(new cleaver::InverseScalarField(fields[0]));
      fields.back()->setName(fields[0]->name() + "-inverse");
  }

  //Error checking for indicator function values.
  for (int i = 0; i < fields.size(); i++)
  {
    //Skip if the segmentation value is 0 (background) or it is an inverse file
    std::size_t found = fields[i]->name().find("inverse");
    if ((segmentation && i == 0) || (found != std::string::npos))
    {
      continue;
    }
    //Check for critical errors
    auto error = ((cleaver::ScalarField<float>*)fields[i])->getError();
    if (error.compare("nan") == 0 || error.compare("maxmin") == 0)
    {
      std::cerr << "Nrrd file read error: No zero crossing in indicator function. Not a valid file or need a lower sigma value." << std::endl;
      return 11;
    }
    //Check for warning
    auto warning = ((cleaver::ScalarField<float>*)fields[i])->getWarning();
    if (warning)
    {
      std::cerr << "Nrrd file read WARNING: Sigma is 10% of volume's size. Gaussian kernel may be truncated." << std::endl;
    }
  }

  cleaver::Volume *volume = new cleaver::Volume(fields);
  cleaver::CleaverMesher mesher(simple);
  mesher.setVolume(volume);
  mesher.setAlphaInit(alpha);


  // Maybe enable recording on debug dump tets.
  if (record_operations) {
    mesher.recordOperations(recording_input);
  }

  //-----------------------------------
  // Load background mesh if provided
  //-----------------------------------
  cleaver::TetMesh *bgMesh = nullptr;
  if (have_background_mesh) {
    std::string nodeFileName = background_mesh + ".node";
    std::string eleFileName = background_mesh + ".ele";
    if (verbose) {
      std::cout << "Loading background mesh: \n\t" << nodeFileName
        << "\n\t" << eleFileName << std::endl;
    }
    bgMesh = cleaver::TetMesh::createFromNodeElePair(nodeFileName, eleFileName);
    mesher.setBackgroundMesh(bgMesh);
  }
  //-----------------------------------
  // otherwise take steps to compute one
  //-----------------------------------
  else {

    //------------------------------------------------------------
    // Load or Construct Sizing Field
    //------------------------------------------------------------
    std::vector<cleaver::AbstractScalarField *> sizingField;
    if (have_sizing_field) {
      std::cout << "Loading sizing field: " << sizing_field << std::endl;
      std::vector<std::string> tmp(1,sizing_field);
      sizingField = NRRDTools::loadNRRDFiles(tmp);
      // todo(jon): add error handling
    } else {
      cleaver::Timer sizing_field_timer;
      sizing_field_timer.start();
      sizingField.push_back(cleaver::SizingFieldCreator::createSizingFieldFromVolume(
        volume,
        (float)(1.0 / lipschitz),
        (float)scaling,
        (float)multiplier,
        (int)padding,
        (mesh_mode == cleaver::Regular ? false : true),
        verbose));
      sizing_field_timer.stop();
      sizing_field_time = sizing_field_timer.time();
    }

    //------------------------------------------------------------
    // Set Sizing Field on Volume
    //------------------------------------------------------------
    volume->setSizingField(sizingField[0]);


    //-----------------------------------------------------------
    // Construct Background Mesh
    //-----------------------------------------------------------
    cleaver::Timer background_timer;
    background_timer.start();
    if (verbose)
      std::cout << "Creating Octree Mesh..." << std::endl;
    switch (mesh_mode) {

    case cleaver::Regular:
      mesher.setAlphas(alpha_long, alpha_short);
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
    bgMesh->writeNodeEle(output_path + "bgmesh", verbose, false, false);
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
  if (strip_exterior) {
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

  if (verbose) {
    std::cout << "Output Info" << std::endl;
    std::cout << "Size: " << volume->size().toString() << std::endl;
    std::cout << "Materials: " << volume->numberOfMaterials() << std::endl;
    std::cout << "Min Dihedral: " << mesh->min_angle << std::endl;
    std::cout << "Max Dihedral: " << mesh->max_angle << std::endl;
    std::cout << "Total Time: " << total_time << " seconds" << std::endl;
    std::cout << "Sizing Field Time: " << sizing_field_time << " seconds" << std::endl;
    std::cout << "Background Mesh Time: " << background_time << " seconds" << std::endl;
    std::cout << "Cleaving Time: " << cleaving_time << " seconds" << std::endl;
  }
  return 0;
}
