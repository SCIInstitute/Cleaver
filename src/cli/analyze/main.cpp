// Utils Includes
#include "util.h"

// STL Includes
#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <cstdlib>

#include <Cleaver/TetMesh.h>

const std::string nodeExt = std::string(".node");
const std::string  eleExt = std::string(".ele");

// Entry Point
int main(int argc,	char* argv[])
{
    //-----------------------------------------------------------
    // Parse Command-line Params
    //-----------------------------------------------------------
    if(argc < 3){
        std::cerr << "usage: " << argv[0] << " [meshname1] [meshname2]" << std::endl;
        exit(0);
    }

    std::string beforeFileName = std::string(argv[1]);
    std::string  afterFileName = std::string(argv[2]);

    //-----------------------------------------------------------
    // Load the Tet Meshes In
    //-----------------------------------------------------------

    cleaver::TetMesh *beforeMesh = cleaver::TetMesh::createFromNodeElePair(beforeFileName + nodeExt, beforeFileName + eleExt);
    cleaver::TetMesh  *afterMesh = cleaver::TetMesh::createFromNodeElePair(afterFileName  + nodeExt, afterFileName  + eleExt);

    if(!beforeMesh || !afterMesh){
        std::cerr << "Failed to load meshes. Aborting." << std::endl;
        return 0;
    }
    else
        std::cout << "Meshes Loaded. Computing.." << std::endl;

    //std::cout << "beforeMesh = " << beforeMesh->tets.size() << " tets total." << std::endl;
    //std::cout << "afterMesh = " << afterMesh->tets.size() << " tets total." << std::endl;

    //-----------------------------------------------------------
    // Perform Analysis and Output to Console
    //-----------------------------------------------------------
    // one sample for every tet in after
    for(size_t t=0; t < afterMesh->tets.size(); t++)
    {
        cleaver::Tet  *afterTet = afterMesh->tets[t];

        unsigned int parentIndex = afterTet->parent - 1;
        if(parentIndex >= beforeMesh->tets.size()){
            std::cerr << "Error: Tet " << parentIndex << " doesn't exist in parent." << std::endl;
            continue;
        }

        cleaver::Tet *beforeTet = beforeMesh->tets[parentIndex];



        std::cout << beforeTet->minAngle() << " " << afterTet->minAngle() << std::endl;
    }

}
