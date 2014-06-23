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
int main(int argc, char* argv[])
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

    beforeMesh->constructBottomUpIncidences();
    afterMesh->constructBottomUpIncidences();

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

        double min_edge_before = 10000000;
        double min_edge_after  = 10000000;

        std::vector<Cleaver::HalfEdge*> beforeEdges = beforeMesh->edgesAroundTet(beforeTet);
        for(unsigned int e=0; e < beforeEdges.size(); e++)
        {
            Cleaver::HalfEdge *edge = beforeEdges[e];
            double edgelength = length(edge->vertex->pos() - edge->mate->vertex->pos());
            if(edgelength < min_edge_before)
                min_edge_before = edgelength;
        }

        std::vector<Cleaver::HalfEdge*>  afterEdges = afterMesh->edgesAroundTet( afterTet);
        for(unsigned int e=0; e < afterEdges.size(); e++)
        {
            Cleaver::HalfEdge *edge = afterEdges[e];
            double edgelength = length(edge->vertex->pos() - edge->mate->vertex->pos());
            if(edgelength < min_edge_after)
                min_edge_after = edgelength;
        }


        // print pair
        std::cout << min_edge_before << " " << min_edge_after << std::endl;
    }

    return 0;
}
