#ifndef __CLEAVERMESHIMP_H__
#define __CLEAVERMESHIMP_H__

#include "vec3.h"
#include "TetMesh.h"
#include "Volume.h"
#include "Vertex.h"
#include "ScalarField.h"
#include "SizingFieldOracle.h"
#include <set>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include "CleaverMesher.h"
#include "InterfaceCalculator.h"
#include "ViolationChecker.h"

namespace cleaver {

class CleaverMesherImp
{
public:
    CleaverMesherImp();
    ~CleaverMesherImp();

    void topologicalCleaving();
    void resetMeshProperties();
    void recordOperations(std::string input);
    TetMesh* createBackgroundMesh(bool verbose = false);
    void setBackgroundMesh(TetMesh*);

    void computeTopologicalInterfaces(bool verbose = false);
    void generalizeTopologicalTets(bool verbose = false);

    void buildAdjacency(bool verbose = false);
    void sampleVolume(bool verbose = false);
    void computeAlphas(bool verbose = false,
                       bool regular = false,
                       double alp_long = 0.4,
                       double alp_short = 0.4);
    void computeAlphasSafely(bool verbose = false);
    void makeTetAlphaSafe(Tet *tet);
    void updateAlphaLengthAroundVertex(Vertex *vertex, float alpha_length);
    float computeSafeAlphaLength1(Tet *tet, int v);
    float computeSafeAlphaLength2(Tet *tet, int v);

    void computeInterfaces(bool verbose = false);
    void generalizeTets(bool verbose = false);
    void snapAndWarpViolations(bool verbose = false);
    void stencilBackgroundTets(bool verbose = false);

    void snapAndWarpVertexViolations(bool verbose = false);
    void snapAndWarpEdgeViolations(bool verbose = false);
    void snapAndWarpFaceViolations(bool verbose = false);

    void snapAndWarpForViolatedVertex(Vertex *vertex);
    void snapAndWarpForViolatedEdge(HalfEdge *edge);
    void snapAndWarpForViolatedFace(HalfFace *face);

    void conformQuadruple(Tet *tet, Vertex *warpVertex, const vec3 &warpPt);
    void conformTriple(HalfFace *face, Vertex *warpVertex, const vec3 &warpPt);

    void snapCutForEdgeToVertex(HalfEdge *edge, Vertex* vertex);
    void snapTripleForFaceToVertex(HalfFace *face, Vertex* vertex);
    void snapQuadrupleForTetToVertex(Tet *tet, Vertex* vertex);

    void snapTripleForFaceToCut(HalfFace *face, Vertex *cut);
    void snapQuadrupleForTetToCut(Tet *tet, Vertex *cut);
    void snapQuadrupleForTetToEdge(Tet *tet, HalfEdge *edge);
    void snapQuadrupleForTetToTriple(Tet *tet, Vertex *triple);

    void resolveDegeneraciesAroundVertex(Vertex *vertex);
    void resolveDegeneraciesAroundEdge(HalfEdge *edge);

    Tet* getInnerTet(HalfEdge *edge, Vertex *warpVertex, const vec3 &warpPt);
    Tet* getInnerTet(HalfFace *face, Vertex *warpVertex, const vec3 &warpPt);
    vec3 projectCut(HalfEdge *edge, Tet *tet, Vertex *warpVertex, const vec3 &warpPt);
    vec3 projectTriple(HalfFace *face, Vertex *quadruple, Vertex *warpVertex, const vec3 &warpPt);


    // -- algorithm state --
    bool m_bBackgroundMeshCreated;
    bool m_bAdjacencyBuilt;
    bool m_bSamplingDone;
    bool m_bAlphasComputed;
    bool m_bInterfacesComputed;
    bool m_bGeneralized;
    bool m_bSnapsAndWarpsDone;
    bool m_bStencilsDone;
    bool m_bComplete;
    bool m_bRecordOperations;

    std::set<size_t> m_tets_to_record;

    double m_sizing_field_time;
    double m_background_time;
    double m_cleaving_time;

    CleaverMesher::TopologyMode m_topologyMode;
    double m_alpha_init;

    Volume *m_volume;
    AbstractScalarField *m_sizingField;
    SizingFieldOracle   *m_sizingOracle;
    InterfaceCalculator *m_interfaceCalculator;
    ViolationChecker    *m_violationChecker;
    TetMesh *m_bgMesh;
    TetMesh *m_mesh;
};

} // namespace cleaver

#endif // __CLEAVERMESHIMP_H__
