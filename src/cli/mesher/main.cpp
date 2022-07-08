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

#include <CLI11.hpp>

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
const double kDefaultSamplingRate = 1.0;
const double kDefaultLipschitz = 0.2;
const double kDefaultFeatureScaling = 1.0;
const int    kDefaultPadding = 0;
const int    kDefaultMaxIterations = 1000;
const double kDefaultSigma = 1.;

// Entry Point
int main(int argc, char* argv[])
{
  bool verbose = false;
  bool fix_tets = false;
  bool segmentation = true;
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
  double feature_scaling = kDefaultFeatureScaling;
  double sampling_rate = kDefaultSamplingRate;
  int padding = kDefaultPadding;
  bool have_sizing_field = false;
  bool have_background_mesh = false;
  bool write_background_mesh = false;
  bool record_operations = false;
  std::string recording_input;
  bool strict = false;
  bool strip_exterior = false;
  enum cleaver::MeshType element_sizing_method = cleaver::Adaptive;
  cleaver::MeshFormat output_format = kDefaultOutputFormat;
  double sigma = kDefaultSigma;

  double sizing_field_time = 0;
  double   background_time = 0;
  double     cleaving_time = 0;


  //-------------------------------
  //  Parse Command Line Params
  //-------------------------------
  try {
    std::string element_sizing_method_string;
    bool show_help = false;
    std::string format_string;
    bool indicator_functions = false;
    bool show_version = false;

    CLI::App app{ "Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library - mesher" };
    //po::options_description description("Command line flags");
    app.add_option("-a,--alpha", alpha, "initial alpha value");
    app.add_option("-s,--alpha_short", alpha_short, "alpha short value for constant element sizing method");
    app.add_option("-l,--alpha_long", alpha_long, "alpha long value for constant element sizing method");
    app.add_option("-b,--background_mesh", background_mesh, "input background mesh");
    app.add_option("-B,--blend_sigma", sigma, "blending function sigma for input(s) to remove alias artifacts");
    app.add_option("-m,--element_sizing_method", element_sizing_method_string, "background mesh mode (adaptive [default], constant)");
    app.add_option("-F,--feature_scaling", feature_scaling, "feature size scaling (higher values make a coarser mesh)");
    app.add_flag("-j,--fix_tet_windup", fix_tets, "ensure positive Jacobians with proper vertex wind-up");
    //app.add_option("-h,--help", show_help, "display help message");
    app.add_option("-i,--input_files", material_fields, "material field paths or segmentation path");
    app.add_option("-L,--lipschitz", lipschitz, "maximum rate of change of element size (1 is uniform)");
    app.add_option("-f,--output_format", format_string, "output mesh format (tetgen [default], scirun, matlab, vtkUSG, vtkPoly, ply [surface mesh only])");
    app.add_option("-n,--output_name", output_name, "output mesh name (default 'output')");
    app.add_option("-o,--output_path", output_path, "output path prefix");
    //app.add_option("-p,--padding", padding, "volume padding");
    app.add_option("-r,--record", recording_input, "record operations on tets from input file");
    app.add_option("-R,--sampling_rate", sampling_rate, "volume sampling rate (lower values make a coarser mesh)");
    app.add_flag("-I,--indicator_functions", indicator_functions, "the input files are indicator functions");
    app.add_flag("--simple", simple, "use simple interface approximation");
    app.add_option("-z,--sizing_field", sizing_field, "sizing field path");
    app.add_flag("-t,--strict", strict, "warnings become errors");
    app.add_flag("-e,--strip_exterior", strip_exterior, "strip exterior tetrahedra");
    app.add_flag("-w,--write_background_mesh", write_background_mesh, "write background mesh");
    app.add_flag("-v,--verbose", verbose, "enable verbose output");
    app.add_flag("-V,--version", show_version, "display version information");

    CLI11_PARSE(app, argc, argv);

    // print help
    if (argc == 1) {
      std::cout << app.help() << std::endl;
      return 0;
    }

    // print version info
    if (show_version) {
      std::cout << cleaver::Version << std::endl;
      return 0;
    }

    // enable indicator_function
    if (indicator_functions) {
      segmentation = false;
    }

    // parse the material field input file names
    if (material_fields.empty()) {
      std::cerr << "Error: At least one material field file must be specified." << std::endl;
      return 1;
    }

    //-----------------------------------------
    // parse the sizing field input file name
    // and check for conflicting parameters
    //----------------------------------------
    if (!sizing_field.empty()) {
      have_sizing_field = true;

      if (app.count("--lipschitz")) {
        if (!strict)
          std::cerr << "Warning: sizing field provided, lipschitz will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and lipschitz parameter provided." << std::endl;
          return 2;
        }
      }
      if (app.count("--feature_scaling")) {
        if (!strict)
          std::cerr << "Warning: sizing field provided, feature scaling will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and feature scaling parameter provided." << std::endl;
          return 3;
        }
      }
      if (app.count("--sampling_rate")) {
        if (!strict)
          std::cerr << "Warning: sizing field provided, sampling rate will be ignored." << std::endl;
        else {
          std::cerr << "Error: both sizing field and sampling rate parameter provided." << std::endl;
          return 4;
        }
      }
    }

    if (!background_mesh.empty()) {
      have_background_mesh = true;

      if (app.count("--sizing_field")) {
        if (!strict)
          std::cerr << "Warning: background mesh provided, sizing field will be ignored." << std::endl;
        else {
          std::cerr << "Error: both background mesh and sizing field provided." << std::endl;
          return 5;
        }
      }
    }


    // parse the background mesh mode
    if (!element_sizing_method_string.empty()) {
      if (element_sizing_method_string.compare("constant") == 0) {
        element_sizing_method = cleaver::Constant;
      } else if (element_sizing_method_string.compare("adaptive") == 0) {
        element_sizing_method = cleaver::Adaptive;
      } else {
        std::cerr << "Error: invalid background element sizing method: " << element_sizing_method_string << std::endl;
        std::cerr << "Valid Methods: [constant] [adaptive] " << std::endl;
        return 6;
      }
    }

    if (!recording_input.empty()) {
      record_operations = true;
    }

    // set the proper output mesh format
    if (!format_string.empty()) {
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
        (float)sampling_rate,
        (float)feature_scaling,
        (int)padding,
        (element_sizing_method != cleaver::Constant),
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
    switch (element_sizing_method) {

    case cleaver::Constant:
      mesher.setAlphas(alpha_long, alpha_short);
      mesher.setConstant(true);
      bgMesh = mesher.createBackgroundMesh(verbose);
      break;
    default:
    case cleaver::Adaptive:
      mesher.setConstant(false);
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
