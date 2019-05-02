

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

#include <boost/program_options.hpp>

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

namespace po = boost::program_options;

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
        po::options_description description("Command line flags");
        description.add_options()
                ("help,h", "display help message")
                ("verbose,v", "enable verbose output")
                ("version", "display version information")
                ("material_fields", po::value<std::vector<std::string> >()->multitoken(), "material field paths")
                ("lipschitz", po::value<double>(&lipschitz)->default_value(kDefaultLipschitz, kDefaultLipschitzString), "sizing field grading")//fix description
                ("feature_scaling", po::value<double>(&featureScaling)->default_value(kDefaultFeatureScaling), "sizing field multiplier")//fix description
                ("sampling_rate", po::value<double>(&samplingRate)->default_value(kDefaultSamplingRate), "sizing field scale")//fix description
                ("output", po::value<std::string>()->default_value(kDefaultOutputName, "sizingfield"), "output path")
                ("padding", po::value<int>()->default_value(kDefaultPadding), "padding")
        ;

        boost::program_options::variables_map variables_map;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
        boost::program_options::notify(variables_map);

        // print version info
        if (variables_map.count("version")) {
            std::cout << cleaver::Version << std::endl;
            return 0;
        }
        // print help
        else if (variables_map.count("help") || (argc ==1)) {
            std::cout << description << std::endl;
            return 0;
        }

        // enable verbose mode
        if (variables_map.count("verbose")) {
            verbose = true;
        }

        // parse the material field input file names
        if (variables_map.count("material_fields")) {
            material_fields = variables_map["material_fields"].as<std::vector<std::string> >();
            int file_count = material_fields.size();
        }
        else{
            std::cout << "Error: At least one material field file must be specified." << std::endl;
            return 0;
        }

        // set output path
        if (variables_map.count("output")) {
            output_path = variables_map["output"].as<std::string>();
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
                (float)sampling_rate,
                (float)feature_scaling,
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
