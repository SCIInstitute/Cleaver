// Utils Includes
#include "util.h"

// STL Includes
#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cmath>

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
    Cleaver::TetMesh *beforeMesh = Cleaver::TetMesh::createFromNodeElePair(beforeFileName + nodeExt, beforeFileName + eleExt);
    Cleaver::TetMesh  *afterMesh = Cleaver::TetMesh::createFromNodeElePair(afterFileName  + nodeExt, afterFileName  + eleExt);

    if(!beforeMesh || !afterMesh){
        std::cerr << "Failed to load meshes. Aborting." << std::endl;
        return 0;
    }

    //std::cout << "beforeMesh = " << beforeMesh->tets.size() << " tets total." << std::endl;
    //std::cout << "afterMesh = " << afterMesh->tets.size() << " tets total." << std::endl;

    //-----------------------------------------------------------
    // Perform Analysis and Output to Console
    //-----------------------------------------------------------
    // one sample for every tet in after
    for(int t=0; t < afterMesh->tets.size(); t++)
    {
        Cleaver::Tet  *afterTet = afterMesh->tets[t];
        Cleaver::Tet *beforeTet = beforeMesh->tets[afterTet->parent - 1];

        std::cout << std::abs(beforeTet->volume()) << " " << std::abs(afterTet->volume()) << std::endl;
    }

}
