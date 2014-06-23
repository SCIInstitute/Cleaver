// Utils Includes
#include "util.h"

// STL Includes
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <fstream>
#include <string>
#include <nrrd2cleaver/nrrd2cleaver.h>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

// Entry Point
int main(int argc,	char* argv[])
{
    bool verbose = false;
    int materials = 1;
    std::string output = "./output";
    std::vector<int> dimensions;


    //-------------------------------
    //  Parse Command Line Params
    //-------------------------------
    try{
        po::options_description description("Command line flags");
        description.add_options()
                ("help,h", "display help message")
                ("verbose,v", "enable verbose output")
                ("dimensions,d",  po::value<std::vector<int> >()->multitoken(), "domain dimensions")
                ("materials,m", po::value<int>(), "number of materials")
                ("output,o", po::value<std::string>(), "output path")
        ;

        boost::program_options::variables_map variables_map;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
        boost::program_options::notify(variables_map);

        // print help
        if (variables_map.count("help") || (argc == 1)) {
            std::cout << description << std::endl;
            return 0;
        }
        // enable verbose mode
        if (variables_map.count("verbose")) {
            verbose = true;
        }

        if(variables_map.count("dimensions")) {
            dimensions = variables_map["dimensions"].as<std::vector<int> >();
            if(dimensions.size() != 3) {
                std::cerr << "Error: Three dimensions required. Received " << dimensions.size() << std::endl;
                return 0;
            }
        }

        if(variables_map.count("materials")) {
            materials = variables_map["materials"].as<int>();
            if(materials < 1){
                std::cerr << "Error: Must have at least 1 material" << std::endl;
                return 0;
            }
        }

        if(variables_map.count("output")) {
            output = variables_map["output"].as<std::string>();
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


    // print user given parameters
    if(verbose) {
        std::cout << "Creating new data volume..." << std::endl;
        std::cout << std::left << std::setw(13) << "width: " <<  dimensions[0] << std::endl;
        std::cout << std::setw(13) << "height: " << dimensions[1] << std::endl;
        std::cout << std::setw(13) << "depth: "  << dimensions[2] << std::endl;
        std::cout << std::setw(13) << "materials: " << materials << std::endl;
        std::cout << std::setw(13) << "output path: " << output << std::endl;
    }

    //-----------------------------------------------------------
    // Parse Command-line Params
    //-----------------------------------------------------------
    InputParameters parameters; //= parseCommandline(argc, argv);
    parameters.dims = cleaver::vec3(dimensions[0], dimensions[1], dimensions[2]);
    parameters.m    = materials;
    parameters.path = output;


    //-----------------------------------------------------------
    // Construct Input Volume
    //-----------------------------------------------------------
    cleaver::Volume *volume = createInputVolume(&parameters);

    //------------------------------------------------------------
    // Write Material Fields to NRRD
    //------------------------------------------------------------
    std::stringstream ss;
    for(int i=0; i < volume->numberOfMaterials(); i++) {
        ss.clear();
        ss.str("");
        cleaver::FloatField *field = static_cast<cleaver::FloatField*>(volume->getMaterial(i));
        ss << output << "_m" << i;
        std::string filepath = ss.str();
        std::cout << "Writing material " << i << ": " << filepath << ".nrrd" << std::endl;
        saveNRRDFile(field, filepath);
    }

}
