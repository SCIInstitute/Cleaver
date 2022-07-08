

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//  - Mesh Info Program
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

// Entry Point
int main(int argc, char* argv[])
{
  bool verbose = false;
  std::string meshName;

  //-------------------------------
  //  Parse Command Line Params
  //-------------------------------
  try {
    CLI::App app{ "Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library - mesh information" };
    app.add_option("-i,--input", meshName, "input mesh")->required();
    app.add_flag("-v,--verbose", verbose, "enable verbose output");

    CLI11_PARSE(app, argc, argv);

    // print help
    if (argc == 1) {
      std::cout << app.help() << std::endl;
      return 0;
    }

  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 8;
  } catch (...) {
    std::cerr << "Unhandled exception caught. Terminating." << std::endl;
    return 9;
  }


  //-----------------------------------
  // Load the input mesh.
  //-----------------------------------
  cleaver::TetMesh *mesh = nullptr;
  std::string nodeFileName = meshName + ".node";
  std::string eleFileName = meshName + ".ele";
  if (verbose) {
    std::cout << "Loading mesh: \n\t" << nodeFileName
      << "\n\t" << eleFileName << std::endl;
  }
  mesh = cleaver::TetMesh::createFromNodeElePair(nodeFileName, eleFileName);

  //-----------------------------------
  // Compute Dihedrals
  //-----------------------------------
  if (verbose) {
    std::cout << "Computing dihedral angles..." << std::endl;
  }

  // TODO(jonbronson): Remove construction of bottom up incidences.
  // Debug dump information is currently created in the compute angles method.
  // This requires full bottom up incidences be built prior to angle
  // computation. Otherwise, any flat tets will trigger a segfault.
  mesh->constructFaces();
  mesh->constructBottomUpIncidences();
  mesh->computeAngles();
  std::cout << "Min Dihedral: " << mesh->min_angle << std::endl;
  std::cout << "Max Dihedral: " << mesh->max_angle << std::endl;

  return 0;
}
