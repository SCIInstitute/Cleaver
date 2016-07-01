#ifndef CLEAVERMESHER_H
#define CLEAVERMESHER_H

#include "Octree.h"
#include "CleaverMesherImpl.h"

namespace cleaver {
class CleaverMesher
{
public:

    CleaverMesher();
    ~CleaverMesher();

    void createTetMesh(bool verbose);
    TetMesh* getTetMesh() const;
    TetMesh* getBackgroundMesh() const;

    void setVolume(const Volume *volume);
    Volume* getVolume() const;

    void cleanup();    
    //================================

    void setTopologyMode(CleaverMesherImp::TopologyMode mode);
		void setAlphaInit(double alpha);

    //================================
    // Functions for development ONLY.
    // Remove after completion.
    //================================    
    TetMesh* createBackgroundMesh(bool verbose = false);
    void setBackgroundMesh(TetMesh *);
    void buildAdjacency(bool verbose = false);
    void sampleVolume(bool verbose = false);
    void computeAlphas(bool verbose = false);
    void computeInterfaces(bool verbose = false);
    void generalizeTets(bool verbose = false);
    void snapsAndWarp(bool verbose = false);
    void stencilTets(bool verbose = false);
    size_t fixVertexWindup(bool verbose = false);

    //================================
    // State Getters.
    //================================
    bool backgroundMeshCreated() const;
    bool adjacencyBuilt() const;
    bool samplingDone() const;
    bool alphasComputed() const;
    bool interfacesComputed() const;
    bool generalized() const;
    bool snapsAndWarpsDone() const;
    bool stencilsDone() const;
    bool completed() const;

    //================================
    // Temporary Experimental Methods
    //================================
    void setSizingFieldTime(double time);
    void setBackgroundTime(double time);
    void setCleavingTime(double time);

    double getSizingFieldTime() const;
    double getBackgroundTime() const;
    double getCleavingTime() const;

    //================================
    // Data Getters / Setters
    //================================
    Octree* getTree() const;
    void setAlphas(double l, double s);
    void setRegular(bool reg);

private:
    CleaverMesherImp m_pimpl;
    double m_alpha_long;
    double m_alpha_short;
    bool m_regular;
};
}

#endif // CLEAVERMESHER_H
