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

        double worst_edge_ratio = 1.0;

        // find shortest and longest edges touching beforeTet
        for(int v=0; v < 4; v++)
        {
            Cleaver::Vertex *vertex = beforeTet->verts[v];
            std::vector<Cleaver::HalfEdge*> edges = beforeMesh->edgesAroundVertex(vertex);
            double shortest_edge = 1000000;
            double longest_edge = 0;

            for(int e=0; e < edges.size(); e++)
            {
                Cleaver::HalfEdge *edge = edges[e];
                double edgelength = length(edge->vertex->pos() - edge->mate->vertex->pos());

                if(edgelength < shortest_edge)
                    shortest_edge = edgelength;
                if(edgelength > longest_edge)
                    longest_edge = edgelength;
            }

            // compute edge ratio
            double edge_ratio = std::min(shortest_edge / longest_edge, longest_edge / shortest_edge);
            if(edge_ratio < worst_edge_ratio)
                worst_edge_ratio = edge_ratio;
        }


        // compute quality penalty ratio
        double quality_ratio = (afterTet->minAngle() / beforeTet->minAngle()) - 1.0;

        // print pair
        std::cout << worst_edge_ratio << " " << quality_ratio << std::endl;
    }

    return 0;
}
