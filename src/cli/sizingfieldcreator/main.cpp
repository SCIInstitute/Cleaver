

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
#include <cleaver/InverseField.h>
#include <cleaver/SizingFieldCreator.h>
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

const std::string kDefaultOutputName   = "sizingfield";

const double kDefaultSamplingRate = 2.0;
const double kDefaultLipschitz = 0.2;
const double kDefaultFeatureScaling = 1.0;
const int    kDefaultPadding = 0;

const std::string kDefaultSamplingRateString = "2.0";
const std::string kDefaultLipschitzString = "0.2";
const std::string kDefaultFeatureScalingString = "1.0";

// Entry Point
int main(int argc,	char* argv[])
{
    bool verbose = false;
    std::vector<std::string> material_fields;
    std::string output_path = kDefaultOutputName;
    double samplingRate      = kDefaultSamplingRate;
    double lipschitz  = kDefaultLipschitz;
    double featureScaling = kDefaultFeatureScaling;
    int    padding    = kDefaultPadding;

    //-------------------------------
    //  Parse Command Line Params
    //-------------------------------
    try{
        bool show_version = false;

        CLI::App app{ "Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library - sizing field creator" };
        app.add_flag("-v,--verbose", verbose, "enable verbose output");
        app.add_flag("--version", show_version, "display version information");
        app.add_option("--material_fields", material_fields, "material field paths")->required();
        app.add_option("--lipschitz", lipschitz, "maximum rate of change of element size (1 is uniform)");
        app.add_option("--feature_scaling", featureScaling, "feature size scaling (higher values make a coarser mesh)");
        app.add_option("--sampling_rate", samplingRate, "volume sampling rate (lower values make a coarser mesh)");
        app.add_option("--output", output_path, "output path");
        app.add_option("--padding", padding, "volume padding");
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
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;
    }
    catch(...) {
        std::cerr << "Unhandled exception caught. Terminating." << std::endl;
        return 0;
    }

    //-----------------------------------
    //  Load Data & Construct  Volume
    //-----------------------------------
    std::cout << " Loading input fields:" << std::endl;
    for (size_t i=0; i < material_fields.size(); i++) {
        std::cout << " - " << material_fields[i] << std::endl;
    }

    std::vector<cleaver::AbstractScalarField*> fields = NRRDTools::loadNRRDFiles(material_fields);//does this need a sigma?
    if(fields.empty()){
        std::cerr << "Failed to load image data. Terminating." << std::endl;
        return 0;
    }
    else if(fields.size() == 1) {
        fields.push_back(new cleaver::InverseScalarField(fields[0]));
    }

    cleaver::Volume *volume = new cleaver::Volume(fields);

    //------------------------------------------------------------
    // Construct Sizing Field
    //------------------------------------------------------------
    cleaver::FloatField *sizingField =
            cleaver::SizingFieldCreator::createSizingFieldFromVolume(
                volume,
                (float)(1.0/lipschitz),
                (float)samplingRate,
                (float)featureScaling,
                (int)padding,
                false);

    //------------------------------------------------------------
    // Write Field to File
    //------------------------------------------------------------
    std::cout << " Writing sizing field to file: " << output_path << ".nrrd" << std::endl;  // todo(jrb) strip path
    NRRDTools::saveNRRDFile(sizingField, output_path);


    // done
    std::cout << " Done." << std::endl;
    return 0;
}
