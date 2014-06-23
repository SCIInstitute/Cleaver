// Project Includes
#include "util.h"
#include <Synthetic/BlobbyField.h>
#include <Particle/ParticleMesher.h>
#include <Cleaver/CleaverMesher.h>

// STL Includes
#include <cstdlib>

// Timing Includes
#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/time.h>
#include <ctime>
#endif

//-------------------
// Helper Function
//-------------------

float urandf()
{
    #ifdef RAND_MAX
    return ((float)rand() / RAND_MAX);
    #else
    return ((float)rand() / std::numeric_limits<int>::max());
    #endif
}



//    // create and return input param structure
//    InputParams *in = new InputParams;
//    in->dims = cleaver::vec3(dim_x, dim_y, dim_z);
//    in->m    = m;
//    in->path = path;



//------------------------------------------------------
// Construct Input Volume
//------------------------------------------------------
cleaver::Volume* createInputVolume(InputParameters *input)
{
    // create bounds
    cleaver::BoundingBox bounds(cleaver::vec3::zero, input->dims);

    // create list to store all of the materials
    std::vector<cleaver::AbstractScalarField*> fields;

    // create background material
    cleaver::ConstantField<float> *backField = new cleaver::ConstantField<float>(0, bounds);
    fields.push_back(backField);

    // create material indicator functions
    for(int i=0; i < input->m; i++)
    {
        // center blobby randomly between 5% and 95% from boundaries
        double rcx = (0.9*urandf() + 0.05) * 0.5*bounds.size.x + 0.25*bounds.size.x;
        double rcy = (0.9*urandf() + 0.05) * 0.5*bounds.size.y + 0.25*bounds.size.y;
        double rcz = (0.9*urandf() + 0.05) * 0.5*bounds.size.z + 0.25*bounds.size.z;

        float rr = 8 + urandf()*20;

        BlobbyField *field = new BlobbyField(cleaver::vec3(rcx,rcy,rcz), rr, 8, bounds);
        fields.push_back(field);
    }


    // construct the volume from fields list
    cleaver::Volume *basevol = new cleaver::Volume(fields);
    cleaver::Volume *volume = cleaver::createFloatFieldVolumeFromVolume(basevol);
    volume->setName("Blobby Volume");

    return volume;
}


cleaver::FloatField* createSizingField(cleaver::Volume *volume, float lipschitz)
{
    return cleaver::SizingFieldCreator::createSizingFieldFromVolume(volume, 1/lipschitz);
}

cleaver::TetMesh* createParticleMesh(cleaver::Volume *volume)
{
    //-----------------------------
    // Sample and Optimize Particles
    //-----------------------------
    ParticleMesher particleMesher;
    particleMesher.setVolume(volume);
    particleMesher.initCPU();

    int i = 0;
    while(!particleMesher.converged() && i < 200)
    {
        particleMesher.stepCPU();
    }
    particleMesher.cleanupCPU();

    //--------------------------
    // Tetrahedralize with Tetgen
    //--------------------------
    particleMesher.tesselateCPU();

    return particleMesher.getMesh();
}

//--------------------------------------------------
// Improve Mesh With Stellar
//--------------------------------------------------
void improveMeshWithStellar(cleaver::TetMesh *mesh)
{

}

//------------------------------------------------------
// Apply Cleaving Algorithm to Mesh
//------------------------------------------------------
void cleaveMeshWithVolume(cleaver::TetMesh *mesh, cleaver::Volume *volume)
{
    cleaver::CleaverMesher mesher(volume);
    mesher.setBackgroundMesh(mesh);
    mesher.buildAdjacency();
    mesher.sampleVolume();
    mesher.computeAlphas();
    mesher.computeInterfaces();
    mesher.generalizeTets();
    mesher.snapsAndWarp();
    mesher.stencilTets();
}

/*
Cleaver::Volume* loadSphereData(int n, Cleaver::BoundingBox &bounds, vector<SphereField*> &sfields)
{

//    SphereField *f1 = new SphereField(Cleaver::vec3(48.1,64,64), 12.0, bounds);
//    SphereField *f2 = new SphereField(Cleaver::vec3(80.1,64,64), 12.0, bounds);
    Cleaver::ConstantField *f3 = new Cleaver::ConstantField(0, bounds);
//    f1->setBounds(bounds);
//    f2->setBounds(bounds);
    std::vector<Cleaver::ScalarField*> fields;
//    fields.push_back(f1);
//    fields.push_back(f2);
//    fields.push_back(f3);

    for (int i=0; i<sfields.size(); i++){
        BlobbyField *bf = new BlobbyField(bounds);
    //    fields.push_back(sfields[i]);
        int num_blob = rand()%5;
        for (int j=0; j<num_blob; j++){
            Cleaver::vec3 pos;
            bf->a =  4*sfields[i]->m_r;
            bf->b = bf->a/2.0;
            bf->c = bf->b/2.0;
            pos[0] = sfields[i]->m_cx[0] + bf->a*(rand()/(double)RAND_MAX - 0.5);
            pos[1] = sfields[i]->m_cx[1] + bf->a*(rand()/(double)RAND_MAX - 0.5);
            pos[2] = sfields[i]->m_cx[2] + bf->a*(rand()/(double)RAND_MAX - 0.5);
            bf->spheres.push_back(pos);
        }
        fields.push_back(bf);
    }
    fields.push_back(f3);
    Cleaver::Volume *basevol = new Cleaver::Volume(fields);
    Cleaver::Volume *volume = Cleaver::createFloatFieldVolumeFromVolume(basevol);
    return volume;
//    volume->setName("Sphere Data");

}
*/
