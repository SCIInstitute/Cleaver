#ifndef CLEAVERMESHER_H
#define CLEAVERMESHER_H

#include "Octree.h"

namespace cleaver
{
    class Volume;
    class TetMesh;
    class CleaverMesherImp;

class CleaverMesher
{
public:

    CleaverMesher(const Volume *volume);
    ~CleaverMesher();

    void createTetMesh(bool verbose);
    TetMesh* getTetMesh() const;
    TetMesh* getBackgroundMesh() const;

    void setVolume(const Volume *volume);
    const Volume* getVolume() const;

    void cleanup();    
    //================================

    enum TopologyMode { TopologyModeNone, TopologyModeSubdivide, TopologyModeCleave };

    void setTopologyMode(TopologyMode mode);
		void setAlphaInit(double alpha);

    //================================
    // Functions for development ONLY.
    // Remove after completion.
    //================================    
    void createBackgroundMesh();
    void setBackgroundMesh(TetMesh *);
    void buildAdjacency();
    void sampleVolume();
    void computeAlphas();
    void computeInterfaces();
    void generalizeTets();
    void snapsAndWarp();
    void stencilTets();

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
    // Data Getters.
    //================================
    Octree* getTree() const;

private:

    CleaverMesherImp *m_pimpl;
};
}

#endif // CLEAVERMESHER_H
