

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

const std::string kDefaultOutputPath   = "./";
const std::string kDefaultOutputName   = "output";
const cleaver::MeshFormat kDefaultOutputFormat = cleaver::Tetgen;


const double kDefaultAlpha = 0.4f;
const double kDefaultScale = 2.0;
const double kDefaultLipschitz = 0.2;
const double kDefaultMultiplier = 1.0;

const std::string kDefaultScaleString = "2.0";
const std::string kDefaultLipschitzString = "0.2";
const std::string kDefaultMultiplierString = "1.0";
const int         kDefaultPadding = 0;
const int         kDefaultMaxIterations = 1000;

namespace po = boost::program_options;

// Entry Point
int main(int argc,	char* argv[])
{  
    bool verbose = false;
    std::vector<std::string> material_fields;
    std::string sizing_field;
    std::string background_mesh;
    std::string output_path = kDefaultOutputPath;
    std::string output_name = kDefaultOutputName;
    double alpha = kDefaultAlpha;
    double scale = kDefaultScale;
    double lipschitz = kDefaultLipschitz;
    double multiplier = kDefaultMultiplier;
    int padding = kDefaultPadding;
    bool have_sizing_field = false;
    bool have_background_mesh = false;
    bool write_background_mesh = false;
    bool improve_mesh = false;
    bool strict = false;
    bool accelerate = false;
    bool strip_exterior = false;
    enum cleaver::MeshType mesh_mode = cleaver::Unstructured;
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
                ("material_fields", po::value<std::vector<std::string> >()->multitoken(), "material field paths")
                ("background_mesh", po::value<std::string>(), "input background mesh")
                ("mesh_mode", po::value<std::string>(), "background mesh mode")
                ("mesh_improve", "improve background quality")
                ("alpha", po::value<double>(), "initial alpha value")
                ("sizing_field", po::value<std::string>(), "sizing field path")
                ("grading", po::value<double>(), "sizing field grading")
                ("multiplier", po::value<double>(), "sizing field multiplier")
                ("scale", po::value<double>(), "sizing field scale")
                ("padding", po::value<int>(), "volume padding")
                ("accelerate", "use acceleration structure")
                ("write_background_mesh", "write background mesh")
                ("strip_exterior", "strip exterior tetrahedra")
                ("output_path", po::value<std::string>(), "output path prefix")
                ("output_name", po::value<std::string>(), "output mesh name")
                ("output_format", po::value<std::string>(), "output mesh format")
                ("strict", "warnings become errors")
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

        if (variables_map.count("accelerate")) {
            accelerate = true;
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
                    return 0;
                }
            }
            if (variables_map.count("multiplier")) {
                if (!strict)
                    std::cerr << "Warning: sizing field provided, multiplier will be ignored." << std::endl;
                else {
                    std::cerr << "Error: both sizing field and multiplier parameter provided." << std::endl;
                    return 0;
                }
            }
            if (variables_map.count("scale")) {
                if (!strict)
                    std::cerr << "Warning: sizing field provided, scale will be ignored." << std::endl;
                else {
                    std::cerr << "Error: both sizing field and scale parameter provided." << std::endl;
                    return 0;
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


        if (variables_map.count("alpha")) {
            alpha = variables_map["alpha"].as<double>();
        }

        if (variables_map.count("background_mesh")) {
            have_background_mesh = true;
            background_mesh = variables_map["background_mesh"].as<std::string>();

            if (variables_map.count("sizing_field")) {
                if (!strict)
                    std::cerr << "Warning: background mesh provided, sizing field will be ignored." << std::endl;
                else {
                    std::cerr << "Error: both background mesh and sizing field provided." << std::endl;
                    return 0;
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
                return 0;
            }
        }

        // does user want background mesh improvement?
        if (variables_map.count("mesh_improve")) {
            improve_mesh = true;
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
            if(format_string.compare(tetgen) == 0){
                output_format = cleaver::Tetgen;
            }
            else if(format_string.compare(scirun) == 0){
                output_format = cleaver::Scirun;
            }
            else if(format_string.compare(matlab) == 0){
                output_format = cleaver::Matlab;
            }
            else{
                std::cerr << "Error: unsupported output format: " << format_string << std::endl;
                return 0;
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
        return 0;
    }
    catch(...) {
        std::cerr << "Unhandled exception caught. Terminating." << std::endl;
        return 0;
    }

    //-----------------------------------
    //  Load Data & Construct Volume
    //-----------------------------------
    cleaver::Timer total_timer;
    total_timer.start();
    if(verbose) {
        std::cout << " Loading input fields:" << std::endl;
        for (size_t i=0; i < material_fields.size(); i++) {
            std::cout << " - " << material_fields[i] << std::endl;
        }
    }

    std::vector<cleaver::AbstractScalarField*> fields = loadNRRDFiles(material_fields, verbose);
    if(fields.empty()){
        std::cerr << "Failed to load image data. Terminating." << std::endl;
        return 0;
    }
    else if(fields.size() == 1) {
        fields.push_back(new cleaver::InverseScalarField(fields[0]));
    }

    cleaver::Volume *volume = new cleaver::Volume(fields);
    cleaver::CleaverMesher mesher(volume);
		mesher.setAlphaInit(alpha);

    //-----------------------------------
    // Load background mesh if provided
    //-----------------------------------
    cleaver::TetMesh *bgMesh;
    if(have_background_mesh) {
        std::string nodeFileName = background_mesh + ".node";
        std::string eleFileName = background_mesh + ".ele";
        bgMesh =  cleaver::TetMesh::createFromNodeElePair(nodeFileName, eleFileName);
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
                        true,
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
        switch(mesh_mode) {

            case cleaver::Regular:
            {
                std::cerr << "Error: Regular background mesh not yet supported." << std::endl;
                return 0;
            }
            default:
            case cleaver::Structured:
            {
                if(verbose)
                    std::cout << "Creating Octree Mesh..." << std::endl;
                //bgMesh = cleaver::createBackgroundMesh(volume, cleaver::Structured);
                mesher.createBackgroundMesh();
                break;
            }
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
    mesher.buildAdjacency();
    mesher.sampleVolume();
    mesher.computeAlphas();
    mesher.computeInterfaces();
    mesher.generalizeTets();
    mesher.snapsAndWarp();
    mesher.stencilTets();
    cleaving_timer.stop();
    cleaving_time = cleaving_timer.time();

    cleaver::TetMesh *mesh = mesher.getTetMesh();


    //-----------------------------------------------------------
    // Strip Exterior Tets
    //-----------------------------------------------------------
    if(strip_exterior){
        cleaver::stripExteriorTets(mesh, volume);   
    }

    //-----------------------------------------------------------
    // Compute Quality If Havn't Already
    //-----------------------------------------------------------
    mesh->computeAngles();

    //-----------------------------------------------------------
    // Write Mesh To File
    //-----------------------------------------------------------    
    mesh->writeNodeEle(output_path + output_name, verbose, true, false);
    mesh->writePly(output_path + output_name, verbose);
    mesh->writeInfo(output_path + output_name, verbose);

    //-----------------------------------------------------------
    // Write Experiment Info to file
    //-----------------------------------------------------------
    total_timer.stop();
    double total_time = total_timer.time();

    if(verbose)
        std::cout << "Writing experiment file: " << output_path << "experiment.info" << std::endl;
    std::string infoFilename = output_path + "experiment.info";
    std::ofstream file(infoFilename.c_str());
    file << "Experiment Info" << std::endl;
    file << "Size: " << volume->size().toString() << std::endl;
    file << "Materials: " << volume->numberOfMaterials() << std::endl;
    file << "Min Dihedral: " << mesh->min_angle << std::endl;
    file << "Max Dihedral: " << mesh->max_angle << std::endl;
    file << "Total Time: " << total_time << " seconds" << std::endl;
    file << "Sizing Field Time: " << sizing_field_time << " seconds" << std::endl;
    file << "Backound Mesh Time: " << background_time << " seconds" << std::endl;
    file << "Cleaving Time: " << cleaving_time << " seconds" << std::endl;
    file.close();

    return 0;
}
