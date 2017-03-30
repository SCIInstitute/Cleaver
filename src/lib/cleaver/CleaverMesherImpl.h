#ifndef __CLEAVERMESHIMP_H__
#define __CLEAVERMESHIMP_H__

#include "vec3.h"
#include "TetMesh.h"
#include "Volume.h"
#include "Vertex.h"
#include "ScalarField.h"
#include "SizingFieldOracle.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include "CleaverMesher.h"

namespace cleaver {

#ifndef nullptr
#define nullptr 0
#endif

#define VERTS_PER_FACE 3
#define EDGES_PER_FACE 3
#define TETS_PER_FACE 2

#define VERTS_PER_TET 4
#define EDGES_PER_TET 6
#define FACES_PER_TET 4

class vec3order{
public:

    bool operator()(const vec3 &a, const vec3 &b) const
    {

        bool less = ( less_eps(a.x, b.x) ||
                    (equal_eps(a.x, b.x) &&  less_eps(a.y, b.y)) ||
                    (equal_eps(a.x, b.x) && equal_eps(a.y, b.y)  && less_eps(a.z, b.z)));


        return less;
    }

private:

    double equal_eps(double x, double y) const
    {
        double tol = std::max(x,y)*eps;

        if(fabs(x-y) <= tol)
            return true;
        else
            return false;
    }

    double less_eps(double x, double y) const
    {
        double tol = std::max(x,y)*eps;

        if(fabs(x-y) <= tol)
            return false;
        else
            return (x < y);
    }
    static double eps;
};


class CleaverMesherImp
{
public:
    CleaverMesherImp();
    ~CleaverMesherImp();
    Octree* createOctree();
    Octree* createOctreeBottomUp();
    void resetCellsAroundPos(const vec3 &pos);
    void resetPosForCell(OTCell *cell);
    std::vector<OTCell*> cellsAroundPos(const vec3 &pos);
    vec3 truePosOfPos(const vec3 &pos);
    bool posWasWarped(const vec3 &pos);
    bool checkSafetyBetween(const vec3 &v1_orig, const vec3 &v2_orig, bool v1_warpable = true, bool v2_warpable = true);
    void createWarpedVertices(OTCell *cell);
    void makeCellRespectTopology(OTCell *cell);
    void subdivideTreeToTopology();
    void conformCellToDomain(OTCell *cell);
    void conformOctreeToDomain();
    void balanceOctree();
    void balanceOctreeNew();
    void altBalanceOctree();
    void createBackgroundVerts();
    void createBackgroundTets();
    void topologicalCleaving();
    void resetMeshProperties();
    TetMesh* createBackgroundMesh(bool verbose = false);
    void setBackgroundMesh(TetMesh*);
    void adaptCell(OTCell *cell);

    void createVisualizationTets(OTCell *cell);
    TetMesh* createVisualizationBackgroundMesh();
    Vertex* vertexForPosition(const vec3 &pos, bool create=true);
    vec3    warpForPosition(const vec3 &position);
    void setWarpForPosition(const vec3 &position, const vec3 &warp);

    void computeLagrangePolynomial(const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4, double coefficients[4]);
    void computeTopologicalInterfaces(bool verbose = false);
    void computeTopologicalCutForEdge(HalfEdge *edge);
    void computeTopologicalCutForEdge2(HalfEdge *edge);
    void computeTopologicalTripleForFace(HalfFace *face);
    void computeTopologicalQuadrupleForTet(Tet *tet);
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
    float computeSafeAlphaLength(Tet *tet, int v);

    void computeInterfaces(bool verbose = false);
    void computeCutForEdge(HalfEdge *edge);
    void computeTripleForFace(HalfFace *face);
    void computeTripleForFace2(HalfFace *face);
    void computeQuadrupleForTet(Tet* tet);
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

    void checkIfCutViolatesVertices(HalfEdge *edge);
    void checkIfTripleViolatesVertices(HalfFace *face);
    void checkIfQuadrupleViolatesVertices(Tet *tet);

    void checkIfTripleViolatesEdges(HalfFace *face);  // to write and use
    void checkIfQuadrupleViolatesEdges(Tet *tet);     // to write and use
    void checkIfQuadrupleViolatesFaces(Tet *tet);     // to write and use

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

    double m_sizing_field_time;
    double m_background_time;
    double m_cleaving_time;

    CleaverMesher::TopologyMode m_topologyMode;
    double m_alpha_init;

    Volume *m_volume;
    AbstractScalarField *m_sizingField;
    SizingFieldOracle   *m_sizingOracle;
    Octree *m_tree;
    TetMesh *m_bgMesh;
    TetMesh *m_mesh;
    std::map<vec3, Vertex*, vec3order> m_vertex_tracker;
    std::map<vec3,    vec3, vec3order> m_warp_tracker;
};

} // namespace cleaver

#endif // !__CLEAVERMESHIMP_H__
