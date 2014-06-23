

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
//  - Dump Angles - Command Line Program
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
#include <Cleaver/InverseField.h>
#include <Cleaver/SizingFieldCreator.h>

#include <boost/program_options.hpp>

// STL Includes
#include <exception>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>

namespace po = boost::program_options;

// Entry Point
int main(int argc,	char* argv[])
{  
    bool no_boundary_elements = false;
    std::string mesh_prefix;

    //-------------------------------
    //  Parse Command Line Params
    //-------------------------------
    try{
        po::options_description description("Command line flags");
        description.add_options()
                ("help,h", "display help message")
                ("mesh_prefix", po::value<std::string>(), "mesh prefix path")
                ("no_boundary_elements", "exclude boundary element angles")
        ;

        boost::program_options::variables_map variables_map;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), variables_map);
        boost::program_options::notify(variables_map);

        // print help
        if (variables_map.count("help") || (argc ==1)) {
            std::cout << description << std::endl;
            return 0;
        }

        // get mesh prefix path
        if (variables_map.count("mesh_prefix")) {
            mesh_prefix = variables_map["mesh_prefix"].as<std::string>();
        }
        else {
            std::cerr << "Error: Must provide an input mesh prefix" << std::endl;
            return 0;
        }

        // check if boundary elements are to be ignored
        if (variables_map.count("no_boundary_elements")) {
            no_boundary_elements = true;
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

    //---------------------------------------------------------------------------
    // Load the Mesh
    //---------------------------------------------------------------------------
    std::string nodeFileName = mesh_prefix + ".node";
    std::string eleFileName = mesh_prefix + ".ele";
    cleaver::TetMesh *mesh =  cleaver::TetMesh::createFromNodeElePair(nodeFileName, eleFileName);
    if(!mesh){
        std::cerr << "Failed to load mesh. Terminating." << std::endl;
        return 0;
    }

    //---------------------------------------------------------------------------
    // Compute Angles and Print Them
    //---------------------------------------------------------------------------
    if(no_boundary_elements){
        mesh->constructBottomUpIncidences();
    }

    for (size_t t=0; t < mesh->tets.size(); t++)
    {
        cleaver::Tet *tet = mesh->tets[t];

        // if missing any adjacent tets, skip it
        if(no_boundary_elements && (tet->tets[0] == NULL || tet->tets[1] == NULL ||
                                    tet->tets[2] == NULL || tet->tets[3] == NULL)) {
            continue;
        }

        //each tet has 6 dihedral angles between pairs of faces
        //compute the face normals for each face
        cleaver::vec3 face_normals[4];

        for (int j=0; j<4; j++) {
           cleaver::vec3 v0 = tet->verts[(j+1)%4]->pos();
           cleaver::vec3 v1 = tet->verts[(j+2)%4]->pos();
           cleaver::vec3 v2 = tet->verts[(j+3)%4]->pos();
           cleaver::vec3 normal = normalize(cross(v1-v0,v2-v0));

           // make sure normal faces 4th (opposite) vertex
           cleaver::vec3 v3 = tet->verts[(j+0)%4]->pos();
           cleaver::vec3 v3_dir = normalize(v3 - v0);
           if(cleaver::dot(v3_dir, normal) > 0)
               normal *= -1;

           face_normals[j] = normal;
        }


        //now compute the 6 dihedral angles between each pair of faces
        for (int j=0; j<4; j++) {
           for (int k=j+1; k<4; k++) {
              double dot_product = dot(face_normals[j], face_normals[k]);
              if (dot_product < -1) {
                 dot_product = -1;
              } else if (dot_product > 1) {
                 dot_product = 1;
              }

              double dihedral_angle = 180.0 - acos(dot_product) * 180.0 / PI;

              std::cout << dihedral_angle << std::endl;
           }
        }
    }


    return 0;
}

