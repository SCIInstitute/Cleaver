//------------------------------------------------------
// Cleaver Includes
#include <Cleaver/CleaverMesher.h>
#include <Cleaver/ScalarField.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/SizingFieldCreator.h>

#include <string>

//------------------------------------------------------
// Command-line Parameters
//------------------------------------------------------
struct InputParameters
{
    cleaver::vec3 dims;        // volume dimensions
    int m;                     // number of materials
    std::string path;    // output mesh filename
};


/*
//------------------------------------------------------
// Parse Command-line Params
//------------------------------------------------------
InputParams* parseCommandline(int argc, char* argv[]);
*/


//------------------------------------------------------
// Construct Input Volume
//------------------------------------------------------
cleaver::Volume* createInputVolume(InputParameters *input);


//------------------------------------------------------
// Construct Sizing Field
//------------------------------------------------------
cleaver::FloatField* createSizingField(cleaver::Volume *volume, float lipschitz);

//------------------------------------------------------
// Construct Particle Mesh
//------------------------------------------------------
cleaver::TetMesh* createParticleMesh(cleaver::Volume *volume);

//------------------------------------------------------
// Improve Mesh With Stellar
//------------------------------------------------------
void improveMeshWithStellar(cleaver::TetMesh *mesh);

//------------------------------------------------------
// Apply Cleaving Algorithm to Mesh
//------------------------------------------------------
void cleaveMeshWithVolume(cleaver::TetMesh *mesh, cleaver::Volume *volume);



//Cleaver::Volume* loadSphereData(int n, Cleaver::BoundingBox &bounds, std::vector<Cleaver::SphereField*> &sfields);
