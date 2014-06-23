#include "CleaverMesher.h"
#include "StencilTable.h"
#include "TetMesh.h"
#include "Octree.h"
#include "ScalarField.h"
#include "BoundingBox.h"
#include "Plane.h"
#include "Matrix3x3.h"
#include "Volume.h"
#include "Timer.h"
#include "SizingFieldCreator.h"
#include "SizingFieldOracle.h"
#include "vec3.h"
#include <queue>
#include <stack>
#include <map>
#include <cmath>
#include <cstdlib>

extern std::vector<cleaver::vec3>  viol_point_list;
extern std::vector<cleaver::Plane> viol_plane_list;
std::vector<cleaver::vec3> badEdges;

int SolveQuadric(double c[ 3 ], double s[ 2 ]);
int SolveCubic(double c[ 4 ], double s[ 3 ]);
void clipRoots(double s[3], int &num_roots);

namespace cleaver
{
#ifndef NULL
#define NULL 0
#endif

#define VERT 0
#define CUT  1
#define TRIP 2
#define QUAD 3

#define VERTS_PER_FACE 3
#define EDGES_PER_FACE 3
#define  TETS_PER_FACE 2

#define VERTS_PER_TET 4
#define EDGES_PER_TET 6
#define FACES_PER_TET 4


bool GRID_WARPING = true;
bool SPLIT_ACROSS_CELLS = true;

const int OctantInverse[8] = {
    7,     // 0,0,0 --> 1,1,1
    6,     // 1,0,0 --> 0,1,1
    5,     // 0,1,0 --> 1,0,1
    4,
    3,
    2,
    1
};

// lexographical ordering
const int FACE_NEIGHBOR_OFFSETS[6][3] = {
    {-1, 0, 0},
    {+1, 0, 0},
    { 0,-1, 0},
    { 0,+1, 0},
    { 0, 0,-1},
    { 0, 0,+1}
};

// faces:    lexographical ordering
// vertices: counter-clockwise as seen from center of cell
const int FACE_VERTICES[6][4] = {
    {0,3,7,4},    // v1 v4 v8 v5  (-x face)
    {5,6,2,1},    // v6 v7 v3 v2  (+x face)
    {4,5,1,0},    // v5 v6 v2 v1  (-y face)
    {3,2,6,7},    // v4 v3 v7 v8  (+y face)
    {0,1,2,3},    // v1 v2 v3 v4  (-z face)
    {7,6,5,4}     // v8 v7 v6 v5  (+z face)
};

// easy way to determine diagonal on
// neighboring octants
const bool FACE_DIAGONAL_BIT[6][8] = {
    {1,1,0,0,0,0,1,1},   // (-x face)
    {0,0,1,1,1,1,0,0},   // (+x face)
    {0,1,0,1,1,0,1,0},   // (-y face)
    {1,0,1,0,0,1,0,1},   // (+y face)
    {1,0,0,1,1,0,0,1},   // (-z face)
    {0,1,1,0,0,1,1,0}    // (+z face)
};


const int CELL_EDGE_VERTS[12][2] = {
    { 0, 1 }, // LowerBack
    { 1, 2 }, // LowerRight
    { 2, 3 }, // LowerFront
    { 3, 0 }, // LowerLeft

    { 4, 5 }, // UpperBack
    { 5, 6 }, // UpperRight
    { 6, 7 }, // UpperFront
    { 7, 4 }, // UpperLeft

    { 0, 4 }, // Back Left
    { 1, 5 }, // Back Right
    { 2, 6 }, // Front Right
    { 3, 7 }, // Front Left
};

const int CELL_VERT_COORDS[8][3] = {
   {0,0,0},   // lower back left
   {1,0,0},   // lower back right
   {1,0,1},   // lower front right
   {0,0,1},   // lower front left
   {0,1,0},   // upper back left
   {1,1,0},   // upper back right
   {1,1,1},   // upper front right
   {0,1,1},   // upper front left
};

class vec3order{
public:

    /*
    bool operator()(const vec3 &a, const vec3 &b)
    {
        return
                ((a.x < b.x) ||
                 (a.x == b.x && a.y < b.y) ||
                 (a.x == b.x && a.y == b.y && a.z < b.z));
    }
    */
    bool operator()(const vec3 &a, const vec3 &b) const
    {
        //if(fabs(a.x - 100) < 1E-3 && fabs(b.x - 12.5) < 1E-3)
        //    std::cout << "here" << std::endl;

        bool less = ( less_eps(a.x, b.x) ||
                    (equal_eps(a.x, b.x) &&  less_eps(a.y, b.y)) ||
                    (equal_eps(a.x, b.x) && equal_eps(a.y, b.y)  && less_eps(a.z, b.z)));

        //if(less)
        //   std::cout << a.toString() << " < " << b.toString() << std::endl;

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

double vec3order::eps = 1E-8;



class CleaverMesherImp
{
public:
    CleaverMesherImp();
    ~CleaverMesherImp();
    ScalarField<float>* createSizingField();
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
    TetMesh* createBackgroundMesh();
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
    void computeAlphas(bool verbose = false);
    void computeAlphasAlt(bool verbose = false);
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

CleaverMesherImp::CleaverMesherImp()
{
    // -- set algorithm state --
    m_bBackgroundMeshCreated = false;
    m_bAdjacencyBuilt        = false;
    m_bSamplingDone          = false;
    m_bAlphasComputed        = false;
    m_bInterfacesComputed    = false;
    m_bGeneralized           = false;
    m_bSnapsAndWarpsDone     = false;
    m_bStencilsDone          = false;
    m_bComplete              = false;

    m_volume = NULL;
    m_sizingField = NULL;
    m_sizingOracle = NULL;
    m_tree = NULL;
    m_bgMesh = NULL;
    m_mesh = NULL;

    m_sizing_field_time = 0;
    m_background_time = 0;
    m_cleaving_time = 0;
		m_alpha_init = 0.4;    
}

CleaverMesherImp::~CleaverMesherImp()
{
    delete m_tree;
}

// -- state getters --
bool CleaverMesher::backgroundMeshCreated() const { return m_pimpl->m_bBackgroundMeshCreated; }
bool CleaverMesher::adjacencyBuilt()        const { return m_pimpl->m_bAdjacencyBuilt;        }
bool CleaverMesher::samplingDone()          const { return m_pimpl->m_bSamplingDone;          }
bool CleaverMesher::alphasComputed()        const { return m_pimpl->m_bAlphasComputed;        }
bool CleaverMesher::interfacesComputed()    const { return m_pimpl->m_bInterfacesComputed;    }
bool CleaverMesher::generalized()           const { return m_pimpl->m_bGeneralized;           }
bool CleaverMesher::snapsAndWarpsDone()     const { return m_pimpl->m_bSnapsAndWarpsDone;     }
bool CleaverMesher::stencilsDone()          const { return m_pimpl->m_bStencilsDone;          }
bool CleaverMesher::completed()             const { return m_pimpl->m_bComplete;              }


CleaverMesher::~CleaverMesher()
{
    //cleanup();
    delete m_pimpl;
}

CleaverMesher::CleaverMesher(const Volume *volume) : m_pimpl(new CleaverMesherImp)
{
    m_pimpl->m_volume = const_cast<Volume*>(volume);
    m_pimpl->m_bgMesh = NULL;
    m_pimpl->m_mesh = NULL;
}

void CleaverMesher::createTetMesh(bool verbose)
{
    m_pimpl->createBackgroundMesh();
    m_pimpl->buildAdjacency();
    m_pimpl->sampleVolume();
    m_pimpl->computeAlphas();
    m_pimpl->computeInterfaces();
    m_pimpl->generalizeTets();
    m_pimpl->snapAndWarpViolations();
    m_pimpl->stencilBackgroundTets();
}

TetMesh* CleaverMesher::getBackgroundMesh() const
{
    return m_pimpl->m_bgMesh;
}

TetMesh* CleaverMesher::getTetMesh() const
{
    return m_pimpl->m_mesh;
}


const Volume* CleaverMesher::getVolume() const
{
    return m_pimpl->m_volume;
}

void CleaverMesher::cleanup()
{
    if(m_pimpl->m_bgMesh)
        delete m_pimpl->m_bgMesh;
}

void CleaverMesher::setVolume(const Volume *volume)
{
    cleanup();
    m_pimpl->m_volume = const_cast<Volume*>(volume);
}

void CleaverMesher::setTopologyMode(TopologyMode mode)
{
    std::cout << "Topology mode set to: " << (int)mode << std::endl;
    m_pimpl->m_topologyMode = mode;
}

void CleaverMesher::setAlphaInit(double alpha)
{
	std::cout << "Setting Alpha Initial to: " << alpha << std::endl;
	m_pimpl->m_alpha_init = alpha;
}

//============================================================================
// - warpForPosition()
//
//  This method takes the given coordinates and looks up a MAP to find if the
// background cell vertex at this position has been warped. Returns invalid
// position if no such warp has occured. Invalid position is a negative coord
// since we always use positive coordinates in the Octree.
//============================================================================
vec3 CleaverMesherImp::warpForPosition(const vec3 &position)
{
    vec3 warp = vec3(-1,-1,-1);

    std::map<vec3, vec3, vec3order>::iterator res = m_warp_tracker.find(position);

    if (res != m_warp_tracker.end())
    {
        warp = res->second;
    }

    return warp;
}

void CleaverMesherImp::setWarpForPosition(const vec3 &position, const vec3 &warp)
{
    if(GRID_WARPING)
        m_warp_tracker[position] = warp;
}


//============================================================================
// - vertexForPosition()
//
//  This method takes the given coordinate and looks up a MAP to find background
//  cell vertex that has already been created for this position. IF no such
//  vertex is found, a new one is created, added to the map, and returned.
//  If create is set to false, no vertex is created if one is missing
//============================================================================
Vertex* CleaverMesherImp::vertexForPosition(const vec3 &position, bool create)
{
    vec3 pos = position;
    // if this point has been warped, use the warped position
    {
        std::map<vec3, vec3, vec3order>::iterator res = m_warp_tracker.find(position);
        if(res != m_warp_tracker.end())
        {
            pos = res->second;
        }
    }


    Vertex *vertex = NULL;

    std::map<vec3, Vertex*, vec3order>::iterator res = m_vertex_tracker.find(pos);

    //std::cout << "checking position: " << position.toString() << std::endl;

    // create new one if necessary
    if (res == m_vertex_tracker.end())
    {
        if(create)
        {
            vertex = new Vertex();
            vertex->pos() = pos;
            m_vertex_tracker[pos] = vertex;
        }
    }
    // or return existing one
    else
    {
        vertex = res->second;
    }

    return vertex;
}

//================================================
// createBackgroundMesh()
//================================================
TetMesh* CleaverMesherImp::createBackgroundMesh()
{    
    m_sizingField = m_volume->getSizingField();

    // Ensure We Have Sizing Field (in future, make this a prerequisite for continuing, don't create one.)
    if(!m_sizingField){
        m_sizingField = createSizingField();
        m_volume->setSizingField(m_sizingField);
    }

    if(!m_sizingOracle){
        m_sizingOracle = new SizingFieldOracle(m_sizingField, m_volume->bounds());
    }


    // Create The Octree
    m_tree = createOctree();

    //m_tree = createOctreeBottomUp();

    // Respect Topology
    //subdivideTreeToTopology();
    /*
    std::cout << "pass 2" << endl;
    subdivideTreeToTopology();
    std::cout << "pass 3" << endl;
    subdivideTreeToTopology();
    */

    //conformOctreeToDomain();
    balanceOctreeNew();   // WORKS but looks like we really do need bottom up not top down
                          // A FIX however, is to store only leaves, and prune them as they split

    if(m_bgMesh)
        delete m_bgMesh;
    m_bgMesh = new TetMesh();

    // visit each cell once, ensure
    // vertices are stored
    createBackgroundVerts();

    // visit each child once, post-order
    // output tets to fill each hex cell
    createBackgroundTets();

    // free vertex tracker list
    m_vertex_tracker.clear();

    // set state
    m_bBackgroundMeshCreated = true;

    // TODO: Actually this can happen even for A Single Material
    /*
    if(m_volume->numberOfMaterials() > 2){
        topologicalCleaving();
        topologicalCleaving();
    }
    */

    std::cout << "Background mesh contains\n\t" << m_bgMesh->verts.size() << " verts, " << m_bgMesh->tets.size() << " tets." << std::endl;
    std::cout << "Saving background mesh to file." << std::endl;
    m_bgMesh->writeNodeEle("background",true,false);

    return m_bgMesh;
}

void CleaverMesherImp::setBackgroundMesh(TetMesh *mesh)
{
    if(m_bgMesh)
        delete m_bgMesh;
    m_bgMesh = mesh;
    m_bBackgroundMeshCreated = true;

    std::cout << "New background mesh set." << std::endl;
    std::cout << "Background mesh contains\n\t" << m_bgMesh->verts.size() << " verts, " << m_bgMesh->tets.size() << " tets." << std::endl;
}


//================================================
// - createBackgroundVerts()
//================================================
void CleaverMesherImp::createBackgroundVerts()
{
    std::queue<OTCell*> q;

    q.push(m_tree->root());
    while(!q.empty())
    {
        // Grab Cell and Bounds
        OTCell *cell = q.front();

        //if(cell->celltype != OTCell::Outside){

            // if children, queue them instead
            if(cell->children[0]){
                for(int i=0; i < 8; i++)
                    q.push(cell->children[i]);
            }
            // otherwise, save verts
            else{
                BoundingBox bounds = cell->bounds;
                vertexForPosition(bounds.minCorner());
                vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, 0,             0));
                vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, 0, bounds.size.z));
                vertexForPosition(bounds.minCorner() + vec3(            0, 0, bounds.size.z));
                vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y, 0));
                vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, bounds.size.y, 0));
                vertexForPosition(bounds.maxCorner());
                vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y, bounds.size.z));
                Vertex *center = vertexForPosition(bounds.center());

                center->dual = true;
            }
        //}

        q.pop();
    }
}

//===========================================
// - createBackgroundTets()
//===========================================
void CleaverMesherImp::createBackgroundTets()
{
    std::queue<OTCell*> q;

    q.push(m_tree->root());
    while(!q.empty())
    {
        // Grab Cell and Bounds
        OTCell *cell = q.front();

        if(true || cell->celltype != OTCell::Outside){

            // if there are children, enqueue them
            if(cell->children[0])
            {
                for(int i=0; i < 8; i++)
                    q.push(cell->children[i]);
                q.pop();
                continue;
            }

            BoundingBox bounds = cell->bounds;

            // get original boundary positions
            vec3 original_positions[9];
            original_positions[0] = bounds.minCorner();
            original_positions[1] = bounds.minCorner() + vec3(bounds.size.x, 0,             0);
            original_positions[2] = bounds.minCorner() + vec3(bounds.size.x, bounds.size.y, 0);
            original_positions[3] = bounds.minCorner() + vec3(            0, bounds.size.y, 0);
            original_positions[4] = bounds.minCorner() + vec3(            0, 0, bounds.size.z);
            original_positions[5] = bounds.minCorner() + vec3(bounds.size.x, 0, bounds.size.z);
            original_positions[6] = bounds.maxCorner();
            original_positions[7] = bounds.minCorner() + vec3(            0, bounds.size.y, bounds.size.z);
            original_positions[8] = bounds.center();

            // Determine Ordered Verts
            Vertex* verts[9] = {0};
            for(int i=0; i < 9; i++)
                verts[i] = vertexForPosition(original_positions[i]);


            // Collect face neighbors
            OTCell* fn[6] = {0};
            for(int f=0; f < 6; f++)
                fn[f] = m_tree->getNeighborAtLevel(cell, f, cell->level);

            Vertex* c1 = verts[8];

            vec3 original_c1 = original_positions[8];

            // create tets for each face
            for(int f=0; f < 6; f++)
            {
                // no neighbor? We're on boundary
                if(!fn[f])
                {
                    // grab vertex in middle of face on boundary
                    Vertex *b = vertexForPosition(0.25*(verts[FACE_VERTICES[f][0]]->pos() +
                                                        verts[FACE_VERTICES[f][1]]->pos() +
                                                        verts[FACE_VERTICES[f][2]]->pos() +
                                                        verts[FACE_VERTICES[f][3]]->pos()));

                    bool split = false;

                    // look at 4 lattice tets, does edge spanning boundary have a middle vertex?
                    for(int e=0; e < 4; e++)
                    {
                        Vertex *v1 = verts[FACE_VERTICES[f][(e+0)%4]];
                        Vertex *v2 = verts[FACE_VERTICES[f][(e+1)%4]];

                        vec3 original_v1 = original_positions[FACE_VERTICES[f][(e+0)%4]];
                        vec3 original_v2 = original_positions[FACE_VERTICES[f][(e+1)%4]];

                        //Vertex * m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
                        Vertex * m = vertexForPosition(0.5*(original_v1 + original_v2), false);

                        if(m){
                            split = true;
                            break;
                        }
                    }

                    // if there are any splits, output 2 quadrisected BCC tets for each
                    // face that needs it and a biseceted BCC tet on the edges without splits
                    if(split)
                    {
                        for(int e=0; e < 4; e++)
                        {
                            Vertex *v1 = verts[FACE_VERTICES[f][(e+0)%4]];
                            Vertex *v2 = verts[FACE_VERTICES[f][(e+1)%4]];

                            vec3 original_v1 = original_positions[FACE_VERTICES[f][(e+0)%4]];
                            vec3 original_v2 = original_positions[FACE_VERTICES[f][(e+1)%4]];

                            //Vertex * m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
                            Vertex * m = vertexForPosition(0.5*(original_v1 + original_v2), false);

                            // if edge is split
                            if(m){
                                // create 2 quadrisected tets (3-->red)
                                m_bgMesh->createTet(c1,v1,m,b,3);
                                m_bgMesh->createTet(c1,m,v2,b,3);
                            }
                            else
                            {
                                // create bisected BCC tet  (2-->yellow)
                                m_bgMesh->createTet(c1,v1,v2,b,2);
                            }
                        }
                    }
                    // otherwise, output 2 pyramids
                    else{

                        Vertex *v1 = verts[FACE_VERTICES[f][0]];
                        Vertex *v2 = verts[FACE_VERTICES[f][1]];
                        Vertex *v3 = verts[FACE_VERTICES[f][2]];
                        Vertex *v4 = verts[FACE_VERTICES[f][3]];

                        // output 2 pyramids
                        // the exterior shared diagonal must adjoin the corner and the center of pCell's parent.
                        if(FACE_DIAGONAL_BIT[f][cell->index()])
                        {
                            m_bgMesh->createTet(c1,v1,v2,v3,5);
                            m_bgMesh->createTet(c1,v3,v4,v1,5);
                        }
                        else
                        {
                            m_bgMesh->createTet(c1,v2,v3,v4,5);
                            m_bgMesh->createTet(c1,v4,v1,v2,5);
                        }
                    }
                }

                // same level?
                else if(fn[f]->level == cell->level && !fn[f]->hasChildren())
                {
                    // only output if in positive side (to avoid duplicate tet when neighbor cell is examined)
                    if(f%2==1){

                        // look at 4 lattice tets, does edge spanning cells have a middle vertex?
                        for(int e=0; e < 4; e++)
                        {
                            Vertex* v1 = verts[FACE_VERTICES[f][(e+0)%4]];
                            Vertex* v2 = verts[FACE_VERTICES[f][(e+1)%4]];

                            vec3 original_v1 = original_positions[FACE_VERTICES[f][(e+0)%4]];
                            vec3 original_v2 = original_positions[FACE_VERTICES[f][(e+1)%4]];

                            //Vertex*  m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
                            Vertex*  m = vertexForPosition(0.5*(original_v1 + original_v2), false);
                            Vertex* c2 = vertexForPosition(fn[f]->bounds.center(), false);

                            vec3 original_c2 = fn[f]->bounds.center();
                            //Vertex* b = vertexForPosition(0.5f*(original_c1 + original_c2));
                            Vertex* b = vertexForPosition(0.25f*original_positions[FACE_VERTICES[f][(e+0)%4]] +
                                                          0.25f*original_positions[FACE_VERTICES[f][(e+1)%4]] +
                                                          0.25f*original_positions[FACE_VERTICES[f][(e+2)%4]] +
                                                          0.25f*original_positions[FACE_VERTICES[f][(e+3)%4]]);

                            // if yes, output 2 bisected BCC tets
                            if(m)
                            {
                                if(SPLIT_ACROSS_CELLS){
                                    m_bgMesh->createTet(c1,v1,m,b,1);
                                    m_bgMesh->createTet(v1,m,b,c2,1);

                                    m_bgMesh->createTet(c1,m,v2,b,1);
                                    m_bgMesh->createTet(m,v2,b,c2,1);
                                }
                                else{
                                    m_bgMesh->createTet(c1,v1,m,c2,1);
                                    m_bgMesh->createTet(c1,m,v2,c2,1);
                                }
                            }                            
                            else{
                                // output 1 normal BCC tet
                                if(SPLIT_ACROSS_CELLS){
                                    m_bgMesh->createTet(c1,v1,v2,b,0);
                                    m_bgMesh->createTet(v1,v2,c2,b,0);
                                }
                                else{
                                    m_bgMesh->createTet(c1,v1,v2,c2,0);
                                }
                            }
                        }

                    }
                }

                // neighbor is lower level (should only be one lower...)
                else
                {

                    // grab vertex in middle of face on boundary
                    //Vertex *b = vertexForPosition(0.25*(verts[FACE_VERTICES[f][0]]->pos() +
                    //                                    verts[FACE_VERTICES[f][1]]->pos() +
                    //                                    verts[FACE_VERTICES[f][2]]->pos() +
                    //                                    verts[FACE_VERTICES[f][3]]->pos()), false);
                    Vertex *b = vertexForPosition(0.25*(original_positions[FACE_VERTICES[f][0]] +
                                                        original_positions[FACE_VERTICES[f][1]] +
                                                        original_positions[FACE_VERTICES[f][2]] +
                                                        original_positions[FACE_VERTICES[f][3]]), false);

                    // look at 4 lattice tets, does edge spanning cells have a middle vertex?
                    for(int e=0; e < 4; e++)
                    {
                        Vertex* v1 = verts[FACE_VERTICES[f][(e+0)%4]];
                        Vertex* v2 = verts[FACE_VERTICES[f][(e+1)%4]];

                        vec3 original_v1 = original_positions[FACE_VERTICES[f][(e+0)%4]];
                        vec3 original_v2 = original_positions[FACE_VERTICES[f][(e+1)%4]];

                        //Vertex*  m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
                        Vertex*  m = vertexForPosition(0.5*(original_v1 + original_v2), false);

                        // output 2 quadrisected tets
                        // MODIFIED on Sun 17th
                        //if(m){
                            m_bgMesh->createTet(c1,v1,m,b,4);
                            m_bgMesh->createTet(c1,m,v2,b,4);
                        //}
                        //else{
                            // What should go here?  looks like 1 bisected tet?
                        //}

                    }
                }
            }
        }

        q.pop();
    }
}


//==========================================================
// - createVisualizationTets()
//==========================================================
void CleaverMesherImp::createVisualizationTets(OTCell *cell)
{
    // if children, move down
    if(cell->children[0])
    {
        for(int i=0; i < 8; i++)
            createVisualizationTets(cell->children[i]);
    }
    // otherwise fill it
    else
    {
        // grab bounds of cell
        BoundingBox bounds = cell->bounds;

        // grab/create the 8 vertices
        Vertex *v1 = vertexForPosition(bounds.minCorner());
        Vertex *v2 = vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, 0,             0));
        Vertex *v3 = vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, 0, bounds.size.z));
        Vertex *v4 = vertexForPosition(bounds.minCorner() + vec3(            0, 0, bounds.size.z));
        Vertex *v5 = vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y, 0));
        Vertex *v6 = vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, bounds.size.y, 0));
        Vertex *v7 = vertexForPosition(bounds.maxCorner());
        Vertex *v8 = vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y, bounds.size.z));


        // give block a random coor
        //int m = rand() % 10;
        int m = cell->level;

        // create tets
        m_bgMesh->createTet(v1,v2,v3,v6,m);
        m_bgMesh->createTet(v1,v3,v8,v6,m);
        m_bgMesh->createTet(v1,v3,v4,v8,m);
        m_bgMesh->createTet(v1,v6,v8,v5,m);
        m_bgMesh->createTet(v3,v8,v6,v7,m);
    }
}

//============================================================
//  createVisualizationBackgroundMesh()
//
// This method fills the background octree with tets, but does
// not worry about making a valid mesh. It simply fills the
// volume with tets for visualization purposes.
//============================================================
TetMesh* CleaverMesherImp::createVisualizationBackgroundMesh()
{
    if(m_bgMesh)
        delete m_bgMesh;
    m_bgMesh = new TetMesh();

    // visit each child once, post-order
    // output tets to fill each hex cell
    createVisualizationTets(m_tree->root());

    // free vertex tracker list
    m_vertex_tracker.clear();

    return m_bgMesh;
}



//================================================
// - createSizingField()
//================================================
ScalarField<float>* CleaverMesherImp::createSizingField()
{
    std::cout << "Creating sizing field... " << std::flush;
    ScalarField<float> *sizingField = SizingFieldCreator::createSizingFieldFromVolume(m_volume, 1.0/0.2, 2.0f);
    std::cout << "Done." << std::endl;

    return sizingField;
}

//============================================
// - adaptCell()
//============================================
void CleaverMesherImp::adaptCell(OTCell *cell)
{
    if(!cell)
        return;


    BoundingBox domainBounds = m_volume->bounds();

    int max_x = (int)(domainBounds.maxCorner().x);
    int max_y = (int)(domainBounds.maxCorner().y);
    int max_z = (int)(domainBounds.maxCorner().z);

    // if cell is completely outside, done
    if(cell->bounds.minCorner().x >= max_x ||
       cell->bounds.minCorner().y >= max_y ||
       cell->bounds.minCorner().z >= max_z)
    {
        cell->celltype = OTCell::Outside;
    }

    // if cell completely inside, done
    else if(cell->bounds.maxCorner().x <= max_x &&
            cell->bounds.maxCorner().y <= max_y &&
            cell->bounds.maxCorner().z <= max_z)
    {
        cell->celltype = OTCell::Inside;
    }
    // otherwise it straddles
    else
        cell->celltype = OTCell::Staddles;

    BoundingBox bounds = cell->bounds;

    /*
    vec3 tx = vec3((bounds.center().x / m_volume->bounds().size.x)*m_sizingField->bounds().size.x,
                   (bounds.center().y / m_volume->bounds().size.y)*m_sizingField->bounds().size.y,
                   (bounds.center().z / m_volume->bounds().size.z)*m_sizingField->bounds().size.z);

    float LFS = m_sizingField->valueAt(tx);
    */

    double LFS = m_sizingOracle->getMinLFS(cell->xLocCode, cell->yLocCode, cell->zLocCode, cell->level);

    //
    //float min_LFS = SmallestFeatureSize(m_sizingField, cell);
    if(LFS < bounds.size.x)
        cell->subdivide();


    if(cell->hasChildren()){
        for(int i=0; i < 8; i++)
        {
            /*
            // if cell is completely outside, done
            if(cell->children[i]->bounds.minCorner().x > max_x ||
                    cell->children[i]->bounds.minCorner().y > max_y ||
                    cell->children[i]->bounds.minCorner().z > max_z)
            {
                cell->children[i]->celltype = OTCell::Outside;
            }
            */

            //else{
                adaptCell(cell->children[i]);
            //}
        }
    }
}

//======================================
// - createOctree()
//======================================
Octree* CleaverMesherImp::createOctree()
{
    // Get Bounds    
    BoundingBox bounds = m_volume->bounds();

    // Create Octree
    Octree *tree = new Octree(bounds);

    // breadth first creation
    adaptCell(tree->root());

    return tree;
}

//======================================
// - createOctreeBottomUp()
//======================================
Octree* CleaverMesherImp::createOctreeBottomUp()
{
    // Get Bounds
    BoundingBox bounds = m_volume->bounds();

    // Allocate an Octree
    Octree *tree = new Octree(bounds);

    int w = (int)(bounds.size.x);
    int h = (int)(bounds.size.y);
    int d = (int)(bounds.size.z);

    // This routine will work only for regular grid data
    for(int k=0; k < d; k++)
    {
        for(int j=0; j < h; j++)
        {
            for(int i=0; i < w; i++)
            {
                double lfs = m_sizingField->valueAt(i+0.5, j+0.5, k+0.5);

                // determine corresponding cell sizes
            }
        }
    }

    return tree;
}


vec3 CleaverMesherImp::truePosOfPos(const vec3 &pos)
{
    vec3 warp = warpForPosition(pos);
    if(warp.x >= 0 && warp.y >= 0 && warp.z >= 0)
        return warp;
    else
        return pos;
}

bool CleaverMesherImp::posWasWarped(const vec3 &pos)
{
    vec3 warp = warpForPosition(pos);
    if(warp.x >= 0 && warp.y >= 0 && warp.z >= 0)
        return true;
    else
        return false;
}

std::vector<OTCell*> CleaverMesherImp::cellsAroundPos(const vec3 &pos)
{
    std::vector<OTCell*> cells;

    // find cell this is the center of
    // if there isn't one, it's a boundary position (skip for now)


    // children all have corners touching this vertex.`



    OTCell *cell= m_tree->root();
    while(cell->hasChildren())
    {
        if(L2(cell->bounds.center() - pos) < 1E-3)
            break;

        for(int i=0; i < 8; i++){
            OTCell *child = cell->children[i];
            if(child->bounds.contains(pos))
            {
                cell = child;
                break;
            }
        }
    }

    if(!(L2(cell->bounds.center() - pos) < 1E-3)){
        //std::cout << "Failed to find the root cell for cellsAroundPos(" << pos.toString() << ")" << std::endl;
        return cells;
    }
    else
        std::cout << "Found root central cell" << std::endl;

    //-------------------------------------------
    // Now,  descend oppositely for each child
    //-------------------------------------------
    if(cell->hasChildren()){
        for(int i=0; i < 8; i++){

            OTCell *child = cell->children[i];

            while(child->hasChildren()){
                child = cell->children[7 - i];
            }

            cells.push_back(child);
        }
    }



    return cells;
}

void CleaverMesherImp::resetPosForCell(OTCell *cell)
{
    for(int i=0; i < 8; i++)
    {
        vec3 v = cell->bounds.minCorner();

        v.x += cell->bounds.size.x * CELL_VERT_COORDS[i][0];
        v.y += cell->bounds.size.y * CELL_VERT_COORDS[i][1];
        v.z += cell->bounds.size.z * CELL_VERT_COORDS[i][2];

        if(posWasWarped(v))
            m_warp_tracker.erase(v);
    }
}

void CleaverMesherImp::resetCellsAroundPos(const vec3 &pos)
{
    std::vector<OTCell*> cells = CleaverMesherImp::cellsAroundPos(pos);

    for(unsigned int i=0; i < cells.size(); i++)
    {
        resetPosForCell(cells[i]);
        cells[i]->subdivide();
    }
}

//------------------------------------------------------------------------------
// This method examines the "edge" between two positions
// If they're not safe, a warp will be attempted to fix them.
// If no warp can fix them, a FALSE is returned, indicating
// they are in an unsafe configuration, and subdivision is required
//------------------------------------------------------------------------------
bool CleaverMesherImp::checkSafetyBetween(const vec3 &v1_orig, const vec3 &v2_orig, bool v1_warpable, bool v2_warpable)
{
    double beta = 0.2f;
    bool safe = true;

    // get their true current locations
    vec3 v1 = truePosOfPos(v1_orig);
    vec3 v2 = truePosOfPos(v2_orig);

    // get their materials
    int a_mat = m_volume->maxAt(v1);
    int b_mat = m_volume->maxAt(v2);

    // if m1 = m2, there are no cuts, safe
    if(a_mat == b_mat)
        return safe;

    // get 3rd material
    int c_mat = -1;
    for(int m=0; m < m_volume->numberOfMaterials(); m++){
        if(m != a_mat && m != b_mat)
        {
            c_mat = m;
            break;
        }
    }

    // compute crossing parameter t  (a,b crossing)
    {
        double a1 = m_volume->valueAt(v1, a_mat);
        double a2 = m_volume->valueAt(v2, a_mat);
        double b1 = m_volume->valueAt(v1, b_mat);
        double b2 = m_volume->valueAt(v2, b_mat);
        double top = (a1 - b1);
        double bot = (b2 - a2 + a1 - b1);
        double t = top / bot;

        vec3   pos = v1*(1-t) + v2*(t);
        double ab  = a1*(1-t) + a2*(t);
    }

    bool ac_crossing = false;
    bool bc_crossing = false;
    double t_ac = -1;
    double t_bc = -1;

    // compute crossing parameter t_ac (a,c crossing)
    {
        double a1 = m_volume->valueAt(v1, a_mat);
        double a2 = m_volume->valueAt(v2, a_mat);
        double c1 = m_volume->valueAt(v1, c_mat);
        double c2 = m_volume->valueAt(v2, c_mat);

        // since a is definitely maximum on v1, can only
        // be a crossing if c is greater than a on v2
        if(c2 > a2)
        {
            double top = (a1 - c1);
            double bot = (c2 - a2 + a1 - c1);
            double t = top / bot;

            vec3   pos = v1*(1-t) + v2*(t);
            double ac  = a1*(1-t) + a2*(t);

            if(ac > m_volume->valueAt(pos, b_mat) && t >= 0.0 && t <= 1.0)
            {
                ac_crossing = true;
                t_ac = t;
            }
        }
    }

    // compute crossing parameter t_bc (b,c crossing)
    {
        double c1 = m_volume->valueAt(v1, c_mat);
        double c2 = m_volume->valueAt(v2, c_mat);
        double b1 = m_volume->valueAt(v1, b_mat);
        double b2 = m_volume->valueAt(v2, b_mat);

        // since b is definitely maximum on v2, can only
        // be a crossing if c is greater than b on v1
        if(c1 > b1)
        {
            double top = (c1 - b1);
            double bot = (b2 - c2 + c1 - b1);
            double t = top / bot;

            vec3   pos = v1*(1-t) + v2*(t);
            double bc  = c1*(1-t) + c2*(t);

            if(bc > m_volume->valueAt(pos, a_mat) && t >= 0.0 && t <= 1.0)
            {
                bc_crossing = true;
                t_bc = t;
            }
        }
    }

    // if 3rd material pops up, handle it
    if(ac_crossing && bc_crossing){
        safe = false;

        // both crossings fall within beta
        if(t_ac < beta && t_bc < beta)
        {
            std::cout << "both 'ac' and 'bc' within beta range " << beta << ". ";
            std::cout << "ac = " << t_ac << ", bc = " << t_bc << ", beta = " << beta << std::endl;

            if(v1_orig != v1){
                std::cout << "BUT v1 already warped, can't fix." << std::endl;
                resetCellsAroundPos(v1_orig);
            }
            else if(v1_warpable)
            {
                // warp v1 to be halfway between ac/bc, ensuring we split it
                double t_warp = 0.5*(t_ac + t_bc);
                vec3 v1_warp = v1*(1 - t_warp) + v2*(t_warp);
                setWarpForPosition(v1, v1_warp);
                safe = true;
            }
        }

        // first crossing only falls within beta
        else if(t_ac < beta && t_bc >= beta)
        {
            std::cout << "only 'ac' within beta range " << beta << std::endl;

            if(v1_orig != v1){
                std::cout << "BUT v1 already warped, can't fix." << std::endl;
                resetCellsAroundPos(v1_orig);
            }
            else if(v1_warpable)
            {
                // warp v1 to be halfway between ac/beta, ensuring good numerics
                double t_warp = 0.5*(t_ac + beta);
                vec3 v1_warp = v1*(1 - t_warp) + v2*(t_warp);
                setWarpForPosition(v1, v1_warp);
                safe = true;
            }
        }

        // both crossings fall within (1-beta)
        else if((1 - t_ac) < beta && (1 - t_bc) < beta)
        {
            std::cout << "both 'ac' and 'bc' within beta range " << 1 - beta << ".";
            std::cout << "beta = " << 1 - beta << ", ac = " << t_ac << ", bc = " << t_bc << std::endl;

            if(v2_orig != v2){
                std::cout << "BUT v2 already warped, can't fix." << std::endl;
                resetCellsAroundPos(v2_orig);
            }
            else if(v2_warpable)
            {
                // warp v2 to be halfway between ac/bc, ensuring we split it
                double t_warp = 0.5f*(t_ac + t_bc);
                vec3 v2_warp = v1*(1 - t_warp) + v2*(t_warp);
                setWarpForPosition(v2, v2_warp);
                safe = true;
            }
        }

        // second crossing only falls within beta
        else if((1 - t_ac) >= beta && (1 - t_bc) < beta)
        {
            std::cout << "only 'bc' within beta range " << 1 - beta << std::endl;

            if(v2_orig != v2){
                std::cout << "BUT v2 already warped, can't fix." << std::endl;
                resetCellsAroundPos(v2_orig);
            }
            else if(v2_warpable)
            {
                // warp v2 to be halfway between bc/(1-beta), ensuring good numerics
                double t_warp = 0.5*(t_bc + (1-beta));
                vec3 v2_warp = v1*(1 - t_warp) + v2*(t_warp);
                setWarpForPosition(v2, v2_warp);
                safe = true;
            }
        }
    }

    return safe;
}

//-----------------------------------------------
// This method takes a freshly warped call, and
// computes new warped locations for dual points,
// and face duals.
//-----------------------------------------------
void CleaverMesherImp::createWarpedVertices(OTCell *cell)
{
    //----------------------------------------------
    // Create the dual cell's central vertex first
    //----------------------------------------------
    vec3 c = cell->bounds.center();
    vec3 avg = vec3::zero;

    for(int i=0; i < 8; i++)
    {
        vec3 v = cell->bounds.minCorner();
        v.x += cell->bounds.size.x * CELL_VERT_COORDS[i][0];
        v.y += cell->bounds.size.y * CELL_VERT_COORDS[i][1];
        v.z += cell->bounds.size.z * CELL_VERT_COORDS[i][2];

        v = truePosOfPos(v);
        avg += (1.0/8.0)*v;
    }

    setWarpForPosition(c, avg);

    //----------------------------------------------------
    // Next create dual vertices on each face of the cell
    //----------------------------------------------------
    for(int f=0; f < 6; f++)
    {
        vec3 avg = vec3::zero;
        vec3 c   = vec3::zero;

        for(int e=0; e < 4; e++)
        {
            vec3 ev = cell->bounds.minCorner();
            ev.x += cell->bounds.size.x * CELL_VERT_COORDS[FACE_VERTICES[f][e]][0];
            ev.y += cell->bounds.size.y * CELL_VERT_COORDS[FACE_VERTICES[f][e]][1];
            ev.z += cell->bounds.size.z * CELL_VERT_COORDS[FACE_VERTICES[f][e]][2];

            c += (1.0/4.0)*ev;

            ev = truePosOfPos(ev);

            avg += (1.0/4.0)*ev;
        }

        setWarpForPosition(c, avg);
    }
}

//================================================
// - makeCellRespectTopology()
//
// This is the recursive method that fullfills the
// guarantees of subdivideTreeToTopology(). It is
// to be called on a leaf cell.
//================================================
void CleaverMesherImp::makeCellRespectTopology(OTCell *cell)
{
    // if there are children already, run on them
    if(cell->hasChildren())
    {
        for(int i=0; i < 8; i++)
        {
            makeCellRespectTopology(cell->children[i]);
        }

        return;
    }

    BoundingBox bounds = cell->bounds;
    bool safe = true;

    //------------------------------------------------
    // Check 12 Primal Edges for Multiple Crossings
    //------------------------------------------------
    for(int e=0; e < 12; e++)
    {
        vec3 v1 = bounds.minCorner();
        v1.x += bounds.size.x * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][0]][0];
        v1.y += bounds.size.y * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][0]][1];
        v1.z += bounds.size.z * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][0]][2];

        vec3 v2 = bounds.minCorner();
        v2.x += bounds.size.x * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][1]][0];
        v2.y += bounds.size.y * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][1]][1];
        v2.z += bounds.size.z * CELL_VERT_COORDS[CELL_EDGE_VERTS[e][1]][2];

        safe = safe && checkSafetyBetween(v1, v2);

        if(!safe)
            break;
    }


    // if not safe, split and recheck
    if(!safe)
    {
        std::cout << "Subdividing to respect topology: Level " << cell->level << std::endl;
        //if(cell->level == 3)
        //    return;
        cell->subdivide();

        createWarpedVertices(cell);

        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
            {
                makeCellRespectTopology(cell->children[i]);
            }
        }

        return;
    }

    //---------------------------------
    // Now Check the 8 Diagonal Edges
    //---------------------------------
    vec3 v1 = bounds.center();

    for(int i=0; i < 8; i++)
    {
        vec3 v2 = bounds.minCorner();
        v2.x += bounds.size.x * CELL_VERT_COORDS[i][0];
        v2.y += bounds.size.y * CELL_VERT_COORDS[i][1];
        v2.z += bounds.size.z * CELL_VERT_COORDS[i][2];

        safe = safe && checkSafetyBetween(v1, v2);

        if(!safe)
            break;
    }

    // if not safe, split and recheck
    if(!safe)
    {
        std::cout << "Subdividing to respect topology: Level " << cell->level << " for diagonal" << std::endl;
        cell->subdivide();

        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
            {
                makeCellRespectTopology(cell->children[i]);
            }
        }

        return;
    }


    //----------------------------------------------
    //  Now check the 6 half-duals
    //----------------------------------------------
    for(int f=0; f < 6; f++)
    {
        OTCell *neighbor = m_tree->getNeighborAtLevel(cell, f, cell->level);
        if(!(neighbor && (neighbor->level == cell->level || neighbor->level == cell->level-1)))
            continue;

        vec3 vn = neighbor->bounds.center();
        vec3 v2 = 0.5f*(v1 + vn);

        safe = safe && checkSafetyBetween(v1, v2); //, true, false);
    }

    // if not safe, split and recheck
    if(!safe)
    {
        std::cout << "Subdividing to respect topology: Level " << cell->level << "  DUAL EDGE!! " << std::endl;
        cell->subdivide();

        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
            {
                makeCellRespectTopology(cell->children[i]);
            }
        }

        return;
    }

    //-----------------------------------------------
    // Now check the 24 face dual edges (4 per face)
    //-----------------------------------------------
    for(int f=0; f < 6; f++)
    {
        OTCell *neighbor = m_tree->getNeighborAtLevel(cell, f, cell->level);
        if(!(neighbor && (neighbor->level == cell->level || neighbor->level == cell->level-1)))
            continue;

        vec3 c1 = cell->bounds.center();
        vec3 c2 = neighbor->bounds.center();
        vec3 v1 = 0.5f*(c1 + c2);

        // grab v2 for each of the 4 face edges
        for(int e=0; e < 4; e++)
        {
            vec3 ev1 = bounds.minCorner();
            ev1.x += bounds.size.x * CELL_VERT_COORDS[FACE_VERTICES[f][(e+0)%4]][0];
            ev1.y += bounds.size.y * CELL_VERT_COORDS[FACE_VERTICES[f][(e+0)%4]][1];
            ev1.z += bounds.size.z * CELL_VERT_COORDS[FACE_VERTICES[f][(e+0)%4]][2];

            vec3 ev2 = bounds.minCorner();
            ev2.x += bounds.size.x * CELL_VERT_COORDS[FACE_VERTICES[f][(e+1)%4]][0];
            ev2.y += bounds.size.y * CELL_VERT_COORDS[FACE_VERTICES[f][(e+1)%4]][1];
            ev2.z += bounds.size.z * CELL_VERT_COORDS[FACE_VERTICES[f][(e+1)%4]][2];

            vec3 v2 = 0.5f*(ev1 + ev2);

            safe = safe && checkSafetyBetween(v1, v2);

            if(!safe)
                break;
        }

        // then grab v2 for each of the 4 face diagonals
        for(int v=0; v < 4; v++)
        {

        }
    }

    // if not safe, split and recheck
    if(!safe)
    {
        std::cout << "Subdividing to respect topology: Level " << cell->level << " face dual edge.. " << std::endl;
        cell->subdivide();

        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
            {
                makeCellRespectTopology(cell->children[i]);
            }
        }

        return;
    }


}

//================================================
// - subdivideTreeToTopology()
//
// This method examines material values on verts
// of all cells and decides if the BCC cell can
// accurately represent the topology with our
// stencils. If it cannot be guaranteed to, we
// subdivide.
//================================================
void CleaverMesherImp::subdivideTreeToTopology()
{
    // naivly check EVERY LEAF CELL
    // ideally we would only check the smallest leaves

    std::vector<OTCell*> leaves = m_tree->getAllLeaves();

    for(unsigned int i=0; i < leaves.size(); i++)
    {
        makeCellRespectTopology(leaves[i]);
    }
}

//================================================
// - conformOctreeToDomain()
//
//  This method refines the octree so that output
// tets conform precisely to the boundary domain.
//================================================
void CleaverMesherImp::conformOctreeToDomain()
{
    conformCellToDomain(m_tree->root());
}

void CleaverMesherImp::conformCellToDomain(OTCell *cell)
{
    BoundingBox domainBounds = m_volume->bounds();

    int max_x = (int)(domainBounds.maxCorner().x);
    int max_y = (int)(domainBounds.maxCorner().y);
    int max_z = (int)(domainBounds.maxCorner().z);

    // if cell is completely outside, done
    if(cell->bounds.minCorner().x >= max_x ||
       cell->bounds.minCorner().y >= max_y ||
       cell->bounds.minCorner().z >= max_z)
    {
        cell->celltype = OTCell::Outside;
    }

    // if cell completely inside, done
    else if(cell->bounds.maxCorner().x <= max_x &&
            cell->bounds.maxCorner().y <= max_y &&
            cell->bounds.maxCorner().z <= max_z)
    {
        cell->celltype = OTCell::Inside;
    }
    // otherwise it straddles, we need to subdivide
    else
    {
        cell->celltype = OTCell::Staddles;

        if(!cell->hasChildren()){
            cell->subdivide();
        }

        if(cell->hasChildren()){
            for(int i=0; i < 8; i++){
                conformCellToDomain(cell->children[i]);
            }
        }
    }
}

int heightPairs[18] =
{
    1, //[0] -x to +x
    0, //[1] +x to -x
    3, //[2] -y to +y
    2, //[3] +y to -y
    5, //[4] -z to +z
    4, //[5] +z to -z
    9, //[6] bottom left to upper right
    8, //[7] bottom right to upper left
    7, //[8] upper left to bottom right
    6, //[9] upper right to bottom left
    13,//[10] back left to front right
    12,//[11] back right to front left
    11,//[12] front left to back right
    10,//[13] front right to back left
    17,//[14] bottom back to upper front
    16,//[15] upper back to bottom front
    15,//[16] bottom front to upper back
    14,//[17] upper front to bottom back
};

int heightPaths[18][4] =
{
    {_000,_010,_100,_110}, // -x       0
    {_001,_011,_101,_111}, // +x       1
    {_000,_001,_100,_101}, // -y       2
    {_010,_011,_110,_111}, // +y       3
    {_000,_010,_001,_011}, // -z       4
    {_100,_110,_101,_111}, // +z       5

    {_000,_100,_000,_100}, // -x,-y   // bottom left    6
    {_001,_101,_001,_101}, // +x,-y   // bottom right   7
    {_010,_110,_010,_110}, // -x,+y   // upper left     8
    {_011,_111,_011,_111}, // +x,+y   // upper right    9

    {_000,_010,_000,_010}, // -x,-z   // back left     10
    {_001,_011,_001,_011}, // +x,-z   // back right    11
    {_100,_110,_100,_110}, // -x,+z   // front left    12
    {_101,_111,_101,_111}, // +x,+z   // front right   13

    {_000,_001,_000,_001}, // -y,-z   // bottom back   14
    {_010,_011,_010,_011}, // +y,-z   // upper back    15
    {_100,_101,_100,_101}, // -y,+z   // bottom front  16
    {_110,_111,_110,_111}, // +y,+z   // upper front   17
};


//====================================================
// - heightForPath()
//====================================================
int heightForPath(OTCell *cell, int path, int depth=0)
{
    int height = 1;
    depth++;
    if(depth == 3)
        return height;

    if(cell->children[0]){

        int max_height = 0;
        for(int i=0; i < 4; i++)
            max_height = std::max(max_height, heightForPath(cell->children[heightPaths[path][i]], path, depth));

        height += max_height;
    }

    return height;
}

//==============================
// - computeDepths()
// compute max level depths,
// post-order
//==============================
void computeDepths(OTCell *cell)
{
    // no children, termination case
    if(!cell->children[0])
    {
        for(int i=0; i < 8; i++)
            cell->depths[i] = cell->level;
        return;
    }

    // compute depths for children
    for(int i=0; i < 8; i++)
        computeDepths(cell->children[i]);

    // X - left (-) sides compute
    cell->depths[0] = cell->level;
    cell->depths[0] = std::min(cell->depths[0], cell->children[_000]->depths[0]);
    cell->depths[0] = std::min(cell->depths[0], cell->children[_010]->depths[0]);
    cell->depths[0] = std::min(cell->depths[0], cell->children[_100]->depths[0]);
    cell->depths[0] = std::min(cell->depths[0], cell->children[_110]->depths[0]);

    // X - right (+) sides compute
    cell->depths[1] = cell->level;
    cell->depths[1] = std::min(cell->depths[1], cell->children[_001]->depths[1]);
    cell->depths[1] = std::min(cell->depths[1], cell->children[_011]->depths[1]);
    cell->depths[1] = std::min(cell->depths[1], cell->children[_101]->depths[1]);
    cell->depths[1] = std::min(cell->depths[1], cell->children[_111]->depths[1]);

    // Y - left (-) sides compute
    cell->depths[2] = cell->level;
    cell->depths[2] = std::min(cell->depths[2], cell->children[_000]->depths[2]);
    cell->depths[2] = std::min(cell->depths[2], cell->children[_001]->depths[2]);
    cell->depths[2] = std::min(cell->depths[2], cell->children[_100]->depths[2]);
    cell->depths[2] = std::min(cell->depths[2], cell->children[_101]->depths[2]);

    // Y - right (+) sides compute
    cell->depths[3] = cell->level;
    cell->depths[3] = std::min(cell->depths[3], cell->children[_010]->depths[3]);
    cell->depths[3] = std::min(cell->depths[3], cell->children[_011]->depths[3]);
    cell->depths[3] = std::min(cell->depths[3], cell->children[_110]->depths[3]);
    cell->depths[3] = std::min(cell->depths[3], cell->children[_111]->depths[3]);

    // Z - left (-) sides compute
    cell->depths[4] = cell->level;
    cell->depths[4] = std::min(cell->depths[4], cell->children[_000]->depths[4]);
    cell->depths[4] = std::min(cell->depths[4], cell->children[_010]->depths[4]);
    cell->depths[4] = std::min(cell->depths[4], cell->children[_001]->depths[4]);
    cell->depths[4] = std::min(cell->depths[4], cell->children[_011]->depths[4]);

    // Z - right (+) sides compute
    cell->depths[5] = cell->level;
    cell->depths[5] = std::min(cell->depths[5], cell->children[_100]->depths[5]);
    cell->depths[5] = std::min(cell->depths[5], cell->children[_110]->depths[5]);
    cell->depths[5] = std::min(cell->depths[5], cell->children[_101]->depths[5]);
    cell->depths[5] = std::min(cell->depths[5], cell->children[_111]->depths[5]);

    //    std::cout << "Depths computed as: ";
    //    for(int i=0; i < 6; i++)
    //        std::cout << cell->depths[i] << " ";
    //    std::cout << std::endl;
}

//======================================================
// - altBalanceOctree()
// An alternative approach to balancing the octree
//======================================================
void CleaverMesherImp::altBalanceOctree()
{
    // start at the bottom, move up
    int maxCode = m_tree->getMaximumCode();
    for(int i=0; i < m_tree->getNumberofLevels(); i++)
    {
        std::list<OTCell*> nodes = m_tree->collectChildrenAtLevel(m_tree->root(), i);
        std::list<OTCell*>::iterator n_iter = nodes.begin();

        // look at each node at this level
        while (n_iter != nodes.end())
        {
            OTCell *cell = *n_iter;

            // examine each neighbor but corners
            for(int x_dir=-1; x_dir <= 1; x_dir++)
            {
                for(int y_dir=-1; y_dir <= 1; y_dir++)
                {
                    for(int z_dir=-1; z_dir <= 1; z_dir++)
                    {
                        // skip corners
                        if(abs(x_dir) + abs(y_dir) + abs(z_dir) == 3)
                            continue;

                        // skip self
                        if(abs(x_dir) + abs(y_dir) + abs(z_dir) == 0)
                            continue;

                        unsigned int shift = 1 << cell->level;

                        unsigned int cur_x = cell->xLocCode;
                        unsigned int cur_y = cell->yLocCode;
                        unsigned int cur_z = cell->zLocCode;

                        unsigned int nbr_x = cur_x + shift*x_dir;
                        unsigned int nbr_y = cur_y + shift*y_dir;
                        unsigned int nbr_z = cur_z + shift*z_dir;

                        // if outside, move on
                        //if(nbr_x < 0 || nbr_y < 0 || nbr_z < 0)  // (JRB: nbr is unsigned, this expression is always false)
                        //    continue;
                        if((int)nbr_x >= maxCode || (int)nbr_y >= maxCode || (int)nbr_z >= maxCode)
                            continue;

                        // otherwise add neighbor parent
                        m_tree->addCellAtLevel(nbr_x, nbr_y, nbr_z, cell->level+1);
                    }
                }

            }

            n_iter++;
        }
    }   
}

//======================================================
// - balanceOctreeNew()
//======================================================
void CleaverMesherImp::balanceOctreeNew()
{
    // first create reverse breadth first list of leaves
    std::queue<OTCell*> q;
    std::stack<OTCell*> s;

    q.push(m_tree->root());

    while(!q.empty())
    {
        OTCell *cell = q.front();

        // ignore bottom leaves, they can't split
        if(cell->level == 0)
        {
            q.pop();
            continue;
        }

        // if there are children, enqueue them
        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
                q.push(cell->children[i]);
        }
        // else put this leaf onto stack
        else
            s.push(cell);

        // done with this node
        q.pop();
    }

    // now have reverse breadth first list of leaves
    while(!s.empty())
    {
        OTCell *cell = s.top();

        // ignore bottom leaves, they can't split
        if(cell->level == 0)
        {
            s.pop();
            continue;
        }

        // if no children check, if branch needed        
        if(!cell->hasChildren())
        {
            // look in all directions, excluding diagonals (need to subdivide?)
            for(int i=0; i < 18; i++)
            {
                OTCell *neighbor = m_tree->getNeighborAtLevel(cell, i, cell->level);

                if(neighbor && heightForPath(neighbor, heightPairs[i]) > 2)
                {
                    cell->subdivide();                    
                    break;
                }
            }
        }

        // done with this node
        s.pop();

        // if there are children now, push them on stack
        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
                s.push(cell->children[i]);
        }
    }

}

//===================================================
// - balanceOctree()
//===================================================
void CleaverMesherImp::balanceOctree()
{
    // first recursively compute - and + max depths  (post order)
    computeDepths(m_tree->root());

    // then balance each branch (breadth first)
    std::queue<OTCell*> q;

    q.push(m_tree->root());
    while(!q.empty())
    {
        OTCell *cell = q.front();

        if(cell->level == 0)
        {
            q.pop();
            continue;
        }

        // if no children check if branch needed
        if(!cell->hasChildren())
        {            
            // look left on each axis (need to subdivide?)
            for(int i=0; i < 6; i+=2)
            {
                OTCell *neighbor = m_tree->getNeighborAtLevel(cell, i, cell->level);

                if(neighbor && (cell->level > neighbor->depths[i+1]) && (cell->level - neighbor->depths[i+1] > 1))
                {
                    cell->subdivide();
                    //computeDepths(cell);
                      computeDepths(m_tree->root());                    
                    break;
                }
            }
        }

        if(!cell->hasChildren())
        {
            // look right on each axis (need to subdivide?)
            for(int i=1; i < 6; i+=2)
            {
                OTCell *neighbor = m_tree->getNeighborAtLevel(cell, i, cell->level);

                if(neighbor && (cell->level > neighbor->depths[i-1]) && (cell->level - neighbor->depths[i-1] > 1))
                {
                    cell->subdivide();
                    //computeDepths(cell);
                      computeDepths(m_tree->root());                    
                    break;
                }
            }
        }

        // if there are children, enqueue them
        if(cell->hasChildren())
        {
            for(int i=0; i < 8; i++)
                q.push(cell->children[i]);
        }

        // done with this node
        q.pop();
    }

}

//==============================
// - getTree()
//==============================
Octree* CleaverMesher::getTree() const
{
    return m_pimpl->m_tree;
}

//========================================
// - createBackgroundMesh()
//========================================
void CleaverMesher::createBackgroundMesh()
{
    cleaver::Timer timer;
    timer.start();
    m_pimpl->createBackgroundMesh();
    timer.stop();
    setBackgroundTime(timer.time());
}

void CleaverMesher::setBackgroundMesh(TetMesh *m)
{
    m_pimpl->setBackgroundMesh(m);
}

//==================================
// - buildAdjacency();
//==================================
void CleaverMesher::buildAdjacency()
{
    m_pimpl->buildAdjacency(true);
}

//================================
// - SampleVolume()
//================================
void CleaverMesher::sampleVolume()
{
    m_pimpl->sampleVolume(true);
}

//=================================
// - computeInterfaces()
//=================================
void CleaverMesher::computeAlphas()
{
    m_pimpl->computeAlphas(true);
}

//=====================================
// - computeInterfaces()
//=====================================
void CleaverMesher::computeInterfaces()
{
    m_pimpl->computeInterfaces(true);
}

//=====================================
// - generalizeTets()
//=====================================
void CleaverMesher::generalizeTets()
{
    m_pimpl->generalizeTets(true);
}

//================================
// - snapAndWarp()
//================================
void CleaverMesher::snapsAndWarp()
{
    m_pimpl->snapAndWarpViolations(true);
}

//===============================
// - stencilTets()
//===============================
void CleaverMesher::stencilTets()
{
    m_pimpl->stencilBackgroundTets(true);
}



//=================================================
// - buildAdjacency()
//=================================================
void CleaverMesherImp::buildAdjacency(bool verbose)
{
    if(verbose)
        std::cout << "Building Adjacency..." << std::flush;

    m_bgMesh->constructFaces();
    m_bgMesh->constructBottomUpIncidences(verbose);

    // set state
    m_bAdjacencyBuilt = true;

    if(verbose)
        std::cout << " done." << std::endl;
}

//=================================================
// - sampleVolume()
//=================================================
void CleaverMesherImp::sampleVolume(bool verbose)
{
    if(verbose)
        std::cout << "Sampling Volume..." << std::flush;

    // Sample Each Background Vertex
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        // Get Vertex
        cleaver::Vertex *vertex = m_bgMesh->verts[v];

        // Grab Material Label
        vertex->label = m_volume->maxAt(vertex->pos());        

        // added feb 20 to attempt boundary conforming
        if(!m_volume->bounds().contains(vertex->pos())){
            vertex->isExterior = true;
            vertex->label = m_volume->numberOfMaterials();
        }
        else{
            vertex->isExterior = false;
        }
    }

    m_bgMesh->material_count = m_volume->numberOfMaterials();

    // set state
    m_bSamplingDone = true;

    if(verbose)
        std::cout << " done." << std::endl;
}

//double alpha_init = 0.4f;  // was 0.2

//================================================
// - computeAlphas()
//================================================
void CleaverMesherImp::computeAlphas(bool verbose)
{
    if(verbose)
        std::cout << "Computing Violation Alphas..." << std::flush;

    // set state
    m_bAlphasComputed = true;


    //---------------------------------------------------
    // set alpha_init for all edges in background mesh
    //---------------------------------------------------
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator edge_iter;

    for(edge_iter = m_bgMesh->halfEdges.begin(); edge_iter != m_bgMesh->halfEdges.end(); edge_iter++)
    {
        HalfEdge *half_edge = (*edge_iter).second;
        half_edge->alpha = (float)m_alpha_init;
        half_edge->alpha_length = (float)(half_edge->alpha*length(half_edge->vertex->pos() - half_edge->mate->vertex->pos()));
    }

    //---------------------------------------------------
    // make alphas consistent around each vertex (method 0)
    //---------------------------------------------------
    /*
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        Vertex *vertex = m_bgMesh->verts[v];

        std::vector<HalfEdge*> adjEdges = m_bgMesh->edgesAroundVertex(vertex);
        std::vector<vec3>      viol_points;
        std::vector<Plane>     viol_planes;

        // loop over adjacent edges and get violation point;

        for(int e=0; e < adjEdges.size(); e++)
        {
            // get edge
            HalfEdge *edge = adjEdges[e];

            // make sure it points out
            if(edge->vertex == vertex)
                edge = edge->mate;

            // compute violation point
            vec3 origin = vertex->pos();
            vec3 ray = edge->vertex->pos() - origin;
            vec3 viol_pt = origin + alpha_init*ray;

            // create plane from point and line(normal)
            Plane plane(viol_pt, ray);

            viol_points.push_back(viol_pt);
            viol_planes.push_back(plane);
        }

            //if(viol_point_list.size() < 1000){
            //    viol_point_list.push_back(e1_viol_pt);
            //    viol_plane_list.push_back(plane);
            //}

        for(int e1=0; e1 < adjEdges.size(); e1++)
        {
            vec3 e1_viol_pt = viol_points[e1];
            Plane plane = viol_planes[e1];

           // clip other violations to within this plane
            for(int e2=0; e2 < adjEdges.size(); e2++)
            {
                // skip self
                if(e1 == e2)
                    continue;

                vec3 e2_viol_pt = viol_points[e2];

                // if vertex and e2_viol_pt are not on same side of plane
                // then neighbor edge violation must be clamped.
                float s1 = plane.n.dot(vertex->pos()) + plane.d;
                float s2 = plane.n.dot(e2_viol_pt)    + plane.d;
                if(s1*s1 < 0)
                {
                    std::cout << "need to clip an alpha" << std::endl;
                }
            }
        }
    }
    */

    //---------------------------------------------------
    // make alphas consistent around each vertex.
    // (method 1) : Use min alpha (alpha ball)
    //---------------------------------------------------
    /*
    double max_alpha_ratio_change = 0;
    double max_alpha_absol_change = 0;
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        Vertex *vertex = m_bgMesh->verts[v];

        std::vector<HalfEdge*> adjEdges = m_bgMesh->edgesAroundVertex(vertex);

        // obtain minimum length
        float min_length = 0;
        for(int e=0; e < adjEdges.size(); e++)
        {
            // get edge
            HalfEdge *edge = adjEdges[e];

            // make sure it points out
            if(edge->vertex == vertex)
                edge = edge->mate;

            // compute violation point
            Cleaver::vec3 origin = vertex->pos();
            Cleaver::vec3 ray = edge->vertex->pos() - origin;

            float viol_dist = m_alpha_init * length(ray);

            if(viol_dist < min_length || e==0)
            {
                min_length = viol_dist;
            }
        }

        // scale other adjacents to match min length
        for(int e=0; e < adjEdges.size(); e++)
        {
            // get edge
            HalfEdge *edge = adjEdges[e];

            // make sure it points out
            if(edge->vertex == vertex)
                edge = edge->mate;

            // compute violation point
            vec3 origin = vertex->pos();
            vec3 ray = edge->vertex->pos() - origin;

            float alpha = min_length / length(ray);
            edge->alpha = alpha;
            edge->alpha_length = min_length;

            float old_dist = m_alpha_init * length(ray);
            float new_dist = alpha      * length(ray);

            float ratio_change = 1.0 - (new_dist / old_dist);
            float absol_change = new_dist - old_dist;

            if(ratio_change > max_alpha_ratio_change)
                max_alpha_ratio_change = ratio_change;
            if(std::abs(absol_change) > std::abs(max_alpha_absol_change))
                max_alpha_absol_change = absol_change;
        }
    }

    std::cout << std::endl;
    std::cout << "Max alpha reduction: " << max_alpha_ratio_change*100 << "%" << std::endl;
    std::cout << "Max dist  reduction: " << max_alpha_absol_change << std::endl;

    std::cout << "Following up with Altitude Fix" << std::endl;
    computeAlphasAlt();
    */

    if(verbose)
        std::cout << " done." << std::endl;
}

void CleaverMesherImp::computeAlphasAlt(bool verbose)
{
    // visit each tet
    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        Tet *tet = m_bgMesh->tets[t];

        // look at all 4 altitudes
        for(unsigned int vidx=0; vidx < VERTS_PER_TET; vidx++)
        {
            Vertex *verts[4];
            verts[0] = tet->verts[(vidx+0)%4];
            verts[1] = tet->verts[(vidx+1)%4];
            verts[2] = tet->verts[(vidx+2)%4];
            verts[3] = tet->verts[(vidx+3)%4];

            vec3 v0 = verts[0]->pos();
            vec3 v1 = verts[1]->pos();
            vec3 v2 = verts[2]->pos();
            vec3 v3 = verts[3]->pos();

            // construct plane from opposite vertices
            Plane plane = Plane::throughPoints(v1,v2,v3);
            vec3 n = plane.n;

            // q_proj = q - dot(q - p, n) * n;
            vec3 proj = v0 - dot(v0 - v1, n) * n;

            // fix normal orientation if necessary..
            float alpha_length[4];
            alpha_length[0] = verts[0]->halfEdges[0]->alpha_length;
            alpha_length[1] = verts[1]->halfEdges[0]->alpha_length;
            alpha_length[2] = verts[2]->halfEdges[0]->alpha_length;
            alpha_length[3] = verts[3]->halfEdges[0]->alpha_length;

            // construct altitude vector
            vec3 alt = proj - v0;
            float req_length = (float)(m_alpha_init*length(alt));

            // if alphaballs are too large, restrict alpha lengths for adj edges
            for(int v=0; v < 4; v++)
            {
                if(alpha_length[v] > req_length)
                {
                    std::vector<HalfEdge*> adjEdges = m_bgMesh->edgesAroundVertex(verts[v]);
                    // scale other adjacents to match length
                    for(size_t e=0; e < adjEdges.size(); e++)
                    {
                        // get edge
                        HalfEdge *edge = adjEdges[e];
                        edge->alpha_length = req_length;

                        // make sure it points out
                        if(edge->vertex == verts[3])  // TODO(JRB): Evaluate if this works properly...
                            edge = edge->mate;

                        // compute violation point
                        vec3 origin = verts[v]->pos();
                        vec3 ray = edge->vertex->pos() - origin;

                        float alpha = (float)(req_length / length(ray));
                        edge->alpha = alpha;
                        edge->alpha_length = req_length;
                    }
                }
            }
        }
    }

    //-----------------------------------------------
    // Test If Vertex Ball Intrudes along Altitude
    //-----------------------------------------------

    // 1. build plane for altitude.
    //    . formed from 3 pts, 1 on each alpha ball
    //    . of the other three vertices. these pts
    //    are the points on the balls closest to the
    //    current vertex under consideration.

    //vec3 v0;       // current vertex
    //vec3 v1,v2,v3; // opposite vertices;

    //double v1_alpha, v2_alpha, v3_alpha;

    // get three points on the cutting plane
    //vec3 p1 = v1_alpha*v0 + (1 - v1_alpha)*v1;
    //vec3 p2 = v2_alpha*v0 + (1 - v1_alpha)*v2;
    //vec3 p3 = v3_alpha*v0 + (1 - v1_alpha)*v3;

    // p0 is found by dropping an altitude down onto the
    // cutting plane, and moving a distance v0_alpha;

    // construct cutting plane from points
    // Plane cuttingPlane = Plane::throughPoints(p1, p2, p3);
    //Plane vertexPlane  = Plane(p0, cuttingPlane.n);
}

//=====================================================
// - computeInterfaces()
// This method iterates over edges of the background
// mesh and computes cuts for any two verts that dont'
// share a label.
//=====================================================
void CleaverMesherImp::computeInterfaces(bool verbose)
{
    int cut_count = 0;
    int triple_count = 0;
    int quadruple_count = 0;

    if(verbose)
        std::cout << "Computing Cuts..." << std::flush;

    {// DEBUG TEST
        std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
        while(iter != m_bgMesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*iter).second;
            edge->evaluated = false;
            iter++;
        }
    }

    //---------------------------------------
    //  Compute Cuts One Edge At A Time
    //---------------------------------------
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
    while(iter != m_bgMesh->halfEdges.end())
    {
        cleaver::HalfEdge *edge = (*iter).second;

        if(!edge->evaluated){
            computeCutForEdge(edge);
            //computeCutForEdge(edge);
            if(edge->cut)
                cut_count++;
        }

        iter++;
    }

    if(verbose){
         std::cout << " done. [" << cut_count << "]" << std::endl;
         std::cout << "Computing Triples..." << std::flush;
    }

    {// DEBUG TEST
        for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
        {
            cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];
            face->evaluated = false;
        }
    }

    //--------------------------------------
    // Compute Triples One Face At A Time
    //--------------------------------------
    for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
    {
        cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

        if(!face->evaluated){
            computeTripleForFace(face);
            if(face->triple)
                triple_count++;
        }
    }

    if(verbose){
         std::cout << " done. [" << triple_count << "]" << std::endl;
         std::cout << "Computing Quadruples..." << std::flush;
    }

    //-------------------------------------
    // Compute Quadruples One Tet At A Time
    //-------------------------------------
    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        cleaver::Tet *tet = m_bgMesh->tets[t];

        //if(!tet->evaluated){
            computeQuadrupleForTet(tet);
            if(tet->quadruple)
                quadruple_count++;
       // }
    }

    if(verbose)
         std::cout << " done. [" << quadruple_count << "]" << std::endl;

    // set state
    m_bInterfacesComputed = true;


}

//=============================================
// Compute the Interpolating Cubic Lagrange
// Polynomial for the 4 Given 3D Points.
//=============================================
void CleaverMesherImp::computeLagrangePolynomial(const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4, double coefficients[4])
{
    //--------------------------------------------------------------------------------
    // L_k = PRODUCT_From[i=0,i!=k]_To[n]      (x - x_i) / (x_k - x_i)
    //
    // Cubic has special form:
    //
    // L_k = x^3 - a1.x^2 - a2.x^2 - a3.x^2 + a1.a2.x + a1.a3.x + a2.a3.x - a1.a2.a3
    //
    // Grouping terms to the form
    // L_k = a.x^3 + b.x^2 + c.x + d
    //   a = 1
    //   b = -a1 -a2 - a3
    //   c = a1*a2 + a2*a3 + a3*a1
    //   d = -a1*a2*a3
    //  divisor term being (x_k - a1)(x_k - a2)(x_k - a3)
    //
    // The final polynomial can be computed in terms of coefficients.
    //--------------------------------------------------------------------------------

    std::vector<vec3> p;
    p.push_back(p1);
    p.push_back(p2);
    p.push_back(p3);
    p.push_back(p4);


    double L[4][4];
    double LX[4][3];
    //double bot[4];  // unreferenced


    for(int k=0; k < 4; k++)
    {
        // initialize values to 1
        L[k][0] = L[k][1] = L[k][2] = L[k][3] = 1;

        int idx=0;
        for(int i=0; i < 4; i++)
        {
            // skip i=k case
            if(i==k)
                continue;

            // grab the appropriate X values for each basis
            LX[k][idx++] = p[i].x;
        }

        /*a*/ L[k][0] = 1;
        /*b*/ L[k][1] = - LX[k][0] - LX[k][1] - LX[k][2] ;
        /*c*/ L[k][2] = LX[k][0]*LX[k][1] + LX[k][1]*LX[k][2] + LX[k][2]*LX[k][0];
        /*d*/ L[k][3] = -1*(LX[k][0] * LX[k][1] * LX[k][2]);

        double bot = (p[k].x - LX[k][0])*(p[k].x - LX[k][1])*(p[k].x - LX[k][2]);

        L[k][0] /= bot;
        L[k][1] /= bot;
        L[k][2] /= bot;
        L[k][3] /= bot;
    }

    //-------------------------------
    // Add basis functions together
    //-------------------------------
    double a = p1.y*L[0][0] + p2.y*L[1][0] + p3.y*L[2][0] + p4.y*L[3][0];
    double b = p1.y*L[0][1] + p2.y*L[1][1] + p3.y*L[2][1] + p4.y*L[3][1];
    double c = p1.y*L[0][2] + p2.y*L[1][2] + p3.y*L[2][2] + p4.y*L[3][2];
    double d = p1.y*L[0][3] + p2.y*L[1][3] + p3.y*L[2][3] + p4.y*L[3][3];

    coefficients[0] = d;
    coefficients[1] = c;
    coefficients[2] = b;
    coefficients[3] = a;

    //----------------------------------------------------------------------------
    // now do a sanity check, compute the 4 values with the interpolating spline
    //----------------------------------------------------------------------------
    /*
    for(int i=0; i < 4; i++)
    {
        double x = p[i].x;
        double xx = x*x;
        double xxx = x*xx;

        double y = a*xxx + b*xx + c*x + d;

        if(std::abs(y - p[i].y) > 1E-5){
            std::cerr << "Interpolation FAILED: " << y << " != " << p[i].y << std::endl;
        }
        else if(y != y)
            std::cerr << "WOOPS!" << std::endl;
        //else
        //    std::cerr << y << " == " << p[i].y << std::endl;
    }
    */
}

//=====================================================
// - computeInterfaces()
// This method iterates over edges of the background
// mesh and computes topological cuts for any edges
//=====================================================
void CleaverMesherImp::computeTopologicalInterfaces(bool verbose)
{
    int cut_count = 0;
    int triple_count = 0;
    int quadruple_count = 0;  // don't compute these

    if(verbose)
        std::cout << "Computing Topological Cuts..." << std::flush;

    {// DEBUG TEST
        std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
        while(iter != m_bgMesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*iter).second;
            edge->evaluated = false;
            edge->mate->evaluated = false;
            iter++;
        }
    }

    //----------------------------------------------
    //  Compute Topological Cuts One Edge At A Time
    //----------------------------------------------
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
    while(iter != m_bgMesh->halfEdges.end())
    {
        cleaver::HalfEdge *edge = (*iter).second;

        if(!edge->evaluated){
            //computeTopologicalCutForEdge(edge);
            computeTopologicalCutForEdge2(edge);
            if(edge->cut)
                cut_count++;
        }

        iter++;
    }


    if(verbose){
         std::cout << " done. [" << cut_count << "]" << std::endl;
         //std::cout << "Computing Topological Triples..." << std::flush;
    }

    {// DEBUG TEST
        for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
        {
            cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];
            face->evaluated = false;
        }
    }

    //--------------------------------------
    // Compute Triples One Face At A Time
    //--------------------------------------
    for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
    {
        cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

        if(!face->evaluated){
            //computeTopologicalTripleForFace(face);
            if(face->triple)
                triple_count++;
        }
    }

    if(verbose){
         std::cout << " done. [" << triple_count << "]" << std::endl;
         std::cout << "Computing Quadruples..." << std::flush;
    }


    //-------------------------------------
    // Compute Quadruples One Tet At A Time
    //-------------------------------------    
    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        cleaver::Tet *tet = m_bgMesh->tets[t];

        //if(!tet->evaluated){
            computeTopologicalQuadrupleForTet(tet);
            if(tet->quadruple)
                quadruple_count++;
       // }
    }


    if(verbose)
         std::cout << " done. [" << quadruple_count << "]" << std::endl;

    // set state
    m_bInterfacesComputed = true;

}

//===============================================
// - computeCutForEdge()
//===============================================
void CleaverMesherImp::computeCutForEdge(HalfEdge *edge)
{
    // order verts
    Vertex *v2 = edge->vertex;
    Vertex *v1 = edge->mate->vertex;

    // set as evaluated
    edge->evaluated = true;
    edge->mate->evaluated = true;

    // do labels differ?
    if(v1->label == v2->label)
        return;

    // added feb 20 to attempt boundary conforming
    if((v1->isExterior && !v2->isExterior) || (!v1->isExterior && v2->isExterior))
    {
        // place a cut exactly on boundary.
        // for now, put it exactly halfway, this is wrong, but it's just a test
        vec3 a,b;
        if(v1->isExterior)
        {
            a = v2->pos();
            b = v1->pos();
        }
        else{
            a = v1->pos();
            b = v2->pos();
        }

        double t = 1000;
        Vertex *cut = new Vertex(m_volume->numberOfMaterials());

        if(b.x > m_volume->bounds().maxCorner().x)
        {
            double tt = (m_volume->bounds().maxCorner().x - a.x) / (b.x - a.x);
            if( tt < t)
                t = tt;
        }
        else if(b.x < m_volume->bounds().minCorner().x)
        {
            double tt = (m_volume->bounds().minCorner().x - a.x) / (b.x - a.x);
            if(tt < t)
                t = tt;
        }

        if(b.y > m_volume->bounds().maxCorner().y)
        {
            double tt = (m_volume->bounds().maxCorner().y - a.y) / (b.y - a.y);
            if( tt < t)
                t = tt;
        }
        else if(b.y < m_volume->bounds().minCorner().y)
        {
            double tt = (m_volume->bounds().minCorner().y - a.y) / (b.y - a.y);
            if( tt < t)
                t = tt;
        }

        if(b.z > m_volume->bounds().maxCorner().z)
        {
            double tt = (m_volume->bounds().maxCorner().z - a.z) / (b.z - a.z);
            if( tt < t)
                t = tt;
        }
        else if(b.z < m_volume->bounds().minCorner().z)
        {
            double tt = (m_volume->bounds().minCorner().z - a.z) / (b.z - a.z);
            if( tt < t)
                t = tt;
        }

        // now use t to compute real point
        if(v1->isExterior)
            cut->pos() = v2->pos()*(1-t) + v1->pos()*t;
        else
            cut->pos() = v1->pos()*(1-t) + v2->pos()*t;

        cut->label = std::min(v1->label, v2->label);
        cut->lbls[v1->label] = true;
        cut->lbls[v2->label] = true;

        // check violating condition
        if ((t <= edge->alpha) || (t >= (1 - edge->mate->alpha)))
            cut->violating = true;
        else
            cut->violating = false;

        if(t < 0.5){
            if(v1->isExterior)
                cut->closestGeometry = v2;
            else
                cut->closestGeometry = v1;
        }
        else{
            if(v1->isExterior)
                cut->closestGeometry = v1;
            else
                cut->closestGeometry = v2;
        }

        edge->cut = cut;
        edge->mate->cut = cut;
        cut->order() = 1;
        return;
    }

    /*
    if(v1->dual && v2->dual)
    {
        //std::cout << "Computing a cut on a dual edge" << std::endl;

        // first compute cut using v1 cell data
        bool cell_1_cut = false;
        vec3 cut1_pos;
        Vertex *cut1 = NULL;
        {
            double tt = 0.4999;
            vec3 p1 = v1->pos();
            vec3 p2 = (1-tt)*v1->pos() + tt*v2->pos();

            int a_mat = m_volume->maxAt(p1);
            int b_mat = m_volume->maxAt(p2);

            if(a_mat != b_mat)
            {
                cell_1_cut = true;
                double a1 = m_volume->valueAt(p1, a_mat);
                double a2 = m_volume->valueAt(p2, a_mat);
                double b1 = m_volume->valueAt(p1, b_mat);
                double b2 = m_volume->valueAt(p2, b_mat);
                double top = (a1 - b1);
                double bot = (b2 - a2 + a1 - b1);
                double t = top / bot;

                cut1 = new Vertex(m_volume->numberOfMaterials());
                t = std::max(t, 0.0);
                t = std::min(t, 1.0);
                cut1->pos() = p1*(1-t) + p2*t;

                cut1->closestGeometry = v1;

                // doesn't really matter which
                cut1->label = a_mat;
                cut1->lbls[a_mat] = true;
                cut1->lbls[b_mat] = true;
                cut1->order() = 1;

                // check violating condition
                if (t <= edge->alpha)
                    cut1->violating = true;
                else
                    cut1->violating = false;
            }
        }


        // next  compute cut using v2 cell data
        bool cell_2_cut = false;
        vec3 cut2_pos;
        Vertex *cut2 = NULL;
        {
            double tt = 0.4999;
            vec3 p1 = v2->pos();
            vec3 p2 = (1-tt)*v2->pos() + tt*v1->pos();

            int a_mat = m_volume->maxAt(p1);
            int b_mat = m_volume->maxAt(p2);

            if(a_mat != b_mat)
            {
                cell_2_cut = true;
                double a1 = m_volume->valueAt(p1, a_mat);
                double a2 = m_volume->valueAt(p2, a_mat);
                double b1 = m_volume->valueAt(p1, b_mat);
                double b2 = m_volume->valueAt(p2, b_mat);
                double top = (a1 - b1);
                double bot = (b2 - a2 + a1 - b1);
                double t = top / bot;

                cut2 = new Vertex(m_volume->numberOfMaterials());
                t = std::max(t, 0.0);
                t = std::min(t, 1.0);
                cut2->pos() = p1*(1-t) + p2*t;

                cut2->closestGeometry = v2;

                // doesn't really matter which
                cut2->label = a_mat;
                cut2->lbls[a_mat] = true;
                cut2->lbls[b_mat] = true;
                cut2->order() = 1;

                // check violating condition
                if (t <= edge->alpha)
                    cut2->violating = true;
                else
                    cut2->violating = false;
            }
        }

        if(cell_1_cut && cell_2_cut){
            std::cout << "Uh oh, 2 cuts found on a dual edge, must split!" << std::endl;

        }
        else if(cell_1_cut){
            // use cell 1 cut location
            edge->cut = cut1;
            edge->mate->cut = cut1;
            delete cut2;
            return;
        }
        else if(cell_2_cut){
            // use cell 2 cut location
            edge->cut = cut2;
            edge->mate->cut = cut2;
            delete cut1;
            return;
        }
        else{
            //std::cout << "No cut wtf" << std::endl;
        }
    }
    */

    //---- The Following is the STANDARD cut computation code

    int a_mat = v1->label;
    int b_mat = v2->label;


    double a1 = m_volume->valueAt(v1->pos(), a_mat); //  v1->vals[a_mat];
    double a2 = m_volume->valueAt(v2->pos(), a_mat); //  v2->vals[a_mat];
    double b1 = m_volume->valueAt(v1->pos(), b_mat); //  v1->vals[b_mat];
    double b2 = m_volume->valueAt(v2->pos(), b_mat); //  v2->vals[b_mat];
    double top = (a1 - b1);
    double bot = (b2 - a2 + a1 - b1);
    double t = top / bot;

    Vertex *cut = new Vertex(m_volume->numberOfMaterials());
    t = std::max(t, 0.0);
    t = std::min(t, 1.0);
    cut->pos() = v1->pos()*(1-t) + v2->pos()*t;

    if(t < 0.5)
        cut->closestGeometry = v1;
    else
        cut->closestGeometry = v2;

    // doesn't really matter which
    cut->label = a_mat;
    cut->lbls[v1->label] = true;
    cut->lbls[v2->label] = true;

    // check violating condition
    if ((t <= edge->alpha) || (t >= (1 - edge->mate->alpha)))
        cut->violating = true;
    else
        cut->violating = false;

    edge->cut = cut;
    edge->mate->cut = cut;
    cut->order() = 1;

    /*
    {
        double t1 = 0.000000;
        double t2 = 0.333333;
        double t3 = 0.666666;
        double t4 = 1.000000;

        vec3 x1 = (1-t1)*v1->pos() + t1*v2->pos();
        vec3 x2 = (1-t2)*v1->pos() + t2*v2->pos();
        vec3 x3 = (1-t3)*v1->pos() + t3*v2->pos();
        vec3 x4 = (1-t4)*v1->pos() + t4*v2->pos();

        double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, b_mat);
        double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, b_mat);
        double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, b_mat);
        double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, b_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1,p2,p3,p4,c);

        if(c[3] == 0)
            num_roots = SolveQuadric(c, s);
        else
            num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if(num_roots != 1)
        {
            std::cout << "wow, unexpected for this dataset!" << std::endl;
            std::cout << "roots = [";
            for(int i=0; i < num_roots; i++)
                std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
            std::cout << std::endl;

            std::cout << "Points: ["
                      << "(" << t1 << "," << y1 << "),"
                      << "(" << t2 << "," << y2 << "),"
                      << "(" << t3 << "," << y3 << "),"
                      << "(" << t4 << "," << y4 << ")]" << std::endl;
            std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                      << ", c=" << c[2] << ", d=" << c[3] << std::endl;

            //exit(0);
            badEdges.push_back(v1->pos());
            badEdges.push_back(v2->pos());
        }

        double t_ab = s[0];
        t_ab = std::max(t_ab, 0.0);
        t_ab = std::min(t_ab, 1.0);

        Vertex *cut = new Vertex(m_volume->numberOfMaterials());
        cut->pos() = (1-t_ab)*v1->pos() + t_ab*v2->pos();

        if(t_ab < 0.5)
            cut->closestGeometry = v1;
        else
            cut->closestGeometry = v2;

        // doesn't really matter which
        cut->label = a_mat;
        cut->lbls[v1->label] = true;
        cut->lbls[v2->label] = true;

        // check violating condition
        if ((t_ab <= edge->alpha) || (t_ab >= (1 - edge->mate->alpha)))
            cut->violating = true;
        else
            cut->violating = false;

        edge->cut = cut;
        edge->mate->cut = cut;
        cut->order() = 1;
    }
    */




    // Now Test If a 3rd material pops up, SANITY CHECK test
    bool ac_crossing = false;
    bool bc_crossing = false;

    /*
    if(m_volume->numberOfMaterials() > 2)
    {        
        double t_ac = -1;
        double t_bc = -1;

        // get 3rd material
        int c_mat = -1;
        for(int m=0; m < m_volume->numberOfMaterials(); m++){
            if(m != a_mat && m != b_mat)
            {
                c_mat = m;
                break;
            }
        }

        // compute crossing parameter t_ac (a,c crossing)
        {
            double a1 = m_volume->valueAt(v1->pos(), a_mat);
            double a2 = m_volume->valueAt(v2->pos(), a_mat);
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);

            // since a is definitely maximum on v1, can only
            // be a crossing if c is greater than a on v2
            if(c2 > a2)
            {
                double top = (a1 - c1);
                double bot = (c2 - a2 + a1 - c1);
                double t = top / bot;

                vec3   pos = v1->pos()*(1-t) + v2->pos()*(t);
                double ac  = a1*(1-t) + a2*(t);

                if(ac >= m_volume->valueAt(pos, b_mat) && t >= 0.0 && t <= 1.0)
                {
                    ac_crossing = true;
                    t_ac = t;
                }
            }
        }

        // compute crossing parameter t_bc (b,c crossing)
        {
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);
            double b1 = m_volume->valueAt(v1->pos(), b_mat);
            double b2 = m_volume->valueAt(v2->pos(), b_mat);

            // since b is definitely maximum on v2, can only
            // be a crossing if c is greater than b on v1
            if(c1 > b1)
            {
                double top = (c1 - b1);
                double bot = (b2 - c2 + c1 - b1);
                double t = top / bot;

                vec3   pos = v1->pos()*(1-t) + v2->pos()*(t);
                double bc  = c1*(1-t) + c2*(t);

                if(bc >= m_volume->valueAt(pos, a_mat) && t >= 0.0 && t <= 1.0)
                {
                    bc_crossing = true;
                    t_bc = t;
                }
            }
        }

        // if 3rd material pops up, handle it
        if(ac_crossing && bc_crossing){
            std::cout << "Sanity FAIL - Computing Multiple Crossings on Edge!" << std::endl;
            std::cout << "\tv1 = " << v1->pos().toString() << std::endl;
            std::cout << "\tv2 = " << v2->pos().toString() << std::endl;

            badEdges.push_back(v1->pos());
            badEdges.push_back(v2->pos());
        }
    }
    */

    /*
    // Now Test If a 3rd material pops up, SANITY CHECK test
    if(m_volume->numberOfMaterials() > 2)// && !ac_crossing && bc_crossing)
    {
        double t_ac = -1;
        double t_bc = -1;

        // get 3rd material
        int c_mat = -1;
        for(int m=0; m < m_volume->numberOfMaterials(); m++){
            if(m != a_mat && m != b_mat)
            {
                c_mat = m;
                break;
            }
        }

        double t1 = 0.000000;
        double t2 = 0.333333;
        double t3 = 0.666666;
        double t4 = 1.000000;

        vec3 x1 = (1-t1)*v1->pos() + t1*v2->pos();
        vec3 x2 = (1-t2)*v1->pos() + t2*v2->pos();
        vec3 x3 = (1-t3)*v1->pos() + t3*v2->pos();
        vec3 x4 = (1-t4)*v1->pos() + t4*v2->pos();

        // compute crossing parameter t_ac (a,c crossing)
        {
            double a1 = m_volume->valueAt(v1->pos(), a_mat);
            double a2 = m_volume->valueAt(v2->pos(), a_mat);
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);

            // since a is definitely maximum on v1, can only
            // be a crossing if c is greater than a on v2
            if(c2 > a2)
            {
                double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(v1->pos());
                    badEdges.push_back(v2->pos());
                }

                t_ac = s[0];

                vec3 pos = (1-t_ac)*v1->pos() + t_ac*v2->pos();

                double ac = m_volume->valueAt(pos, a_mat);
                double  b = m_volume->valueAt(pos, b_mat);
                if(ac >= b && t_ac >= 0.0 && t_ac <= 1.0)
                {
                    ac_crossing = true;
                }
            }
        }

        // compute crossing parameter t_bc (b,c crossing)
        {
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);
            double b1 = m_volume->valueAt(v1->pos(), b_mat);
            double b2 = m_volume->valueAt(v2->pos(), b_mat);

            // since b is definitely maximum on v2, can only
            // be a crossing if c is greater than b on v1
            if(c1 > b1)
            {
                double y1 = m_volume->valueAt(x1, b_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, b_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, b_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, b_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(v1->pos());
                    badEdges.push_back(v2->pos());
                }

                t_bc = s[0];

                vec3 pos = (1-t_bc)*v1->pos() + t_bc*v2->pos();

                double bc = m_volume->valueAt(pos, b_mat);
                double  a = m_volume->valueAt(pos, a_mat);
                if(bc >= a && t_bc >= 0.0 && t_bc <= 1.0)
                {
                    bc_crossing = true;
                }
            }
        }

        // if 3rd material pops up, handle it
        if(ac_crossing && bc_crossing){
            std::cout << "Improved Sanity FAIL - Computing Multiple Crossings on Edge!" << std::endl;
            std::cout << "\tv1 = " << v1->pos().toString() << v1->order() << std::endl;
            std::cout << "\tv2 = " << v2->pos().toString() << v2->order() << std::endl;
            std::cout << "\tt_ac = " << t_ac << std::endl;
            std::cout << "\tt_bc = " << t_bc << std::endl;

            badEdges.push_back(v1->pos());
            badEdges.push_back(v2->pos());
        }
    }
    */
}

/*
void construct_plane(const vec3 &p1, const vec3 &p2, const vec3 &p3, float &a, float &b, float &c, float &d)
{
    vec3 n = normalize(((p2 - p1).cross(p3 - p1)));
    a = (float)n.x;
    b = (float)n.y;
    c = (float)n.z;
    d = (float)-n.dot(p1);
}
*/

void CleaverMesherImp::computeTopologicalCutForEdge2(HalfEdge *edge)
{
    double t_ab;
    double t_ac;
    double t_bc;
    bool ac_crossing = false;
    bool bc_crossing = false;

    // order verts
    Vertex *v2 = edge->vertex;
    Vertex *v1 = edge->mate->vertex;

    // set as evaluated
    edge->evaluated = true;
    edge->mate->evaluated = true;

    // TODO: Strictly speaking, could still have a material popup, even if the SAME lable.
    //       REMOVE THIS CHECK WHEN READY
    // do labels differ?
    if(v1->label == v2->label)
        return;

    double t1 = 0.000000;
    double t2 = 0.333333;
    double t3 = 0.666666;
    double t4 = 1.000000;

    vec3 x1 = (1-t1)*v1->pos() + t1*v2->pos();
    vec3 x2 = (1-t2)*v1->pos() + t2*v2->pos();
    vec3 x3 = (1-t3)*v1->pos() + t3*v2->pos();
    vec3 x4 = (1-t4)*v1->pos() + t4*v2->pos();

    int a_mat = v1->label;
    int b_mat = v2->label;


    {
        double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, b_mat);
        double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, b_mat);
        double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, b_mat);
        double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, b_mat);

        vec3 p1 = vec3(t1, y1, 0.0);
        vec3 p2 = vec3(t2, y2, 0.0);
        vec3 p3 = vec3(t3, y3, 0.0);
        vec3 p4 = vec3(t4, y4, 0.0);


        double c[4];             // coefficients (a,b,c,d)
        double s[3];             // solutions    (roots)
        int num_roots = 0;

        computeLagrangePolynomial(p1,p2,p3,p4,c);

        if(c[3] == 0)
            num_roots = SolveQuadric(c, s);
        else
            num_roots = SolveCubic(c, s);
        clipRoots(s, num_roots);

        if(num_roots != 1)
        {
            std::cout << "wow, unexpected for this dataset!" << std::endl;
            std::cout << "roots = [";
            for(int i=0; i < num_roots; i++)
                std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
            std::cout << std::endl;

            std::cout << "Points: ["
                      << "(" << t1 << "," << y1 << "),"
                      << "(" << t2 << "," << y2 << "),"
                      << "(" << t3 << "," << y3 << "),"
                      << "(" << t4 << "," << y4 << ")]" << std::endl;
            std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                      << ", c=" << c[2] << ", d=" << c[3] << std::endl;

            //exit(0);
            badEdges.push_back(v1->pos());
            badEdges.push_back(v2->pos());
        }

        t_ab = s[0];
    }

    // Now Test if a 3rd Material pops up
    if(m_volume->numberOfMaterials() > 2)
    {

        // get 3rd material
        int c_mat = -1;
        for(int m=0; m < m_volume->numberOfMaterials(); m++){
            if(m != a_mat && m != b_mat)
            {
                c_mat = m;
                break;
            }
        }

        // compute crossing parameter t_ac (a,c crossing)
        {
            double a1 = m_volume->valueAt(v1->pos(), a_mat);
            double a2 = m_volume->valueAt(v2->pos(), a_mat);
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);

            // since a is definitely maximum on v1, can only
            // be a crossing if c is greater than a on v2
            if(c2 > a2)
            {
                double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(v1->pos());
                    badEdges.push_back(v2->pos());
                }

                t_ac = s[0];

                vec3 pos = (1-t_ac)*v1->pos() + t_ac*v2->pos();

                double ac = m_volume->valueAt(pos, a_mat);
                double  b = m_volume->valueAt(pos, b_mat);
                if(ac >= b && t_ac >= 0.0 && t_ac <= 1.0)
                {
                    ac_crossing = true;
                }
            }
        }

        // compute crossing parameter t_bc (b,c crossing)
        {
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);
            double b1 = m_volume->valueAt(v1->pos(), b_mat);
            double b2 = m_volume->valueAt(v2->pos(), b_mat);

            // since b is definitely maximum on v2, can only
            // be a crossing if c is greater than b on v1
            if(c1 > b1)
            {
                double y1 = m_volume->valueAt(x1, b_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, b_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, b_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, b_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(v1->pos());
                    badEdges.push_back(v2->pos());
                }

                t_bc = s[0];

                vec3 pos = (1-t_bc)*v1->pos() + t_bc*v2->pos();

                double bc = m_volume->valueAt(pos, b_mat);
                double  a = m_volume->valueAt(pos, a_mat);
                if(bc >= a && t_bc >= 0.0 && t_bc <= 1.0)
                {
                    bc_crossing = true;
                }
            }
        }

        // if 3rd material pops up, handle it
        if(ac_crossing && bc_crossing)
        {

            // put topological cut haflway between ac/bc interfaces
            double tt = 0.5f*(t_ac + t_bc);

            Vertex *cut = new Vertex(m_volume->numberOfMaterials());
            tt = std::max(tt, 0.0);
            tt = std::min(tt, 1.0);

            // check violating condition
            if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
                cut->violating = true;
            else
                cut->violating = false;

            cut->order() = 1;
            cut->pos() = v1->pos()*(1-tt) + v2->pos()*tt;

            if(tt < 0.5)
                cut->closestGeometry = v1;
            else
                cut->closestGeometry = v2;

            // doesn't really matter which
            cut->label = c_mat;
            cut->lbls[c_mat] = true;

            // check violating condition
            if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
                cut->violating = true;
            else
                cut->violating = false;

            cut->order() = 1;

            //---------------------
            // attach cut to edge
            //---------------------
            cut->phantom = false;   // does it actually split a topology?
            edge->cut = cut;
            edge->mate->cut = cut;
        }
    }
}

void CleaverMesherImp::computeTopologicalCutForEdge(HalfEdge *edge)
{
    // order verts
    Vertex *v2 = edge->vertex;
    Vertex *v1 = edge->mate->vertex;

    // set as evaluated
    edge->evaluated = true;
    edge->mate->evaluated = true;

    // do labels differ?
    if(v1->label == v2->label)
        return;

    // added feb 20 to attempt boundary conforming
    if((v1->isExterior && !v2->isExterior) || (!v1->isExterior && v2->isExterior))
    {


    }

    int a_mat = v1->label;
    int b_mat = v2->label;

    double a1 = m_volume->valueAt(v1->pos(), a_mat); //  v1->vals[a_mat];
    double a2 = m_volume->valueAt(v2->pos(), a_mat); //  v2->vals[a_mat];
    double b1 = m_volume->valueAt(v1->pos(), b_mat); //  v1->vals[b_mat];
    double b2 = m_volume->valueAt(v2->pos(), b_mat); //  v2->vals[b_mat];
    double top = (a1 - b1);
    double bot = (b2 - a2 + a1 - b1);
    double t = top / bot;

    // Now Test If a 3rd material pops up, test
    if(m_volume->numberOfMaterials() > 2)
    {
        bool ac_crossing = false;
        bool bc_crossing = false;
        double t_ac = -1;
        double t_bc = -1;

        // get 3rd material
        int c_mat = -1;
        for(int m=0; m < m_volume->numberOfMaterials(); m++)
        {
            if(m != a_mat && m != b_mat)
            {
                c_mat = m;
                break;
            }
        }

        // compute crossing parameter t_ac (a,c crossing)
        {
            double a1 = m_volume->valueAt(v1->pos(), a_mat);
            double a2 = m_volume->valueAt(v2->pos(), a_mat);
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);

            // since a is definitely maximum on v1, can only
            // be a crossing if c is greater than a on v2
            if(c2 > a2)
            {
                double top = (a1 - c1);
                double bot = (c2 - a2 + a1 - c1);
                double t = top / bot;

                vec3   pos = v1->pos()*(1-t) + v2->pos()*(t);
                //double ac  = a1*(1-t) + a2*(t);
                double ac  = m_volume->valueAt(pos, c_mat);

                if(ac >= m_volume->valueAt(pos, b_mat) && t >= 0.0 && t <= 1.0)
                {
                    ac_crossing = true;
                    t_ac = t;
                }
            }
        }

        // compute crossing parameter t_bc (b,c crossing)
        {
            double c1 = m_volume->valueAt(v1->pos(), c_mat);
            double c2 = m_volume->valueAt(v2->pos(), c_mat);
            double b1 = m_volume->valueAt(v1->pos(), b_mat);
            double b2 = m_volume->valueAt(v2->pos(), b_mat);

            // since b is definitely maximum on v2, can only
            // be a crossing if c is greater than b on v1
            if(c1 > b1)
            {
                double top = (c1 - b1);
                double bot = (b2 - c2 + c1 - b1);
                double t = top / bot;

                vec3   pos = v1->pos()*(1-t) + v2->pos()*(t);
                //double bc  = c1*(1-t) + c2*(t);
                double bc = m_volume->valueAt(pos, c_mat);

                if(bc >= m_volume->valueAt(pos, a_mat) && t >= 0.0 && t <= 1.0)
                {
                    bc_crossing = true;
                    t_bc = t;
                }
            }
        }

        // if 3rd material pops up, handle it
        if(ac_crossing && bc_crossing){

            // put topological cut haflway between ac/bc interfaces
            double tt = 0.5f*(t_ac + t_bc);

            Vertex *cut = new Vertex(m_volume->numberOfMaterials());
            tt = std::max(tt, 0.0);
            tt = std::min(tt, 1.0);

            // check violating condition
            if ((tt <= edge->alpha) || (tt >= (1 - edge->mate->alpha)))
                cut->violating = true;
            else
                cut->violating = false;

            cut->order() = 1;
            cut->pos() = (1-tt)*v1->pos() + tt*v2->pos();

            if(tt < 0.5)
                cut->closestGeometry = v1;
            else
                cut->closestGeometry = v2;

            // doesn't really matter which
            cut->label = c_mat;
            cut->lbls[c_mat] = true;

            //---------------------
            // attach cut to edge
            //---------------------
            cut->phantom = false;   // does it actually split a topology?
            edge->cut = cut;
            edge->mate->cut = cut;

        }
    }
}


//====================================================================
//   plane_intersect()
//====================================================================
bool plane_intersect(Vertex *v1, Vertex *v2, Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, float epsilon = 1E-4)
{
    //-------------------------------------------------
    // if v1, v2, and v3 are not unique, return FALSE
    //-------------------------------------------------
    if(v1->isEqualTo(v2) || v2->isEqualTo(v3) || v1->isEqualTo(v3))
        return false;
    else if(L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v1->pos() - v3->pos()) < epsilon)
        return false;


    vec3 p1 = origin;
    vec3 p2 = origin + ray;
    vec3 p3 = v1->pos();

    vec3 n = normalize(cross(normalize(v3->pos() - v1->pos()), normalize(v2->pos() - v1->pos())));

    double top = n.dot(p3 - p1);
    double bot = n.dot(p2 - p1);

    double t = top / bot;

    pt = origin + t*ray;

    if(pt != pt)
        return false;
    else
        return true;
}

void force_point_in_triangle(vec3 a, vec3 b, vec3 c, vec3 &p)
{
    // Compute vectors
    vec3 v0 = c - a;
    vec3 v1 = b - a;
    vec3 v2 = p - a;

    // Compute dot products
    double dot00 = dot(v0, v0);
    double dot01 = dot(v0, v1);
    double dot02 = dot(v0, v2);
    double dot11 = dot(v1, v1);
    double dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    double w = 1 - u - v;

    vec3 test = (1 - u - v)*a + v*b + u*c;

    // Check if point is in triangle
    u = std::max(0.0, u);
    v = std::max(0.0, v);
    w = std::max(0.0, w);

    double L1 = u + v + w;
    if(L1 > 0){
        u /= L1;
        v /= L1;
    }

    p = (1 - u - v)*a + v*b + u*c;
}


//---------------------------------------------------
//  compute Triple()
//---------------------------------------------------
void CleaverMesherImp::computeTripleForFace(HalfFace *face)
{
    // set as evaluated
    face->evaluated = true;
    if(face->mate)
        face->mate->evaluated = true;

    //------------------------------------------------------------------------
    // THIS CODE SHOULD NOT BE HERE - DEBUGGING CODE ONLY - REMOVE IMMEDIATELY (start)
    //------------------------------------------------------------------------
    /*
    if(!face->halfEdges[0] || !face->halfEdges[1] || !face->halfEdges[2])
        return;
    */
    //------------------------------------------------------------------------
    // THIS CODE SHOULD NOT BE HERE - DEBUGGING CODE ONLY - REMOVE IMMEDIATELY (end)
    //------------------------------------------------------------------------

    // only continue if 3 cuts exist
    if(!face->halfEdges[0]->cut || !face->halfEdges[1]->cut || !face->halfEdges[2]->cut)
        return;

    Vertex *verts[3];
    HalfEdge *edges[3];

    m_mesh->getAdjacencyListsForFace(face, verts, edges);
    Vertex *v1 = verts[0];
    Vertex *v2 = verts[1];
    Vertex *v3 = verts[2];

    vec3 result = vec3::zero;

    double a1,b1,c1,d1;
    double a2,b2,c2,d2;
    double a3,b3,c3,d3;

    // get materials
    int m1 = v1->label;
    int m2 = v2->label;
    int m3 = v3->label;

    // added feb 20 to attempt boundary conforming
    if(v1->isExterior || v2->isExterior || v3->isExterior)
    {
        // place a triple exactly on boundary.
        // for now, put in middle, its wrong but just to test
        int external_vertex;
        for(int i=0; i < 3; i++){
            if(verts[i]->isExterior){
                external_vertex = i;
                break;
            }
        }

        vec3 a = edges[(external_vertex+1)%3]->cut->pos();
        vec3 b = edges[(external_vertex+2)%3]->cut->pos();


        Vertex *triple = new Vertex(m_volume->numberOfMaterials());
        triple->pos() = (0.5)*(a + b);
        triple->lbls[v1->label] = true;
        triple->lbls[v2->label] = true;
        triple->lbls[v3->label] = true;
        triple->order() = TRIP;
        triple->violating = false;
        triple->closestGeometry = NULL;
        face->triple = triple;
        if(face->mate)
            face->mate->triple = triple;

        checkIfTripleViolatesVertices(face);

        return;
    }

    //computeTripleForFace2(face);
    //return;


    // determine orientation, pick an axis
    vec3 n = normalize((v2 ->pos() - v1->pos()).cross(v3->pos() - v1->pos()));

    int axis;
    double nx = fabs(n.dot(vec3(1,0,0)));
    double ny = fabs(n.dot(vec3(0,1,0)));
    double nz = fabs(n.dot(vec3(0,0,1)));

    if(nx >= ny && nx >= nz)
        axis = 1;
    else if(ny >= nx && ny >= nz)
        axis = 2;
    else
        axis = 3;

    if(axis == 1)
    {
        /*
        vec3 p1_m1 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m1), v1->pos().z);
        vec3 p2_m1 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m1), v2->pos().z);
        vec3 p3_m1 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m1), v3->pos().z);

        vec3 p1_m2 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m2), v1->pos().z);
        vec3 p2_m2 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m2), v2->pos().z);
        vec3 p3_m2 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m2), v3->pos().z);

        vec3 p1_m3 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m3), v1->pos().z);
        vec3 p2_m3 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m3), v2->pos().z);
        vec3 p3_m3 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m3), v3->pos().z);
        */

        vec3 p1_m1 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m1), v1->pos().z);
        vec3 p2_m1 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m1), v2->pos().z);
        vec3 p3_m1 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m1), v3->pos().z);

        vec3 p1_m2 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m2), v1->pos().z);
        vec3 p2_m2 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m2), v2->pos().z);
        vec3 p3_m2 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m2), v3->pos().z);

        vec3 p1_m3 = vec3(v1->pos().y, m_volume->valueAt(v1->pos(), m3), v1->pos().z);
        vec3 p2_m3 = vec3(v2->pos().y, m_volume->valueAt(v2->pos(), m3), v2->pos().z);
        vec3 p3_m3 = vec3(v3->pos().y, m_volume->valueAt(v3->pos(), m3), v3->pos().z);


        //construct_plane(p1_m1, p2_m1, p3_m1, a1,b1,c1,d1);
        //construct_plane(p1_m2, p2_m2, p3_m2, a2,b2,c2,d2);
        //construct_plane(p1_m3, p2_m3, p3_m3, a3,b3,c3,d3);
        Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
        Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
        Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

        plane1.toScalars(a1,b1,c1,d1);
        plane2.toScalars(a2,b2,c2,d2);
        plane3.toScalars(a3,b3,c3,d3);

        double A[2][2];
        double b[2];

        A[0][0] = (a3/b3 - a1/b1);  //(a3/c3 - a1/c1);
        A[0][1] = (c3/b3 - c1/b1);  //(b3/c3 - b1/c1);
        A[1][0] = (a3/b3 - a2/b2);  //(a3/c3 - a2/c2);
        A[1][1] = (c3/b3 - c2/b2);  //(b3/c3 - b2/c2);

        b[0] = (d1/b1 - d3/b3);     //(d1/c1 - d3/c3);
        b[1] = (d2/b2 - d3/b3);     //(d2/c2 - d3/c3);

        // solve using cramers rule
        vec3 result2d = vec3::zero;

        double det = A[0][0]*A[1][1] - A[0][1]*A[1][0];

        result2d.x = (b[0]*A[1][1] - b[1]*A[0][1])/ det;
        result2d.y = (b[1]*A[0][0] - b[0]*A[1][0])/ det;

        // intersect triangle plane with line (from result2d point along axis)
        vec3 origin(0, result2d.x, result2d.y);
        vec3 ray(1, 0, 0);
        bool success = plane_intersect(v1, v2, v3, origin, ray, result);
        if(!success)
        {
            //std::cout << "Failed to Project Triple BACK into 3D: Using Barycenter" << std::endl;
            //result = (1.0/3.0)*(v1->pos() + v2->pos() + v3->pos());
            //axis = 2;
            std::cout << "Failed Axis==1, the most likely candidate to succeeed..." << std::endl;
            exit(0);
        }

    }
    else if(axis == 2)
    {
        vec3 p1_m1 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m1), v1->pos().z);
        vec3 p2_m1 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m1), v2->pos().z);
        vec3 p3_m1 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m1), v3->pos().z);

        vec3 p1_m2 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m2), v1->pos().z);
        vec3 p2_m2 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m2), v2->pos().z);
        vec3 p3_m2 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m2), v3->pos().z);

        vec3 p1_m3 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m3), v1->pos().z);
        vec3 p2_m3 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m3), v2->pos().z);
        vec3 p3_m3 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m3), v3->pos().z);

        //construct_plane(p1_m1, p2_m1, p3_m1, a1,b1,c1,d1);
        //construct_plane(p1_m2, p2_m2, p3_m2, a2,b2,c2,d2);
        //construct_plane(p1_m3, p2_m3, p3_m3, a3,b3,c3,d3);
        Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
        Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
        Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

        plane1.toScalars(a1,b1,c1,d1);
        plane2.toScalars(a2,b2,c2,d2);
        plane3.toScalars(a3,b3,c3,d3);

        double A[2][2];
        double b[2];

        A[0][0] = (a3/b3 - a1/b1);  //(a3/c3 - a1/c1);
        A[0][1] = (c3/b3 - c1/b1);  //(b3/c3 - b1/c1);
        A[1][0] = (a3/b3 - a2/b2);  //(a3/c3 - a2/c2);
        A[1][1] = (c3/b3 - c2/b2);  //(b3/c3 - b2/c2);

        b[0] = (d1/b1 - d3/b3);     //(d1/c1 - d3/c3);
        b[1] = (d2/b2 - d3/b3);     //(d2/c2 - d3/c3);

        // solve using cramers rule
        vec3 result2d = vec3::zero;

        double det = A[0][0]*A[1][1] - A[0][1]*A[1][0];

        result2d.x = (b[0]*A[1][1] - b[1]*A[0][1])/ det;
        result2d.y = (b[1]*A[0][0] - b[0]*A[1][0])/ det;

        // intersect triangle plane with line (from result2d point along axis)
        vec3 origin(result2d.x, 0, result2d.y);
        vec3 ray(0, 1, 0);
        bool success = plane_intersect(v1, v2, v3, origin, ray, result);
        if(!success)
        {
            //std::cout << "Failed to Project Triple BACK into 3D. Using barycenter: " << std::endl;
            //result = (1.0/3.0)*(v1->pos() + v2->pos() + v3->pos());
            //axis = 3;
            std::cout << "Failed Axis==2, the most likely candidate to succeeed..." << std::endl;
            exit(0);
        }
    }
    else if(axis == 3)
    {
        vec3 p1_m1 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m1), v1->pos().y);
        vec3 p2_m1 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m1), v2->pos().y);
        vec3 p3_m1 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m1), v3->pos().y);

        vec3 p1_m2 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m2), v1->pos().y);
        vec3 p2_m2 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m2), v2->pos().y);
        vec3 p3_m2 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m2), v3->pos().y);

        vec3 p1_m3 = vec3(v1->pos().x, m_volume->valueAt(v1->pos(), m3), v1->pos().y);
        vec3 p2_m3 = vec3(v2->pos().x, m_volume->valueAt(v2->pos(), m3), v2->pos().y);
        vec3 p3_m3 = vec3(v3->pos().x, m_volume->valueAt(v3->pos(), m3), v3->pos().y);

        //cout << "axis 3" << endl;
        //construct_plane(p1_m1, p2_m1, p3_m1, a1,b1,c1,d1);
        //construct_plane(p1_m2, p2_m2, p3_m2, a2,b2,c2,d2);
        //construct_plane(p1_m3, p2_m3, p3_m3, a3,b3,c3,d3);
        Plane plane1 = Plane::throughPoints(p1_m1, p2_m1, p3_m1);
        Plane plane2 = Plane::throughPoints(p1_m2, p2_m2, p3_m2);
        Plane plane3 = Plane::throughPoints(p1_m3, p2_m3, p3_m3);

        plane1.toScalars(a1,b1,c1,d1);
        plane2.toScalars(a2,b2,c2,d2);
        plane3.toScalars(a3,b3,c3,d3);

        double A[2][2];
        double b[2];

        A[0][0] = (a3/b3 - a1/b1);  //(a3/c3 - a1/c1);
        A[0][1] = (c3/b3 - c1/b1);  //(b3/c3 - b1/c1);
        A[1][0] = (a3/b3 - a2/b2);  //(a3/c3 - a2/c2);
        A[1][1] = (c3/b3 - c2/b2);  //(b3/c3 - b2/c2);

        b[0] = (d1/b1 - d3/b3);     //(d1/c1 - d3/c3);
        b[1] = (d2/b2 - d3/b3);     //(d2/c2 - d3/c3);

        // solve using cramers rule
        vec3 result2d = vec3::zero;

        double det = A[0][0]*A[1][1] - A[0][1]*A[1][0];

        result2d.x = (b[0]*A[1][1] - b[1]*A[0][1])/ det;
        result2d.y = (b[1]*A[0][0] - b[0]*A[1][0])/ det;

        // intersect triangle plane with line (from result2d point along axis)
        vec3 origin(result2d.x, result2d.y, 0);
        vec3 ray(0, 0, 1);
        bool success = plane_intersect(v1, v2, v3, origin, ray, result);
        if(!success)
        {
            //std::cout << "Failed to Project Triple BACK into 3D. Using barycenter: " << std::endl;
            //result = (1.0/3.0)*(v1->pos() + v2->pos() + v3->pos());
            std::cout << "Failed Axis==3, the most likely candidate to succeeed..." << std::endl;
            exit(0);
        }
    }

    force_point_in_triangle(v1->pos(), v2->pos(), v3->pos(), result);


    //-------------------------------------------------------
    // Create the Triple Vertex
    //-------------------------------------------------------
    Vertex *triple = new Vertex(m_volume->numberOfMaterials());
    triple->pos() = result;
    triple->lbls[v1->label] = true;
    triple->lbls[v2->label] = true;
    triple->lbls[v3->label] = true;
    triple->order() = TRIP;
    triple->violating = false;
    triple->closestGeometry = NULL;
    face->triple = triple;
    if(face->mate)
        face->mate->triple = triple;

    // check if point is violating
    checkIfTripleViolatesVertices(face);
}

//===============================================
//  computeTripleForFace2()
//
//===============================================
void CleaverMesherImp::computeTripleForFace2(HalfFace *face)
{
    // set as evaluated
    face->evaluated = true;
    if(face->mate)
        face->mate->evaluated = true;

    // only continue if 3 cuts exist
    if(!face->halfEdges[0]->cut || !face->halfEdges[1]->cut || !face->halfEdges[2]->cut)
        return;

    Vertex *verts[3];
    HalfEdge *edges[3];

    m_mesh->getAdjacencyListsForFace(face, verts, edges);
    Vertex *v1 = verts[0];
    Vertex *v2 = verts[1];
    Vertex *v3 = verts[2];

    // get materials
    int m1 = verts[0]->label;
    int m2 = verts[1]->label;
    int m3 = verts[2]->label;

    // Create Matrix with Material Values
    Matrix3x3 M;
    M(0,0) = m_volume->valueAt(verts[0]->pos(), m1);
    M(0,1) = m_volume->valueAt(verts[0]->pos(), m2);
    M(0,2) = m_volume->valueAt(verts[0]->pos(), m3);
    M(1,0) = m_volume->valueAt(verts[1]->pos(), m1);
    M(1,1) = m_volume->valueAt(verts[1]->pos(), m2);
    M(1,2) = m_volume->valueAt(verts[1]->pos(), m3);
    M(2,0) = m_volume->valueAt(verts[2]->pos(), m1);
    M(2,1) = m_volume->valueAt(verts[2]->pos(), m2);
    M(2,2) = m_volume->valueAt(verts[2]->pos(), m2);

    // Solve Inverse
    Matrix3x3 Inv = M.inverse();

    // Multiply Inverse by 1 column vector [1,1,1]^T
    vec3 one(1,1,1);
    vec3 slambda = Inv*one;
    vec3  lambda = slambda / L1(slambda);


    vec3 result( lambda.dot(v1->pos()) ,
                 lambda.dot(v2->pos()) ,
                 lambda.dot(v2->pos()) );

    if(lambda.x < 0 || lambda.y < 0 || lambda.z < 0)
        std::cout << "Triple location suggests topology is wrong" << std::endl;

    force_point_in_triangle(v1->pos(), v2->pos(), v3->pos(), result);


    //-------------------------------------------------------
    // Create the Triple Vertex
    //-------------------------------------------------------
    Vertex *triple = new Vertex(m_volume->numberOfMaterials());
    triple->pos() = result;
    triple->lbls[v1->label] = true;
    triple->lbls[v2->label] = true;
    triple->lbls[v3->label] = true;
    triple->order() = TRIP;
    triple->violating = false;
    triple->closestGeometry = NULL;
    face->triple = triple;
    if(face->mate)
        face->mate->triple = triple;

    // check if point is violating
    checkIfTripleViolatesVertices(face);
}

void CleaverMesherImp::computeTopologicalTripleForFace(HalfFace *face)
{
    // set as evaluated
    face->evaluated = true;
    if(face->mate)
        face->mate->evaluated = true;

    // if  least 1 topological cut exists, determine how to place other missing cuts
    if((face->halfEdges[0]->cut && face->halfEdges[0]->cut->phantom) ||
       (face->halfEdges[1]->cut && face->halfEdges[1]->cut->phantom) ||
       (face->halfEdges[2]->cut && face->halfEdges[2]->cut->phantom)){

        int mat_count = 3;
        int top_count = 0;
        int mats[3];
        for(int e=0; e < 3; e++){
            mats[e] = face->halfEdges[e]->vertex->label;
            if(face->halfEdges[e]->cut && !face->halfEdges[e]->cut->phantom)
                top_count++;
        }
        if(mats[0] == mats[1] || mats[0] == mats[2])
            mat_count--;
        if(mats[1] == mats[2])
            mat_count--;

        // 2-mat has 2 cases, only 1 case needs a triple
        if(mat_count == 2)
        {
            // if there are 2 top cuts, there is no topological triple
            if(top_count == 2 )
                return;

            // first, determine which edge is which
            HalfEdge *edge_with_3_mats = NULL;
            HalfEdge *edge_with_2_mats = NULL;
            HalfEdge *edge_with_1_mats = NULL;
            int e1 = -1, e2 = -1, e3 = -1;

            for(int e=0; e < 3; e++){
                // 1 edge must have 0 mat cuts (2 verts with same label)
                if(face->halfEdges[e]->vertex->label == face->halfEdges[e]->mate->vertex->label)
                {
                    edge_with_1_mats = face->halfEdges[e];
                    e1 = e;
                    continue;
                }
                // 1 edge must have 1 mat cuts (materials equal to vertices of edge with top cut)
                if(!face->halfEdges[e]->cut || (face->halfEdges[e]->cut && face->halfEdges[e]->cut->phantom))
                {
                    edge_with_2_mats = face->halfEdges[e];
                    e2 = e;
                    continue;
                }
                // 1 edge must have 2 mat cuts (the only non-phantom topological cut)
                if(face->halfEdges[e]->cut && !face->halfEdges[e]->cut->phantom)
                {
                    edge_with_3_mats = face->halfEdges[e];
                    e3 = e;
                    continue;
                }
            }

            // Sanity Check
            if((!edge_with_1_mats || !edge_with_2_mats || !edge_with_3_mats) &&
                    !face->halfEdges[0]->vertex->isExterior &&
                    !face->halfEdges[1]->vertex->isExterior &&
                    !face->halfEdges[2]->vertex->isExterior)
            {
                std::cerr << "Failed to understand a Topological Triple case. Aborting" << std::endl;
                exit(13);
            }

            //-------------------------------------------------------------------------
            // new logic moves the topological cut to interfaces nearest the 1-mat edge
            //-------------------------------------------------------------------------
            Vertex *v1 = edge_with_3_mats->vertex;
            Vertex *v2 = edge_with_3_mats->mate->vertex;
            Vertex *near = NULL;
            Vertex *far = NULL;

            // 1. Check is v1 or v2 the vertex incident to the edge with 1 material
            if(edge_with_1_mats->vertex == v1 || edge_with_1_mats->mate->vertex == v1)
            {
                near = v1;
                far = v2;
            }
            else if(edge_with_1_mats->vertex == v2 || edge_with_1_mats->mate->vertex == v2)
            {
                near = v2;
                far = v1;
            }
            else{
                std::cerr << "Fatal Error. Bad Triangle Setup. Aborting." << std::endl;
                exit(0);
            }

            // 2. Recompute the 2 crossings
            int a_mat = near->label;
            int b_mat = far->label;
            int c_mat = -1;
            for(int m=0; m < m_volume->numberOfMaterials(); m++)
            {
                if(m != a_mat && m != b_mat)
                {
                    c_mat = m;
                    break;
                }
            }
            double t_ac;
            double t_bc;
            double t1 = 0.000000;
            double t2 = 0.333333;
            double t3 = 0.666666;
            double t4 = 1.000000;
            vec3 x1 = (1-t1)*near->pos() + t1*far->pos();
            vec3 x2 = (1-t2)*near->pos() + t2*far->pos();
            vec3 x3 = (1-t3)*near->pos() + t3*far->pos();
            vec3 x4 = (1-t4)*near->pos() + t4*far->pos();

            // find ac crossing first
            {
                double y1 = m_volume->valueAt(x1, a_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, a_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, a_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, a_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(v1->pos());
                    badEdges.push_back(v2->pos());
                }

                t_ac = s[0];
            }
            // find bc crossing second
            {
                double y1 = m_volume->valueAt(x1, b_mat) - m_volume->valueAt(x1, c_mat);
                double y2 = m_volume->valueAt(x2, b_mat) - m_volume->valueAt(x2, c_mat);
                double y3 = m_volume->valueAt(x3, b_mat) - m_volume->valueAt(x3, c_mat);
                double y4 = m_volume->valueAt(x4, b_mat) - m_volume->valueAt(x4, c_mat);

                vec3 p1 = vec3(t1, y1, 0.0);
                vec3 p2 = vec3(t2, y2, 0.0);
                vec3 p3 = vec3(t3, y3, 0.0);
                vec3 p4 = vec3(t4, y4, 0.0);


                double c[4];             // coefficients (a,b,c,d)
                double s[3];             // solutions    (roots)
                int num_roots = 0;

                computeLagrangePolynomial(p1,p2,p3,p4,c);

                if(c[3] == 0)
                    num_roots = SolveQuadric(c, s);
                else
                    num_roots = SolveCubic(c, s);
                clipRoots(s, num_roots);

                if(num_roots != 1)
                {
                    std::cout << "wow, unexpected for this dataset!" << std::endl;
                    std::cout << "roots = [";
                    for(int i=0; i < num_roots; i++)
                        std::cout << s[i] << ((i+1 < num_roots) ? ", " : "] ");
                    std::cout << std::endl;

                    std::cout << "Points: ["
                              << "(" << t1 << "," << y1 << "),"
                              << "(" << t2 << "," << y2 << "),"
                              << "(" << t3 << "," << y3 << "),"
                              << "(" << t4 << "," << y4 << ")]" << std::endl;
                    std::cout << "Coefficients: a=" << c[0] << " ,b=" << c[1]
                              << ", c=" << c[2] << ", d=" << c[3] << std::endl;

                    //exit(0);
                    badEdges.push_back(near->pos());
                    badEdges.push_back(far->pos());
                }

                t_bc = s[0];
            }


            // 3. Move cut to the one that's closer to incident vertex (near)
            if(t_ac < t_bc)
            {
                t_ac += 1E-2;
                edge_with_3_mats->cut->pos() = (1-t_ac)*near->pos() + t_ac*far->pos();
            }
            else
            {
                t_bc += 1E-2;
                edge_with_3_mats->cut->pos() = (1-t_bc)*near->pos() + t_bc*far->pos();
            }


            //-------------------------------------------------------------------------
            // previous logic created missing phantom cuts and then
            //-------------------------------------------------------------------------
            // added a triple point
            // is edge1 missing a cut? if so, create it
            /*
            if(!edge_with_1_mats->cut)
            {
                Vertex *cut = new Vertex(m_volume->numberOfMaterials());
                Vertex *v1 = edge_with_1_mats->vertex;
                Vertex *v2 = edge_with_1_mats->mate->vertex;
                double tt = 0.5;
                cut->order() = 1;
                cut->pos() = v1->pos()*(1-tt) + v2->pos()*tt;
                cut->phantom = true;
                cut->label = v1->label;
                cut->lbls[v1->label] = true;

                edge_with_1_mats->cut = cut;
                edge_with_1_mats->mate->cut = cut;
            }
            */

            // is edge2 missing a cut? If so, create it
            /*
            if(!edge_with_2_mats->cut)
            {
                Vertex *cut = new Vertex(m_volume->numberOfMaterials());
                Vertex *v1 = edge_with_2_mats->vertex;
                Vertex *v2 = edge_with_2_mats->mate->vertex;
                double tt = 0.5;
                cut->order() = 1;
                cut->pos() = v1->pos()*(1-tt) + v2->pos()*tt;
                cut->phantom = true;
                cut->label = v1->label;
                cut->lbls[v1->label] = true;

                edge_with_2_mats->cut = cut;
                edge_with_2_mats->mate->cut = cut;
            }
            */

            //------------------------------------------------------
            // Now Create The Topological Triple
            //------------------------------------------------------
            /*
            vec3 triple_pos = edge_with_3_mats->cut->pos();

            Vertex *triple = new Vertex(m_volume->numberOfMaterials());
            triple->pos() = triple_pos;
            triple->lbls[face->halfEdges[0]->vertex->label] = true;
            triple->lbls[face->halfEdges[1]->vertex->label] = true;
            triple->lbls[face->halfEdges[2]->vertex->label] = true;
            triple->order() = TRIP;
            triple->violating = false;
            triple->closestGeometry = NULL;
            face->triple = triple;
            if(face->mate)
                face->mate->triple = triple;

            checkIfTripleViolatesVertices(face);
            */
        }

        return;

        // 3-mate case requires no triple, but does require a phantom cut
        /*
        else if(mat_count == 3)
        {

            //------------------------------------------------
            // First, determine which edge is the 3-mat edge
            //------------------------------------------------
            HalfEdge *edge_with_3_mats = NULL;
            int e3 = -1;

            for(int e=0; e < 3; e++)
            {
                if(face->halfEdges[e]->cut && !face->halfEdges[e]->cut->phantom)
                {
                    edge_with_3_mats = face->halfEdges[e];
                    e3 = e;
                    break;
                }
            }

            // sanity check
            if(e3 < 0 && top_count > 0){
                std::cout << "Couldn't find topological crossing edge. Aborting." << std::endl;
                exit(14);
            }

            //------------------------------------------------------
            // check, does a phantom cut exist on either edge yet?
            // if not, create them both
            //------------------------------------------------------
            /*
            vec3 triple_pos = vec3::zero;

            for(int e=0; e < 3; e++)
            {
                if((e != e3) && (!face->halfEdges[e]->cut))
                {
                    Vertex *cut = new Vertex(m_volume->numberOfMaterials());
                    Vertex *v1 = face->halfEdges[e]->vertex;
                    Vertex *v2 = face->halfEdges[e]->mate->vertex;
                    double tt = 0.5;
                    cut->order() = 1;
                    cut->pos() = v1->pos()*(1-tt) + v2->pos()*tt;
                    cut->phantom = true;
                    cut->label = v1->label;
                    cut->lbls[v1->label] = true;

                    face->halfEdges[e]->cut = cut;
                    face->halfEdges[e]->mate->cut = cut;

                    triple_pos += 0.5*cut->pos();
                }
            }

            //------------------------------------------------------
            // Now Create The Topological Triple
            //------------------------------------------------------
            {
                Vertex *triple = new Vertex(m_volume->numberOfMaterials());
                triple->pos() = triple_pos;
                triple->lbls[face->halfEdges[0]->vertex->label] = true;
                triple->lbls[face->halfEdges[1]->vertex->label] = true;
                triple->lbls[face->halfEdges[2]->vertex->label] = true;
                triple->order() = TRIP;
                triple->violating = false;
                triple->closestGeometry = NULL;
                face->triple = triple;
                if(face->mate)
                    face->mate->triple = triple;

                checkIfTripleViolatesVertices(face);
            }            

            // if not, make one on vertex connected to both edges opposite topological cut
        }
        */

    }
}

//===============================================
//  compute Quadruple()
//===============================================
void CleaverMesherImp::computeQuadrupleForTet(Tet *tet)
{
    // set as evaluated
    tet->evaluated = true;

    Vertex *verts[4];
    HalfEdge *edges[6];
    HalfFace *faces[4];

    m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

    // only continue if 6 cuts exist
    for(int e=0; e < 6; e++){
        if(!edges[e]->cut)
            return;
    }

    // TODO:   Implement Compute Quadruple

    // for now, take middle

    std::cerr << "computing a quadruple" << std::endl;

    Vertex *quadruple = new Vertex(m_volume->numberOfMaterials());

    Vertex *v1 = verts[0];
    Vertex *v2 = verts[1];
    Vertex *v3 = verts[2];
    Vertex *v4 = verts[3];

    quadruple->pos() = (1.0/4.0)*(v1->pos() + v2->pos() + v3->pos() + v4->pos());
    quadruple->lbls[v1->label] = true;
    quadruple->lbls[v2->label] = true;
    quadruple->lbls[v3->label] = true;
    quadruple->lbls[v4->label] = true;
    quadruple->label = std::min(v1->label, v2->label);
    tet->quadruple = quadruple;
    tet->quadruple->violating = false;
}

//==============================================
// - computeToplogicalQuadrupleForTet()
//==============================================
void CleaverMesherImp::computeTopologicalQuadrupleForTet(Tet *tet)
{
    // set as evaluated
    tet->evaluated = true;

    Vertex *verts[4];
    HalfEdge *edges[6];
    HalfFace *faces[4];

    m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

    // only continue if 6 cuts exist
    for(int e=0; e < 6; e++){
        if(!edges[e]->cut)
            return;
    }

    Vertex *quadruple = new Vertex(m_volume->numberOfMaterials());

    Vertex *v1 = verts[0];
    Vertex *v2 = verts[1];
    Vertex *v3 = verts[2];
    Vertex *v4 = verts[3];

    quadruple->pos() = (1.0/4.0)*(v1->pos() + v2->pos() + v3->pos() + v4->pos());
    quadruple->lbls[v1->label] = true;
    quadruple->lbls[v2->label] = true;
    quadruple->lbls[v3->label] = true;
    quadruple->lbls[v4->label] = true;
    quadruple->label = std::min(v1->label, v2->label);
    tet->quadruple = quadruple;
    tet->quadruple->phantom = true;
    tet->quadruple->violating = false;
}

//===============================================================
// - checkIfCutViolatesVertices(HalfEdge *edge)
//===============================================================
void CleaverMesherImp::checkIfCutViolatesVertices(HalfEdge *edge)
{
    Vertex *cut = edge->cut;
    vec3 a = edge->mate->vertex->pos();
    vec3 b = edge->vertex->pos();
    vec3 c = cut->pos();

    double t = L2(c - a) / L2(b - a);

    // Check Violations
    if (t <= edge->alpha){
        cut->violating = true;
        cut->closestGeometry = edge->mate->vertex;
    }
    else if(t >= (1 - edge->mate->alpha)){
        cut->violating = true;
        cut->closestGeometry = edge->vertex;
    }
    else{
        cut->violating = false;
    }
}

//===============================================================
// - checkIfTripleViolatesVertices(HalfFace *face)
// This method generalizes the rules that dictate whether a cutpoint
// violates a lattice vertex. Similarly, we also want to know when a
// Triple Point violates such a vertex so that it can be included in
// the warping rules. The generalization follows by extending lines
// from the alpha points, to their opposite corners. The intersection
// of these two edges forms a region of violation for the triple point
// and a vertex. This test must be performed three times, once for each
// vertex in the triangle. Only one vertex can be violated at a time,
// and which vertex is violated is stored.
//===============================================================
void CleaverMesherImp::checkIfTripleViolatesVertices(HalfFace *face)
{
    HalfEdge *edges[EDGES_PER_FACE];
    Vertex *verts[VERTS_PER_FACE];
    Vertex *triple = face->triple;
    triple->violating = false;
    triple->closestGeometry = NULL;

    m_bgMesh->getAdjacencyListsForFace(face, verts, edges);

    vec3 v0 = verts[0]->pos();
    vec3 v1 = verts[1]->pos();
    vec3 v2 = verts[2]->pos();
    vec3 trip = triple->pos();

    // check v0
    if(!triple->violating){
        vec3 e1 = normalize(v0 - v2);       vec3 e2 = normalize(v0 - v1);
        vec3 t1 = normalize(trip - v2);     vec3 t2 = normalize(trip - v1);

        // WARNING: Old Method Normalized Alpha to new edge length: Consider replacing alpha with LENGTH
        double alpha1 = edges[2]->alphaForVertex(verts[0]);
        double alpha2 = edges[1]->alphaForVertex(verts[0]);

        vec3 c1 = v0*(1-alpha1) + alpha1*v1;
        vec3 c2 = v0*(1-alpha2) + alpha2*v2;

        c1 = normalize(c1 - v2);
        c2 = normalize(c2 - v1);

        if(dot(e1, t1) >= dot(e1, c1) &&
           dot(e2, t2) >= dot(e2, c2)){
            triple->violating = true;
            triple->closestGeometry = verts[0];
            return;
        }
    }

    // check v1
    if(!triple->violating){
        vec3 e1 = normalize(v1 - v0);       vec3 e2 = normalize(v1 - v2);
        vec3 t1 = normalize(trip - v0);     vec3 t2 = normalize(trip - v2);

        double alpha1 = edges[0]->alphaForVertex(verts[1]);
        double alpha2 = edges[2]->alphaForVertex(verts[1]);

        vec3 c1 = v1*(1-alpha1) + alpha1*v2;
        vec3 c2 = v1*(1-alpha2) + alpha2*v0;

        c1 = normalize(c1 - v0);
        c2 = normalize(c2 - v2);

        if(dot(e1, t1) >= dot(e1, c1) &&
           dot(e2, t2) >= dot(e2, c2)){
            triple->violating = true;
            triple->closestGeometry = verts[1];
            return;
        }
    }

    // check v2
    if(!triple->violating){
        vec3 e1 = normalize(v2 - v1);      vec3 e2 = normalize(v2 - v0);
        vec3 t1 = normalize(trip - v1);    vec3 t2 = normalize(trip - v0);

        double alpha1 = edges[1]->alphaForVertex(verts[2]);
        double alpha2 = edges[0]->alphaForVertex(verts[2]);

        vec3 c1 = v2*(1-alpha1) + alpha1*v0;
        vec3 c2 = v2*(1-alpha2) + alpha2*v1;

        c1 = normalize(c1 - v1);
        c2 = normalize(c2 - v0);

        if(dot(e1, t1) >= dot(e1, c1) &&
           dot(e2, t2) >= dot(e2, c2)){
            triple->violating = true;
            triple->closestGeometry = verts[2];
            return;
        }
    }
}

//===============================================================
// - checkIfQuadrupleViolatesVertices(Tet *tet)
//
// This method generalizes the rules that dictate whether a triplepoint
// violates a lattice vertex. Similarly, we also want to know when a
// Quadruple Point violates such a vertex so that it can be included
// in the warping rules. The generalization follows by extending lines
// from the alpha points, to their opposite corners. The intersection
// of these three edges forms a region of violation for the quadruple
// point and a vertex. This test must be performed four times, once for
// each vertex in the Lattice Tet. Only one vertex can be violated at
// a time, and which vertex is violated is stored.
//===============================================================
void CleaverMesherImp::checkIfQuadrupleViolatesVertices(Tet *tet)
{
    Vertex *quad = tet->quadruple;
    quad->violating = false;

    Vertex *verts[VERTS_PER_TET];
    HalfEdge *edges[EDGES_PER_TET];
    HalfFace *faces[FACES_PER_TET];

    m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

    vec3 v1 = verts[0]->pos();
    vec3 v2 = verts[1]->pos();
    vec3 v3 = verts[2]->pos();
    vec3 v4 = verts[3]->pos();
    vec3 q  = quad->pos();

    // check v1 - using edges e1, e2, e3
    if(!quad->violating){
        float t1 = edges[0]->alphaForVertex(verts[0]);
        vec3 ev1 = (1 - t1)*v1 + t1*v2;
        vec3  n1 = normalize(cross(v3 - ev1, v4 - ev1));
        vec3  q1 = q - ev1;
        double d1 = dot(n1, q1);

        float t2 = edges[1]->alphaForVertex(verts[0]);
        vec3 ev2 = (1 - t2)*v1 + t2*v3;
        vec3  n2 = normalize(cross(v4 - ev2, v2 - ev2));
        vec3  q2 = q - ev2;
        double d2 = dot(n2, q2);

        float t3 = edges[2]->alphaForVertex(verts[0]);
        vec3 ev3 = (1 - t3)*v1 + t3*v4;                            // edge violation crosspoint
        vec3  n3 = normalize(cross(v2 - ev3, v3 - ev3));           // normal to plane
        vec3  q3 = q - ev3;                                        // quadruple in locall coordinate fram
        double d3 = dot(n3, q3);                                   // distance from quad to plane

        if(d1 < 0 && d2 < 0 && d3 < 0){
            quad->violating = true;
            quad->closestGeometry = verts[0];
        }
    }

      // check v2 - using edges e1(v1), e4(v3), e6(v4)
    if(!quad->violating){
        float t1 = edges[0]->alphaForVertex(verts[1]);
        vec3 ev1 = (1 - t1)*v2 + t1*v1;
        vec3  n1 = normalize(cross(v4 - ev1, v3 - ev1));
        vec3  q1 = q - ev1;
        double d1 = dot(n1, q1);

        float t2 = edges[5]->alphaForVertex(verts[1]);
        vec3 ev2 = (1 - t2)*v2 + t2*v4;
        vec3  n2 = normalize(cross(v3 - ev2, v1 - ev2));
        vec3  q2 = q - ev2;
        double d2 = dot(n2, q2);

        float t3 = edges[3]->alphaForVertex(verts[1]);
        vec3 ev3 = (1 - t3)*v2 + t3*v3;
        vec3  n3 = normalize(cross(v1 - ev3, v4 - ev3));
        vec3  q3 = q - ev3;
        double d3 = dot(n3, q3);

        if(d1 < 0 && d2 < 0 && d3 < 0){
            quad->violating = true;
            quad->closestGeometry = verts[1];
        }
    }

    // check v3 - using edges e2, e4, e5
    if(!quad->violating){
        double t1 = edges[1]->alphaForVertex(verts[2]);
        vec3 ev1 = (1 - t1)*v3  + t1*v1;
        vec3  n1 = normalize(cross(v2 - ev1, v4 - ev1));
        vec3  q1 = q - ev1;
        double d1 = dot(n1, q1);

        double t2 = edges[4]->alphaForVertex(verts[2]);
        vec3 ev2 = (1 - t2)*v3  + t2*v4;
        vec3  n2 = normalize(cross(v1 - ev2, v2 - ev2));
        vec3  q2 = q - ev2;
        double d2 = dot(n2, q2);

        double t3 = edges[3]->alphaForVertex(verts[2]);
        vec3 ev3 = (1 - t3)*v3  + t3*v2;
        vec3  n3 = normalize(cross(v4 - ev3, v1 - ev3));
        vec3  q3 = q - ev3;
        double d3 = dot(n3, q3);

        if(d1 < 0 && d2 < 0 && d3 < 0){
            quad->violating = true;
            quad->closestGeometry = verts[2];
        }
    }

     // check v4 - using edges e3, e5, e6
    if(!quad->violating){
        double t1 = edges[2]->alphaForVertex(verts[3]);
        vec3 ev1 = (1 - t1)*v4  + t1*v1;
        vec3  n1 = normalize(cross(v3 - ev1, v2 - ev1));
        vec3  q1 = q - ev1;
        double d1 = dot(n1, q1);

        double t2 = edges[4]->alphaForVertex(verts[3]);
        vec3 ev2 = (1 - t2)*v4  + t2*v3;
        vec3  n2 = normalize(cross(v2 - ev2, v1 - ev2));
        vec3  q2 = q - ev2;
        double d2 = dot(n2, q2);

        double t3 = edges[5]->alphaForVertex(verts[3]);
        vec3 ev3 = (1 - t3)*v4 + t3*v2;
        vec3  n3 = normalize(cross(v1 - ev3, v3 - ev3));
        vec3  q3 = q - ev3;
        double d3 = dot(n3, q3);

        if(d1 < 0 && d2 < 0 && d3 < 0){
            quad->violating = true;
            quad->closestGeometry = verts[3];
        }
    }
}


//---------------------------------------------------------------
//  checkIfTripleViolatesEdges()
//
//  This method checks whether a triple violates either of 3 edges
// that surround its face. This is used in the second phase of
// the warping algorithm.
//---------------------------------------------------------------
void CleaverMesherImp::checkIfTripleViolatesEdges(HalfFace *face)
{
    // Return immediately if triple doesn't exist
    if(!face->triple || face->triple->order() != TRIP)
        return;

    Vertex *triple = face->triple;
    triple->violating = false;

    double d[3];
    double d_min = 100000;
    bool violating[3] = {0};

    Vertex   *verts[3];
    HalfEdge *edges[3];

    m_bgMesh->getAdjacencyListsForFace(face, verts, edges);

    Vertex *v1 = verts[0];
    Vertex *v2 = verts[1];
    Vertex *v3 = verts[2];

    vec3 p1 = verts[0]->pos();
    vec3 p2 = verts[1]->pos();
    vec3 p3 = verts[2]->pos();
    vec3 trip = triple->pos();

    // check violating edge0
    {
        vec3 e1 = normalize(p3 - p2);
        vec3 e2 = normalize(p2 - p3);
        vec3 t1 = normalize(trip - p2);
        vec3 t2 = normalize(trip - p3);

        double alpha1 = edges[1]->alphaForVertex(v3);
        double alpha2 = edges[2]->alphaForVertex(v2);

        vec3 c1 = p3*(1 - alpha1) + alpha1*p1;
        vec3 c2 = p2*(1 - alpha2) + alpha2*p1;

        c1 = normalize(c1 - p2);
        c2 = normalize(c2 - p3);

        if(dot(e1, t1) > dot(e1, c1) ||
           dot(e2, t2) > dot(e2, c2)){

            double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
            double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

            if(dot1 > dot2)
                d[0] = acos(dot1);
            else
                d[0] = acos(dot2);

            violating[0] = true;
        }
    }

    // check violating edge1
    {
        vec3 e1 = normalize(p3 - p1);
        vec3 e2 = normalize(p1 - p3);
        vec3 t1 = normalize(trip - p1);
        vec3 t2 = normalize(trip - p3);

        double alpha1 = edges[0]->alphaForVertex(v3);
        double alpha2 = edges[2]->alphaForVertex(v1);

        vec3 c1 = p3*(1 - alpha1) + alpha1*p2;
        vec3 c2 = p1*(1 - alpha2) + alpha2*p2;

        c1 = normalize(c1 - p1);
        c2 = normalize(c2 - p3);

        if(dot(e1, t1) > dot(e1, c1) ||
           dot(e2, t2) > dot(e2, c2)){

            double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
            double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

            if(dot1 > dot2)
                d[1] = acos(dot1);
            else
                d[1] = acos(dot2);

            violating[1] = true;
        }
    }

    // check violating edge2
    {
        vec3 e1 = normalize(p2 - p1);
        vec3 e2 = normalize(p1 - p2);
        vec3 t1 = normalize(trip - p1);
        vec3 t2 = normalize(trip - p2);

        double alpha1 = edges[0]->alphaForVertex(v2);
        double alpha2 = edges[1]->alphaForVertex(v1);

        vec3 c1 = p2*(1 - alpha1) + alpha1*p3;
        vec3 c2 = p1*(1 - alpha2) + alpha2*p3;

        c1 = normalize(c1 - p1);
        c2 = normalize(c2 - p2);

        if(dot(e1, t1) > dot(e1, c1) ||
           dot(e2, t2) > dot(e2, c2)){

            double dot1 = clamp(dot(e1, t1), -1.0, 1.0);
            double dot2 = clamp(dot(e2, t2), -1.0, 1.0);

            if(dot1 > dot2)
                d[2] = acos(dot1);
            else
                d[2] = acos(dot2);

            violating[2] = true;
        }
    }

    // compare violatings, choose minimum
    for(int i=0; i < 3; i++){
        if(violating[i] && d[i] < d_min){
            triple->violating = true;
            triple->closestGeometry = edges[i];
            d_min = d[i];
        }
    }
}

//---------------------------------------------------------------
//  checkIfQuadrupleViolatesEdges()
//---------------------------------------------------------------
void CleaverMesherImp::checkIfQuadrupleViolatesEdges(Tet *tet)
{

}

//---------------------------------------------------------------
//  checkIfQuadrupleViolatesFaces()
//---------------------------------------------------------------
void CleaverMesherImp::checkIfQuadrupleViolatesFaces(Tet *tet)
{

}



//---------------------------------------------------
//  generalizeTets()
//---------------------------------------------------
void CleaverMesherImp::generalizeTets(bool verbose)
{
    if(verbose)
        std::cout << "Generalizing Tets..." << std::flush;

    //--------------------------------------
    // Loop over all tets that contain cuts
    //--------------------------------------
    // (For Now, Looping over ALL tets)

    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        cleaver::Tet *tet = m_bgMesh->tets[t];

        //------------------------------
        // if no quad, start generalization
        //------------------------------
        if(tet && !tet->quadruple)
        {
            // look up generalization
            Vertex *verts[4];
            HalfEdge *edges[6];
            HalfFace *faces[4];

            m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

            int cut_count = 0;
            for(int e=0; e < 6; e++)
                cut_count += (edges[e]->cut && (edges[e]->cut->order() == CUT) ? 1 : 0);


            //------------------------------
            // determine virtual edge cuts
            //------------------------------
            for(int e=0; e < 6; e++){
                if(edges[e]->cut == NULL)
                {
                    // always go towards the smaller index
                    if(edges[e]->vertex->tm_v_index < edges[e]->mate->vertex->tm_v_index)
                        edges[e]->cut = edges[e]->vertex;
                    else
                        edges[e]->cut = edges[e]->mate->vertex;

                    // copy info to mate edge
                    edges[e]->mate->cut = edges[e]->cut;
                }
            }


            //------------------------------
            // determine virtual face cuts
            //------------------------------
            for(int f=0; f < 4; f++)
            {
                if(faces[f]->triple == NULL)
                {
                    HalfEdge *e[3];
                    Vertex   *v[3];
                    m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

                    int v_count = 0;
                    int v_e = 0;
                    for(int i=0; i < 3; i++)
                    {
                        if(e[i]->cut->order() != CUT)
                        {
                            v_count++;
                            v_e = i;   // save index to virtual edge
                        }
                    }

                    // move to edge virtual cut went to
                    if(v_count == 1)
                    {
                        for(int i=0; i < 3; i++)
                        {
                            // skip edge that has virtual cut
                            if(i == v_e)
                                continue;

                            if(e[i]->vertex == e[v_e]->cut || e[i]->mate->vertex == e[v_e]->cut)
                            {
                                faces[f]->triple = e[i]->cut;
                                break;
                            }
                        }
                    }
                    // move to minimal index vertex
                    else if(v_count == 3)
                    {
                        if((v[0]->tm_v_index < v[1]->tm_v_index) && v[0]->tm_v_index < v[2]->tm_v_index)
                            faces[f]->triple = v[0];
                        else if((v[1]->tm_v_index < v[2]->tm_v_index) && v[1]->tm_v_index < v[0]->tm_v_index)
                            faces[f]->triple = v[1];
                        else
                            faces[f]->triple = v[2];
                    }
                    else
                    {
                        std::cerr << "HUGE PROBLEM: virtual count = " << v_count << std::endl;
                        for(int j=0; j < 3; j++){
                            if(v[j]->isExterior)
                                std::cout << "But it's Exterior!" << std::endl;
                        }

                        exit(8);
                    }

                    // copy info to mate face if it exists
                    if(faces[f]->mate)
                        faces[f]->mate->triple = faces[f]->triple;
                }
            }

            //------------------------------
            // determine virtual quadruple
            //------------------------------
            if(cut_count == 3)
            {
                if(faces[0]->triple == faces[1]->triple ||
                   faces[0]->triple == faces[2]->triple ||
                   faces[0]->triple == faces[3]->triple)
                    tet->quadruple = faces[0]->triple;
                else if(faces[1]->triple == faces[2]->triple ||
                        faces[1]->triple == faces[3]->triple)
                    tet->quadruple = faces[1]->triple;
                else if(faces[2]->triple == faces[3]->triple)
                    tet->quadruple = faces[2]->triple;
            }
            else if(cut_count == 4)
            {
                for(int f=0; f < 4; f++)
                {
                    if(faces[f]->triple->order() < TRIP && (faces[(f+1)%4]->triple == faces[f]->triple ||
                                                            faces[(f+2)%4]->triple == faces[f]->triple ||
                                                            faces[(f+3)%4]->triple == faces[f]->triple))
                    {
                        tet->quadruple = faces[f]->triple;
                        break;
                    }
                }
            }
            else if(cut_count == 5)
            {
                for(int f=0; f < 4; f++)
                {
                    if(faces[f]->triple->order() == TRIP)
                    {
                        tet->quadruple = faces[f]->triple;
                        break;
                    }
                }
            }
            else // 0
            {
                for(int f=0; f < 4; f++)
                {
                    if(faces[f]->triple->order() < TRIP)
                    {
                        tet->quadruple = faces[f]->triple;
                        break;
                    }
                }
            }

            if(tet->quadruple == NULL)
            {
                std::cerr << "Generalization Failed!!" << std::endl;
                std::cerr << "problem tet contains " << cut_count << " cuts." << std::endl;
            }

            if(tet->quadruple->order() < 0)
                std::cerr << "GOT YA!" << std::endl;

            /*
            // begin alternative approach (appears to not work properly)
            //------------------------------
            // determine virtual edge cuts
            //------------------------------
            for(int e=0; e < 6; e++)
            {
                if(edges[e]->cut == NULL)
                {
                    // todo, precompute parity or remove from edge storage
                    bool parity = (edges[e]->vertex->tm_v_index > edges[e]->mate->vertex->tm_v_index);
                    if(parity){
                        edges[e]->parity       = true;
                        edges[e]->cut       = edges[e]->vertex;
                        edges[e]->mate->cut = edges[e]->vertex;
                    }
                    else{
                        edges[e]->mate->parity = true;
                        edges[e]->cut       = edges[e]->mate->vertex;
                        edges[e]->mate->cut = edges[e]->mate->vertex;
                    }
                }
            }

            //------------------------------
            // determine virtual face cuts
            //------------------------------
            for(int f=0; f < 4; f++)
            {
                if(faces[f]->triple == NULL)
                {
                    HalfEdge *fedges[3];
                    Vertex   *fverts[3];

                    m_bgMesh->getAdjacencyListsForFace(faces[f], fverts, fedges);

                    int hole_count=0,index=0;
                    for(int e=0; e < 3; e++)
                    {
                        if(fedges[e]->cut->order() == VERT){
                            hole_count++;
                            index = e;
                        }
                    }
                    // if 0-cuts, move to highest ranking vert
                    if(hole_count == 3)
                    {
                        int max_count = 0;
                        for(int v=0; v < 3; v++)
                        {
                            if(fverts[v]->tm_v_index > fverts[max_count]->tm_v_index)
                                max_count = v;
                        }

                        faces[f]->triple = fverts[max_count];
                    }
                    // else look at parity on the zero order edge
                    else if(hole_count == 1)
                    {
                        if(fedges[index]->parity)
                            faces[f]->triple = fedges[(index+1)%3]->cut;
                        else
                            faces[f]->triple = fedges[(index+2)%3]->cut;
                    }
                    else{
                        std::cerr << "Fatal Error: Impossible Cut Count (hole_count = " << hole_count << ")" << std::endl;
                    }
                }
            }

            //------------------------------
            // determine virtual quadruple
            //------------------------------
            if(tet->quadruple == NULL)
            {              
                std::map<Vertex*, int> triple_counts;
                for(int f=0; f < 4; f++)
                    triple_counts[faces[f]->triple] = 0;
                for(int f=0; f < 4; f++)
                    triple_counts[faces[f]->triple] += 1;
                Vertex *destination = NULL;
                int max_count = -1;
                for(std::map<Vertex*,int>::iterator ii=triple_counts.begin(); ii != triple_counts.end(); ++ii)
                {
                    if(ii->second > max_count){
                        max_count = ii->second;
                        destination = ii->first;
                    }
                }                

                tet->quadruple = destination;             
            }
            */

            // if 0-cut, move to vertex with most triples (3?)
            // if 3-cut, move to edge with most triples (2)
            // if 4-cut, move to edge with most triples (2)
            // if 5-cut, move to face with both triples on its edges (2)
            // if 6-cut, there should already exist a quadpoint
            // Q: Can these rules fit into a single algorithm?
            // A: Count coinciding triples, take one that repeats most

            /* old code below
            if(tet->quadruple == NULL)
            {
                int hole_count=0,index=0;
                for(int f=0; f < 4; f++)
                {
                    if(faces[f]->triple->order() == VERT){
                        hole_count++;
                        index = f;
                    }
                }
                // if 0-triples, move to highest ranking vert
                if(hole_count == 4)
                {
                    int max_count = 0;
                    for(int v=0; v < 4; v++)
                    {
                        if(verts[v]->tm_v_index > verts[max_count]->tm_v_index)
                            max_count = v;
                    }

                    tet->quadruple = verts[max_count];
                }
                else{

                }
            }
            */
        }
    }

    // set state
    m_bGeneralized = true;

    if(verbose)
        std::cout << " done." << std::endl;
}

//=======================================
// generalizeTopologicalTets()
//=======================================
void CleaverMesherImp::generalizeTopologicalTets(bool verbose)
{
    if(verbose)
        std::cout << "Generalizing Tets..." << std::flush;

    //--------------------------------------
    // Loop over all tets that contain cuts
    //--------------------------------------
    // (For Now, Looping over ALL tets)

    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        cleaver::Tet *tet = m_bgMesh->tets[t];
        bool case_is_v2 = false;

        //------------------------------
        // if no quad, start generalization
        //------------------------------
        //if(tet && !tet->quadruple)
        {
            // look up generalization
            Vertex *verts[4];
            HalfEdge *edges[6];
            HalfFace *faces[4];

            m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

            int cut_count = 0;
            for(int e=0; e < 6; e++)
                cut_count += ((edges[e]->cut && (edges[e]->cut->order() == CUT)) ? 1 : 0);  // to do, this bug probably appaers in regular
                                                                                            // generalization function, fix it!
            //------------------------------
            // determine virtual edge cuts
            //------------------------------
            for(int e=0; e < 6; e++){
                if(edges[e]->cut == NULL)
                {
                    // always go towards the smaller index
                    if(edges[e]->vertex->tm_v_index < edges[e]->mate->vertex->tm_v_index)
                        edges[e]->cut = edges[e]->vertex;
                    else
                        edges[e]->cut = edges[e]->mate->vertex;

                    // copy info to mate edge
                    edges[e]->mate->cut = edges[e]->cut;
                }
            }

            //------------------------------
            // determine virtual face cuts
            //------------------------------
            for(int f=0; f < 4; f++)
            {
                if(faces[f]->triple == NULL)
                {
                    HalfEdge *e[3];
                    Vertex   *v[3];
                    m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

                    int virtual_count = 0;
                    int v_e = 0;
                    for(int i=0; i < 3; i++)
                    {
                        if(e[i]->cut->order() != CUT)
                        {
                            virtual_count++;
                            v_e = i;   // save index to virtual edge
                        }
                    }
                    // 3 cuts, but no triple? Make one
                    if(virtual_count == 0)
                    {
                        std::cout << "got a 3-cuts but no triple case" << std::endl;
                        exit(19);
                        // technically this case should never happen, but if
                        // it does, let's just make a triple point in the center
                        //faces[f]->triple = e[0]->cut;
                        faces[f]->triple = new Vertex(m_volume->numberOfMaterials());
                        faces[f]->triple->pos() = (1.0/3.0)*(v[0]->pos() + v[1]->pos() + v[2]->pos());
                        faces[f]->triple->order() = TRIP;
                        faces[f]->triple->lbls[v[0]->label] = true;
                        faces[f]->triple->lbls[v[1]->label] = true;
                        faces[f]->triple->lbls[v[2]->label] = true;
                        faces[f]->triple->violating = false;
                        faces[f]->triple->closestGeometry = NULL;
                        if(faces[f]->mate)
                            faces[f]->mate->triple = faces[f]->triple;
                    }
                    // move triple to edge virtual cut went to
                    else if(virtual_count == 1)
                    {
                        for(int i=0; i < 3; i++)
                        {
                            // skip edge that has virtual cut
                            if(i == v_e)
                                continue;

                            if(e[i]->vertex == e[v_e]->cut || e[i]->mate->vertex == e[v_e]->cut)
                            {
                                faces[f]->triple = e[i]->cut;
                                if(faces[f]->mate)
                                    faces[f]->mate->triple = e[i]->cut;
                                break;
                            }                           
                        }
                        if(faces[f]->triple == NULL){
                            std::cerr << "WTF??" << std::endl;
                            exit(12412);
                        }
                    }
                    // move triple to the vertex opposite the real cut
                    else if(virtual_count == 2)
                    {
                        //if(cut_count == 1 || cut_count == 2 || cut_count == 3){
                        for(int i=0; i < 3; i++){
                            if(e[i]->cut && e[i]->cut->order() == CUT)
                            {
                                Vertex *v1 = e[i]->vertex;
                                Vertex *v2 = e[i]->mate->vertex;
                                Vertex *op;

                                HalfEdge *en1 = e[(i+1)%3];
                                HalfEdge *en2 = e[(i+2)%3];
                                if(en1->vertex != v1 && en1->vertex != v2)
                                    op = en1->vertex;
                                else
                                    op = en1->mate->vertex;

                                // if both virtual cuts went to vertex
                                // opposite the edge with real cut, go there
                                /*
                                if(en1->cut == op || en2->cut == op)
                                {
                                    faces[f]->triple = op;
                                    if(faces[f]->mate)
                                        faces[f]->mate->triple = op;
                                }
                                // otherwise go to the edge cut
                                */
                                //else
                                {
                                    faces[f]->triple = e[i]->cut;
                                    if(faces[f]->mate)
                                        faces[f]->mate->triple = e[i]->cut;
                                }
                                if(faces[f]->triple == NULL){
                                    std::cerr << "triple block with vcount=2 failed!" << std::endl;
                                    exit(12321);
                                }

                                break;
                            }
                        //}
                        }
                        case_is_v2 = true;
                    }
                    // move triple to minimal index vertex
                    else if(virtual_count == 3)
                    {
                        if((v[0]->tm_v_index < v[1]->tm_v_index) && v[0]->tm_v_index < v[2]->tm_v_index)
                            faces[f]->triple = v[0];
                        else if((v[1]->tm_v_index < v[2]->tm_v_index) && v[1]->tm_v_index < v[0]->tm_v_index)
                            faces[f]->triple = v[1];
                        else
                            faces[f]->triple = v[2];
                    }

                    // copy info to mate face if it exists
                    if(faces[f]->mate)
                        faces[f]->mate->triple = faces[f]->triple;
                }

                if(cut_count == 2 && faces[f]->triple == NULL)
                {
                    std::cerr << "Finding a  triple failed for 2-cut tet" << std::endl;
                    exit(12412);
                }
            }

            //------------------------------
            // determine virtual quadruple
            //------------------------------
            if(tet->quadruple == NULL)
            {

                if(cut_count == 1)
                {
                    // put quad on the cut edge
                    for(int e=0; e < 6; e++){
                        if(edges[e]->cut && edges[e]->cut->order() == CUT)
                            tet->quadruple = edges[e]->cut;
                    }
                    // Just make it and see if this finishes
                    /*
                    tet->quadruple = new Vertex(m_volume->numberOfMaterials());
                    tet->quadruple->pos() = (1.0/4.0)*(verts[0]->pos() + verts[1]->pos() + verts[2]->pos() + verts[3]->pos());
                    tet->quadruple->order() = QUAD;
                    tet->quadruple->lbls[verts[0]->label] = true;
                    tet->quadruple->lbls[verts[1]->label] = true;
                    tet->quadruple->lbls[verts[2]->label] = true;
                    tet->quadruple->lbls[verts[3]->label] = true;
                    tet->quadruple->violating = false;
                    tet->quadruple->closestGeometry = NULL;
                    */
                }
                else if(cut_count == 2)
                {
                    // put quad on the cut that was chosen by triple
                    for(int f=0; f < 4; f++){
                        int count = 0;
                        HalfEdge *e[3];
                        Vertex   *v[3];
                        m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

                        for(int i=0; i < 3; i++)
                        {
                            if(e[i]->cut->order() == CUT)
                            {
                                count++;
                            }
                        }
                        // found the face
                        if(count == 2){
                            tet->quadruple = faces[f]->triple;
                            if(tet->quadruple == NULL){
                                std::cerr << "PROBLEM!" << std::endl;
                                exit(12412);
                            }
                            break;
                        }
                    }
                    if(tet->quadruple == NULL){
                        //std::cerr << "looks like 2 cuts aren't on incident edges" << std::endl;
                        //exit(1412);

                        // put on either cut, both should have 2 triples on them
                        tet->quadruple = faces[0]->triple;

                    }

                    // Just make it and see if this finishes
                    /*
                    tet->quadruple = new Vertex(m_volume->numberOfMaterials());
                    tet->quadruple->pos() = (1.0/4.0)*(verts[0]->pos() + verts[1]->pos() + verts[2]->pos() + verts[3]->pos());
                    tet->quadruple->order() = QUAD;
                    tet->quadruple->lbls[verts[0]->label] = true;
                    tet->quadruple->lbls[verts[1]->label] = true;
                    tet->quadruple->lbls[verts[2]->label] = true;
                    tet->quadruple->lbls[verts[3]->label] = true;
                    tet->quadruple->violating = false;
                    tet->quadruple->closestGeometry = NULL;
                    */
                }
                else if(cut_count == 3)
                {
                    if(faces[0]->triple == faces[1]->triple ||
                            faces[0]->triple == faces[2]->triple ||
                            faces[0]->triple == faces[3]->triple)
                        tet->quadruple = faces[0]->triple;
                    else if(faces[1]->triple == faces[2]->triple ||
                            faces[1]->triple == faces[3]->triple)
                        tet->quadruple = faces[1]->triple;
                    else if(faces[2]->triple == faces[3]->triple)
                        tet->quadruple = faces[2]->triple;
                    else{

                        // Just make it and see if this finishes
                        /*
                        tet->quadruple = new Vertex(m_volume->numberOfMaterials());
                        tet->quadruple->pos() = (1.0/4.0)*(verts[0]->pos() + verts[1]->pos() + verts[2]->pos() + verts[3]->pos());
                        tet->quadruple->order() = QUAD;
                        tet->quadruple->lbls[verts[0]->label] = true;
                        tet->quadruple->lbls[verts[1]->label] = true;
                        tet->quadruple->lbls[verts[2]->label] = true;
                        tet->quadruple->lbls[verts[3]->label] = true;
                        tet->quadruple->violating = false;
                        tet->quadruple->closestGeometry = NULL;
                        */

                        // take quadruple to central edge of connected component of cut edges
                        /*
                        HalfEdge *pe[3]; int idx=0;

                        for(int e=0; e < 6; e++){
                            if(edges[e]->cut->order() == CUT)
                                pe[idx++] = edges[e];
                        }
                        for(int e=0; e < 3; e++){
                            int common_count = 0;
                            if(pe[e]->vertex == pe[(e+1)%3]->vertex)
                                common_count++;
                            if(pe[e]->mate->vertex == pe[(e+1)%3]->vertex)
                                common_count++;
                            if(pe[e]->vertex == pe[(e+1)%3]->mate->vertex)
                                common_count++;
                            if(pe[e]->mate->vertex == pe[(e+1)%3]->mate->vertex)
                                common_count++;
                            if(pe[e]->vertex == pe[(e+2)%3]->vertex)
                                common_count++;
                            if(pe[e]->mate->vertex == pe[(e+2)%3]->vertex)
                                common_count++;
                            if(pe[e]->vertex == pe[(e+2)%3]->mate->vertex)
                                common_count++;
                            if(pe[e]->mate->vertex == pe[(e+2)%3]->mate->vertex)
                                common_count++;

                            if(common_count == 2){
                                tet->quadruple = pe[e]->cut;
                                break;
                            }
                        }
                        */
                        //
                    }
                }
                else if(cut_count == 4)
                {
                    bool found = false;
                    for(int f=0; f < 4; f++)
                    {
                        if(faces[f]->triple->order() < TRIP && (faces[(f+1)%4]->triple == faces[f]->triple ||
                                                                faces[(f+2)%4]->triple == faces[f]->triple ||
                                                                faces[(f+3)%4]->triple == faces[f]->triple))
                        {
                            tet->quadruple = faces[f]->triple;
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        // there's no 2 triples co-located, select any of them
                        tet->quadruple = faces[0]->triple;
                    }
                }
                else if(cut_count == 5)
                {
                    bool found = false;
                    for(int f=0; f < 4; f++)
                    {
                        if(faces[f]->triple->order() == TRIP)
                        {
                            tet->quadruple = faces[f]->triple;
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        // there's no real triple, select any of them
                        tet->quadruple = faces[0]->triple;
                    }
                }
                else // 0
                {
                    for(int f=0; f < 4; f++)
                    {
                        if(faces[f]-> triple && faces[f]->triple->order() < TRIP)
                        {
                            tet->quadruple = faces[f]->triple;
                            break;
                        }
                    }
                }
            }



            if(!(cut_count == 0 ||
                 cut_count == 1 ||
                 cut_count == 2 ||
                 cut_count == 3 ||
                 cut_count == 4 ||
                 cut_count == 5 ||
                 cut_count == 6))
            {
                tet->quadruple = NULL;
            }

            if(cut_count == 3){
                /*
                std::cerr << "3-cuts: ";
                for(int e=0; e < 6; e++){
                    if(edges[e]->cut->order() == CUT)
                        std::cerr << "c" << e+1 << ", ";
                }
                std::cerr << std::endl;
                */

                bool around_v1 = false;
                bool around_v2 = false;
                bool around_v3 = false;
                bool around_v4 = false;

                if(edges[0]->cut->order() == CUT && edges[1]->cut->order() == CUT && edges[2]->cut->order() == CUT)
                    around_v1 = true;
                if(edges[0]->cut->order() == CUT && edges[3]->cut->order() == CUT && edges[4]->cut->order() == CUT)
                    around_v2 = true;
                if(edges[1]->cut->order() == CUT && edges[3]->cut->order() == CUT && edges[5]->cut->order() == CUT)
                    around_v3 = true;
                if(edges[2]->cut->order() == CUT && edges[4]->cut->order() == CUT && edges[5]->cut->order() == CUT)
                    around_v4 = true;

                //if(/*!around_v1 &&*/ !around_v2 && !around_v3 && !around_v4)
                //    tet->quadruple = NULL;
            }

            /*
            if(false && case_is_v2)
            {
                if(true && cut_count != 1){
                    tet->quadruple = NULL;
                    for(int e=0; e < 6; e++){
                        if(edges[e]->cut->order() != CUT){
                            edges[e]->cut = NULL;
                            edges[e]->mate->cut = NULL;
                        }
                    }
                    for(int f=0; f < 4; f++){
                        if(faces[f]->triple->order() != TRIP){
                            faces[f]->triple = NULL;
                            if(faces[f]->mate)
                                faces[f]->mate->triple = NULL;
                        }
                    }

                }
                else{
                    std::cerr << "2 virtual cuts on face case: " << std::endl;
                    std::cerr << "v1, v2, v3, v4, ";
                    for(int e=0; e < 6; e++){
                        if(edges[e]->cut->order() == CUT)
                            std::cerr << "c" << e+1 << ", ";
                    }
                    for(int f=0; f < 4; f++){
                        if(faces[f]->triple && faces[f]->triple->order() == TRIP)
                            std::cerr << "t" << t+1 << ", ";
                    }
                    std::cerr << std::endl;
                }
            }
            */


            if(tet->quadruple == NULL)
            {
                std::cerr << "Generalization Failed!!" << std::endl;
                std::cerr << "problem tet contains " << cut_count << " cuts." << std::endl;

                std::cerr << "v1, v2, v3, v4, ";
                for(int e=0; e < 6; e++){
                    if(edges[e]->cut && edges[e]->cut->order() == CUT)
                        std::cerr << "c" << e+1 << ", ";
                }
                for(int f=0; f < 4; f++){
                    if(faces[f]->triple && faces[f]->triple->order() == TRIP)
                        std::cerr << "t" << t+1 << ", ";
                }
                std::cerr << std::endl;
                for(int f=0; f < 4; f++){
                    if(!faces[f]->triple)
                        std::cout << "Missing triple T" << f+1 << std::endl;
                }
            }
            for(int f=0; f < 4; f++)
            {
                if(faces[f]->triple == NULL){
                    std::cerr << "Generalization Failed!!" << std::endl;
                    std::cerr << "missing a triple" << std::endl;
                }
            }
            for(int e=0; e < 6; e++)
            {
                if(edges[e]->cut == NULL){
                    std::cerr << "Generalization Failed!!" << std::endl;
                    std::cerr << "missing a cut" << std::endl;
                }
            }

        }
    }


    // set state
    m_bGeneralized = true;

    if(verbose)
        std::cout << " done." << std::endl;
}

//=======================================
//    Executes all Snaps and Warps
//=======================================
void CleaverMesherImp::snapAndWarpViolations(bool verbose)
{
    if(verbose)
        std::cout << "Beginning Snapping and Warping..." << std::endl;

    //------------------------
    //     Snap To Verts
    //------------------------
    snapAndWarpVertexViolations(verbose);

    //------------------------
    //     Snap to Edges
    //------------------------
    snapAndWarpEdgeViolations(verbose);

    //------------------------
    //     Snap to Faces
    //------------------------
    snapAndWarpFaceViolations(verbose);


    // set state
    m_bSnapsAndWarpsDone = true;

    if(verbose)
        std::cout << "Snapping/warping complete." << std::endl;
}

//==============================================================
// Snap and Warp All Vertex Violations
//==============================================================
void CleaverMesherImp::snapAndWarpVertexViolations(bool verbose)
{
    //---------------------------------------------------
    //  Apply vertex warping to all vertices in lattice
    //---------------------------------------------------
    std::cout << "preparing to examine " << m_bgMesh->verts.size() << " verts" << std::endl;
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        Vertex *vertex = m_bgMesh->verts[v];            // TODO: add check for vertex->hasAdjacentCuts
        snapAndWarpForViolatedVertex(vertex);           //       to reduce workload significantly.
    }

    std::cout << "Phase 1 Complete" << std::endl;
}



//=========================================================================
//   Conform Triple Point to Remain in the Triangle Face after Warp.
//=========================================================================
void CleaverMesherImp::conformTriple(HalfFace *face, Vertex *warpVertex, const vec3 &warpPt)
{
    double EPS = 1E-3;

    //-------------------------------------
    //    Compute Barycentric Coordinates
    //-------------------------------------

    Vertex *trip = face->triple;
    Vertex *verts[VERTS_PER_FACE];
    HalfEdge *edges[EDGES_PER_FACE];

    m_bgMesh->getAdjacencyListsForFace(face, verts, edges);

    for (int i=0; i < 3; i++)
    {
        if (verts[i] == warpVertex)
        {
            // swap so moving vertex is first in list
            verts[i] = verts[0];
            verts[0] = warpVertex;
            break;
        }
    }

    Vertex *dv1 = verts[0];
    Vertex *dv2 = verts[1];
    Vertex *dv3 = verts[2];

    vec3 p1 = dv1->pos();
    vec3 p2 = dv2->pos();
    vec3 p3 = dv3->pos();

    // Create Matrix A and solve for Inverse
    double A[3][3];
    vec3 triple = trip->pos_next();   // was ->pos  8/11/11
    vec3 inv1,inv2,inv3;
    vec3 v1 = warpPt;
    vec3 v2 = verts[1]->pos();
    vec3 v3 = verts[2]->pos();
    vec3 v4 = v1 + normalize(cross(normalize(v3 - v1), normalize(v2 - v1)));

    // Fill Coordinate Matrix
    A[0][0] = v1.x - v4.x; A[0][1] = v2.x - v4.x; A[0][2] = v3.x - v4.x;
    A[1][0] = v1.y - v4.y; A[1][1] = v2.y - v4.y; A[1][2] = v3.y - v4.y;
    A[2][0] = v1.z - v4.z; A[2][1] = v2.z - v4.z; A[2][2] = v3.z - v4.z;

    // Solve Inverse
    double det = +A[0][0]*(A[1][1]*A[2][2]-A[2][1]*A[1][2])
                 -A[0][1]*(A[1][0]*A[2][2]-A[1][2]*A[2][0])
                 +A[0][2]*(A[1][0]*A[2][1]-A[1][1]*A[2][0]);
    double invdet = 1/det;
    inv1.x =  (A[1][1]*A[2][2]-A[2][1]*A[1][2])*invdet;
    inv2.x = -(A[1][0]*A[2][2]-A[1][2]*A[2][0])*invdet;
    inv3.x =  (A[1][0]*A[2][1]-A[2][0]*A[1][1])*invdet;

    inv1.y = -(A[0][1]*A[2][2]-A[0][2]*A[2][1])*invdet;
    inv2.y =  (A[0][0]*A[2][2]-A[0][2]*A[2][0])*invdet;
    inv3.y = -(A[0][0]*A[2][1]-A[2][0]*A[0][1])*invdet;

    inv1.z =  (A[0][1]*A[1][2]-A[0][2]*A[1][1])*invdet;
    inv2.z = -(A[0][0]*A[1][2]-A[1][0]*A[0][2])*invdet;
    inv3.z =  (A[0][0]*A[1][1]-A[1][0]*A[0][1])*invdet;

    // Multiply Inverse*Coordinate to get Lambda (Barycentric)
    vec3 lambda;
    lambda.x = inv1.x*(triple.x - v4.x) + inv1.y*(triple.y - v4.y) + inv1.z*(triple.z - v4.z);
    lambda.y = inv2.x*(triple.x - v4.x) + inv2.y*(triple.y - v4.y) + inv2.z*(triple.z - v4.z);
    lambda.z = inv3.x*(triple.x - v4.x) + inv3.y*(triple.y - v4.y) + inv3.z*(triple.z - v4.z);

    //--------------------------------------------------------------
    // Is any coordinate negative?
    // If so, make it 0, adjust other weights but keep ratio
    //--------------------------------------------------------------

    if(lambda.x < EPS){
        lambda.x = 0;

        for(int i=0; i < EDGES_PER_FACE; i++){
            if(edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[2])){
                trip->conformedEdge = edges[i];
                break;
            }
        }
    }
    else if(lambda.y < EPS){
        lambda.y = 0;

        for(int i=0; i < EDGES_PER_FACE; i++){
            if(edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[2])){
                trip->conformedEdge = edges[i];
                break;
            }
        }
    }
    else if(lambda.z < EPS){
        lambda.z = 0;

        for(int i=0; i < EDGES_PER_FACE; i++){
            if(edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[1])){
                trip->conformedEdge = edges[i];
                break;
            }
        }
    }
    else
    {
        trip->conformedEdge = NULL;
    }

    lambda /= L1(lambda);

    // Compute New Triple Coordinate
    triple.x = lambda.x*v1.x + lambda.y*v2.x + lambda.z*v3.x;
    triple.y = lambda.x*v1.y + lambda.y*v2.y + lambda.z*v3.y;
    triple.z = lambda.x*v1.z + lambda.y*v2.z + lambda.z*v3.z;

    if(triple == vec3::zero || triple != triple)
    {
        std::cerr << "Error Conforming Triple!" << std::endl;
        exit(-1);
    }

    face->triple->pos_next() = triple;
}

//======================================================================
// - conformQuadruple(Tet *tet, Vertex *warpVertex, const vec3 &warpPt)
//
// This method enforces that the location of a quad undergoing a warp
// remains in the interior of the bounding tetrahdron. If it does not,
// it is 'conformed' to a face, edge or vertex, by projecting any
// negative barycentric coordinates to be zero.
//======================================================================
void CleaverMesherImp::conformQuadruple(Tet *tet, Vertex *warpVertex, const vec3 &warpPt)
{
    double EPS = 1E-3;

    //-------------------------------------
    //    Compute Barycentric Coordinates
    //-------------------------------------
    Vertex *quad = tet->quadruple;
    Vertex *verts[VERTS_PER_TET];
    HalfEdge *edges[EDGES_PER_TET];
    HalfFace *faces[FACES_PER_TET];

    m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

    quad->conformedFace = NULL;
    quad->conformedEdge = NULL;
    quad->conformedVertex = NULL;

    for (int i=0; i < 4; i++)
    {
        if (verts[i] == warpVertex)
        {
            // swap so moving vertex is first in list
            verts[i] = verts[0];
            verts[0] = warpVertex;
            break;
        }
    }

    // Create Matrix A and solve for Inverse
    double A[3][3];
    vec3 quadruple = quad->pos();
    vec3 inv1,inv2,inv3;
    vec3 v1 = warpPt;
    vec3 v2 = verts[1]->pos();
    vec3 v3 = verts[2]->pos();
    vec3 v4 = verts[3]->pos();

    // Fill Coordinate Matrix
    A[0][0] = v1.x - v4.x; A[0][1] = v2.x - v4.x; A[0][2] = v3.x - v4.x;
    A[1][0] = v1.y - v4.y; A[1][1] = v2.y - v4.y; A[1][2] = v3.y - v4.y;
    A[2][0] = v1.z - v4.z; A[2][1] = v2.z - v4.z; A[2][2] = v3.z - v4.z;

    // Solve Inverse
    double det = +A[0][0]*(A[1][1]*A[2][2]-A[2][1]*A[1][2])
                 -A[0][1]*(A[1][0]*A[2][2]-A[1][2]*A[2][0])
                 +A[0][2]*(A[1][0]*A[2][1]-A[1][1]*A[2][0]);
    double invdet = 1/det;
    inv1.x =  (A[1][1]*A[2][2]-A[2][1]*A[1][2])*invdet;
    inv2.x = -(A[1][0]*A[2][2]-A[1][2]*A[2][0])*invdet;
    inv3.x =  (A[1][0]*A[2][1]-A[2][0]*A[1][1])*invdet;

    inv1.y = -(A[0][1]*A[2][2]-A[0][2]*A[2][1])*invdet;
    inv2.y =  (A[0][0]*A[2][2]-A[0][2]*A[2][0])*invdet;
    inv3.y = -(A[0][0]*A[2][1]-A[2][0]*A[0][1])*invdet;

    inv1.z =  (A[0][1]*A[1][2]-A[0][2]*A[1][1])*invdet;
    inv2.z = -(A[0][0]*A[1][2]-A[1][0]*A[0][2])*invdet;
    inv3.z =  (A[0][0]*A[1][1]-A[1][0]*A[0][1])*invdet;

    // Multiply Inverse*Coordinate to get Lambda (Barycentric)
    vec3 lambda;
    lambda.x = inv1.x*(quadruple.x - v4.x) + inv1.y*(quadruple.y - v4.y) + inv1.z*(quadruple.z - v4.z);
    lambda.y = inv2.x*(quadruple.x - v4.x) + inv2.y*(quadruple.y - v4.y) + inv2.z*(quadruple.z - v4.z);
    lambda.z = inv3.x*(quadruple.x - v4.x) + inv3.y*(quadruple.y - v4.y) + inv3.z*(quadruple.z - v4.z);

    //--------------------------------------------------------------
    // Is any coordinate negative?
    // If so, make it 0, adjust other weights but keep ratio
    //--------------------------------------------------------------

    double lambda_w = 1.0 - (lambda.x + lambda.y + lambda.z);

    if(lambda.x < EPS){

        // two negatives
        if(lambda.y < EPS){

            lambda.x = 0;
            lambda.y = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[2]) && edges[i]->incidentToVertex(verts[3])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        else if(lambda.z < EPS){

            lambda.x = 0;
            lambda.z = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[3])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        else if(lambda_w < EPS){

            lambda.x = 0;
            lambda_w = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[2])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        // one negative
        else{

            lambda.x = 0;

            for(int i=0; i < 4; i++)
            {
                if(!faces[i]->incidentToVertex(verts[0])){
                    quad->conformedFace = faces[i];
                    //cout << "Conformed to Face" << endl;
                    break;
                }
            }
        }

    }
    else if(lambda.y < EPS){
        // two negatives
        if(lambda.z < EPS){

            lambda.y = 0;
            lambda.z = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[3])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        else if(lambda_w < EPS){

            lambda.y = 0;
            lambda_w = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[2])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        // one negative
        else{

            lambda.y = 0;

            for(int i=0; i < 4; i++)
            {
                if(!faces[i]->incidentToVertex(verts[1])){
                    quad->conformedFace = faces[i];
                    //cout << "Conformed to Face" << endl;
                    break;
                }
            }
        }
    }
    else if(lambda.z < EPS){
        // two negatives
        if(lambda_w < EPS){

            lambda.z = 0;
            lambda_w = 0;

            for(int i=0; i < 6; i++){
                if(edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[1])){
                    quad->conformedEdge = edges[i];
                    //cout << "conformed to Edge" << endl;
                    break;
                }
            }
        }
        // one negative
        else{

            lambda.z = 0;

            for(int i=0; i < 4; i++)
            {
                if(!faces[i]->incidentToVertex(verts[2])){
                    quad->conformedFace = faces[i];
                    //cout << "Conformed to Face" << endl;
                    break;
                }
            }
        }
    }
    else if(lambda_w < EPS)
    {
        // one negative
        lambda_w = 0;

        for(int i=0; i < 4; i++)
        {
            if(!faces[i]->incidentToVertex(verts[3])){
                quad->conformedFace = faces[i];
                //cout << "Conformed to Face" << endl;
                break;
            }
        }
    }
    else
    {
        quad->conformedFace = NULL;
        quad->conformedEdge =  NULL;
        quad->conformedVertex = NULL;
    }

    if(quad->conformedVertex != NULL){
        std::cerr << "unhandled exception: quad->conformedVertex != NULL" << std::endl;
        exit(-1);
    }

    double L1 = lambda.x + lambda.y + lambda.z + lambda_w;
    lambda /= L1;

    // Compute New Triple Coordinate
    quadruple.x = lambda.x*v1.x + lambda.y*v2.x + lambda.z*v3.x + (1.0 - (lambda.x + lambda.y + lambda.z))*v4.x;
    quadruple.y = lambda.x*v1.y + lambda.y*v2.y + lambda.z*v3.y + (1.0 - (lambda.x + lambda.y + lambda.z))*v4.y;
    quadruple.z = lambda.x*v1.z + lambda.y*v2.z + lambda.z*v3.z + (1.0 - (lambda.x + lambda.y + lambda.z))*v4.z;

    quad->pos_next() = quadruple;
}

//=================================================================
//  Snap and Warp Violations Surrounding a Vertex
//=================================================================
void CleaverMesherImp::snapAndWarpForViolatedVertex(Vertex *vertex)
{
    std::vector<HalfEdge*>   viol_edges;      // violating cut edges
    std::vector<HalfFace*>   viol_faces;      // violating triple-points
    std::vector<Tet*>        viol_tets;       // violating quadruple-points

    std::vector<HalfEdge*>   part_edges;      // participating cut edges
    std::vector<HalfFace*>   part_faces;      // participating triple-points
    std::vector<Tet*>        part_tets;       // participating quadruple-points


    // TODO: This shouldn't here. It's a temporary fix
    //
    //  Explanation:  The algorithm loops through each vertex naively.
    //           If a warped edge moves an edge to violate some other
    //           vertex that has not yet been warped, it does nothing,
    //           knowing that vertex will take of it when it's evaluated.
    //      The problem is that vertex may have already been touched in
    //      the list, but simply had no violations so it wasn't warped.
    //      It will never be evaluated again, and the violation will remain.
    //  The PROPER fix would be to create a list of potential vertices that
    //  we pop off of, and we will push new vertices back onto the list if
    //  we think they might have recently become violated.
    //
    vertex->warped = true;
    //--------------------

    //---------------------------------------------------------
    //   Add Participating & Violating CutPoints  (Edges)
    //---------------------------------------------------------
    std::vector<HalfEdge*> incidentEdges = m_bgMesh->edgesAroundVertex(vertex);

    for(unsigned int e=0; e < incidentEdges.size(); e++)
    {
        HalfEdge *edge = incidentEdges[e];
        if(edge->cut->order() == CUT)
        {
            if(edge->cut->violating && edge->cut->closestGeometry == vertex)
                viol_edges.push_back(edge);
            else
                part_edges.push_back(edge);
        }
    }

    //---------------------------------------------------------
    // Add Participating & Violating TriplePoints   (Faces)
    //---------------------------------------------------------
    std::vector<HalfFace*> incidentFaces = m_bgMesh->facesAroundVertex(vertex);

    for(unsigned int f=0; f < incidentFaces.size(); f++)
    {
        HalfFace *face = incidentFaces[f];

        if(face->triple->order() == TRIP)
        {
            if(face->triple->violating && face->triple->closestGeometry == vertex)
                viol_faces.push_back(face);
            else
                part_faces.push_back(face);
        }
    }

    //---------------------------------------------------------
    // Add Participating & Violating QuaduplePoints   (Tets)
    //---------------------------------------------------------
    std::vector<Tet*> incidentTets = m_bgMesh->tetsAroundVertex(vertex);

    for(unsigned int t=0; t < incidentTets.size(); t++)
    {
        Tet *tet = incidentTets[t];
        if(tet->quadruple->order() == QUAD)
        {
            if(tet->quadruple->violating && tet->quadruple->closestGeometry == vertex)
                viol_tets.push_back(tet);
            else
                part_tets.push_back(tet);
        }
    }

    //-----------------------------------------
    // If no violations, move to next vertex
    //-----------------------------------------
    if(viol_edges.empty() && viol_faces.empty() && viol_tets.empty())
    {
        return;
    }


    //-----------------------------------------
    // Compute Warp Point
    //-----------------------------------------
    vec3 warp_point = vec3::zero;

    // If 1 Quadpoint is Violating, take quad position
    if(viol_tets.size() == 1)
    {
        warp_point = viol_tets[0]->quadruple->pos();
    }
    // Else If 1 Triplepoint is Violating
    else if(viol_faces.size() == 1)
    {
        warp_point = viol_faces[0]->triple->pos();
    }
    // Otherwise Take Center of Mass
    else
    {
        for(unsigned int i=0; i < viol_edges.size(); i++)
            warp_point += viol_edges[i]->cut->pos();

        for(unsigned int i=0; i < viol_faces.size(); i++)
            warp_point += viol_faces[i]->triple->pos();

        for(unsigned int i=0; i < viol_tets.size(); i++)
            warp_point += viol_tets[i]->quadruple->pos();

        warp_point /= viol_edges.size() + viol_faces.size() + viol_tets.size();
    }

    //---------------------------------------
    //  Conform Quadruple Pt (if it exists)
    //---------------------------------------
    for(unsigned int t=0; t < part_tets.size(); t++)
    {
        conformQuadruple(part_tets[t], vertex, warp_point);
    }


    //---------------------------------------------------------
    // Project Any TriplePoints That Survive On A Warped Face
    //---------------------------------------------------------

    // WARNING: Comparing edges must also compare against MATE edges.
    //          Added checks here for safety, but issue might exist
    //          elsewhere. Keep an eye out.
    for(unsigned int f=0; f < part_faces.size(); f++)
    {
        HalfFace *face = part_faces[f];
        Tet  *innerTet = getInnerTet(face, vertex, warp_point);
        Vertex     *q = innerTet->quadruple;

        // conform triple if it's also a quad (would not end up in part_quads list)
        if(q->isEqualTo(face->triple)){
            conformQuadruple(innerTet, vertex, warp_point);
        }
        // coincide with Quad if it conformed to the face
        else if(q->order() == QUAD && q->conformedFace->sameAs(face))
        {
            part_faces[f]->triple->pos_next() = q->pos_next();
            part_faces[f]->triple->conformedEdge = NULL;
        }
        // coincide with Quad if it conformed to one of faces edges
        else if(q->order() == QUAD && (q->conformedEdge->sameAs(face->halfEdges[0]) ||
                                       q->conformedEdge->sameAs(face->halfEdges[1]) ||
                                       q->conformedEdge->sameAs(face->halfEdges[2])))
        {
          part_faces[f]->triple->pos_next() = q->pos_next();
          part_faces[f]->triple->conformedEdge = q->conformedEdge;
        }
        // otherwise intersect lattice face with Q-T interface
        else{
            part_faces[f]->triple->pos_next() = projectTriple(part_faces[f], q, vertex, warp_point);
            conformTriple(part_faces[f], vertex, warp_point);
        }

        // Sanity Check for NaN
        /*
        if(part_faces[f]->triple->pos_next() != part_faces[f]->triple->pos_next())
        {
            std::cerr << "Fatal Error:  Triplepoint set to NaN: Failed to project triple using InnerTet." << std::endl;
            exit(1445);
        }

        // Sanity Check for Zero
        if(part_faces[f]->triple->pos_next() == vec3::zero)
        {
            std::cerr << "Fatal Error:  Triplepoint set to vec3::zero == (0,0,0)" << std::endl;
            exit(1452);
        }
        */
    }


    //------------------------------------------------------
    // Project Any Cutpoints That Survived On A Warped Edge
    //------------------------------------------------------
    for(unsigned int e=0; e < part_edges.size(); e++)
    {
        HalfEdge *edge = part_edges[e];
        Tet *innertet = getInnerTet(edge, vertex, warp_point);

        std::vector<HalfFace*> faces = m_bgMesh->facesAroundEdge(edge);

        bool handled = false;
        for(unsigned int f=0; f < faces.size(); f++)
        {
            // if triple conformed to this edge, use it's position
            if(faces[f]->triple->order() == TRIP && faces[f]->triple->conformedEdge->sameAs(part_edges[e])){
                part_edges[e]->cut->pos_next() = faces[f]->triple->pos_next();
                handled = true;
                break;
            }
        }

        if(handled && edge->cut->pos_next() == vec3::zero)
            std::cerr << "Conformed Cut Problem!" << std::endl;

        // TODO: What about conformedVertex like quadpoint?
        // otherwise compute projection with innerTet
        if(!handled)
            edge->cut->pos_next() = projectCut(edge, innertet, vertex, warp_point);


        if(edge->cut->pos_next() == vec3::zero)
            std::cerr << "Cut Projection Problem!" << std::endl;

    }


    //------------------------------------
    //   Update Vertices
    //------------------------------------
    vertex->pos() = warp_point;
    vertex->warped = true;

    // move remaining cuts and check for violation
    for (unsigned int e=0; e < part_edges.size(); e++)
    {
        HalfEdge *edge = part_edges[e];
        edge->cut->pos() = edge->cut->pos_next();
        checkIfCutViolatesVertices(edge);
    }
    // move remaining triples and check for violation
    for (unsigned int f=0; f < part_faces.size(); f++)
    {
        HalfFace *face = part_faces[f];
        face->triple->pos() = face->triple->pos_next();
        checkIfTripleViolatesVertices(face);
    }
    // move remaining quadruples and check for violation
    for (unsigned int t=0; t < part_tets.size(); t++){
        Tet *tet = part_tets[t];
        tet->quadruple->pos() = tet->quadruple->pos_next();
        checkIfQuadrupleViolatesVertices(tet);
    }

    //------------------------------------------------------
    // Delete cuts of the same interface type
    //------------------------------------------------------
    // TODO:  Incorporate Triple and Quadpoint Material Values Checks
    for(unsigned int e=0; e < part_edges.size(); e++){

        // check if same as one of the violating interface types
        bool affected = false;

        for(unsigned int c=0; c < viol_edges.size() && !affected; c++)
        {
            bool same = true;
            for(int m=0; m < m_volume->numberOfMaterials(); m++){
                if(part_edges[e]->cut->lbls[m] != viol_edges[c]->cut->lbls[m]){
                    same = false;
                    break;
                }
            }
            affected = same;
        }

        //-----------------------
        // If Affected, Snap it
        //-----------------------
        if(affected)
        {
            snapCutForEdgeToVertex(part_edges[e], vertex);
        }
    }


    //------------------------------------------------------------------
    //  Delete Cut if Projection Makes it Violate New Vertex Location
    //------------------------------------------------------------------
    for (unsigned int e=0; e < part_edges.size(); e++)
    {
        Vertex *cut = part_edges[e]->cut;

        if(cut->order() == CUT && cut->violating)
        {
            // if now violating this vertex, snap to it
            if(cut->closestGeometry == vertex)
                snapCutForEdgeToVertex(part_edges[e], vertex);

            // else if violating an already warped vertex, snap to it
            else if(((Vertex*)cut->closestGeometry)->warped)
            {
                snapCutForEdgeToVertex(part_edges[e], (Vertex*)cut->closestGeometry);

                // Probably should call resolve_degeneracies around vertex(closestGeometry); to be safe
                resolveDegeneraciesAroundVertex((Vertex*)cut->closestGeometry);
            }           
        }
    }



    //---------------------------------------------------------------------
    //  Delete Triple if Projection Makies it Violate New Vertex Location
    //---------------------------------------------------------------------
    for (unsigned int f=0; f < part_faces.size(); f++)
    {
        Vertex *triple = part_faces[f]->triple;

        if(triple->order() == TRIP && triple->violating)
        {
            // if now violating this vertex, snap to it
            if(triple->closestGeometry == vertex)
            {
                snapTripleForFaceToVertex(part_faces[f], vertex);
            }
            // else if violating an already warped vertex, snap to it
            else if(((Vertex*)triple->closestGeometry)->warped)
            {
                snapTripleForFaceToVertex(part_faces[f], (Vertex*)triple->closestGeometry);

                // Probably should call resolve_degeneracies around vertex(closestGeometry); to be safe
                resolveDegeneraciesAroundVertex((Vertex*)triple->closestGeometry);
            }
        }
    }

    //------------------------------------------------------------------------
    //  Delete Quadruple If Projection Makies it Violate New Vertex Location
    //------------------------------------------------------------------------
    for (unsigned int t=0; t < part_tets.size(); t++)
    {
        Vertex *quadruple = part_tets[t]->quadruple;

        // TODO: This should follow same logic as Triple Deletion Above.
        if(quadruple->order() == QUAD && quadruple->violating && quadruple->closestGeometry == vertex)
        {
            snapQuadrupleForTetToVertex(part_tets[t], vertex);
        }
    }

    //------------------------
    // Delete All Violations
    //------------------------
    // 1) cuts
    for(unsigned int e=0; e < viol_edges.size(); e++)
        snapCutForEdgeToVertex(viol_edges[e], vertex);


    // 2) triples
    for(unsigned int f=0; f < viol_faces.size(); f++)
        snapTripleForFaceToVertex(viol_faces[f], vertex);


    // 3) quadruples
    for(unsigned int t=0; t < viol_tets.size(); t++)
       snapQuadrupleForTetToVertex(viol_tets[t], vertex);


    //--------------------------------------
    //  Resolve Degeneracies Around Vertex
    //--------------------------------------
    resolveDegeneraciesAroundVertex(vertex);

    //---------------------------------------------------------------------
    // end. CleaverMesherImp::snapAndWarpForViolatedVertex(Vertex *vertex)
    //---------------------------------------------------------------------
}



//=================================
// Helper function for getInnerTet
//
// Switches the addresses of two
// vertex pointers.
//=================================
void swap(Vertex* &v1, Vertex* &v2)
{
    Vertex *temp = v1;
    v1 = v2;
    v2 = temp;
}


//--------------------------------------------------------------------------------------------
//  triangle_intersect()
//
//  This method computes the intersection of a ray and triangle. The intersection point
//  is stored in 'pt', while a boolean returned indicates whether or not the intersection
//  occurred in the triangle. Epsilon tolerance is given to boundary case.
//--------------------------------------------------------------------------------------------
bool triangle_intersection(Vertex *v1, Vertex *v2, Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, float epsilon = 1E-8)
{
    float epsilon2 = (float)1E-3;

    //-------------------------------------------------
    // if v1, v2, and v3 are not unique, return FALSE
    //-------------------------------------------------
    if(v1 == v2 || v2 == v3 || v1 == v3)
        return false;
    else if(L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v1->pos() - v3->pos()) < epsilon)
        return false;

    //----------------------------------------------
    // compute intersection with plane, store in pt
    //----------------------------------------------
    vec3 e1 = v1->pos() - v3->pos();
    vec3 e2 = v2->pos() - v3->pos();

    ray = normalize(ray);
    vec3 r1 = ray.cross(e2);
    double denom = e1.dot(r1);

    if( fabs(denom) < epsilon)
        return false;

    double inv_denom = 1.0 / denom;
    vec3 s = origin - v3->pos();
    double b1 = s.dot(r1) * inv_denom;

    if(b1 < (0.0 - epsilon2) || b1 > (1.0 + epsilon2))
        return false;

    vec3 r2 = s.cross(e1);
    double b2 = ray.dot(r2) * inv_denom;

    if(b2 < (0.0 - epsilon2) || (b1 + b2) > (1.0 + 2*epsilon2))
        return false;

    double t = e2.dot(r2) * inv_denom;
    pt = origin + t*ray;


    if(t < 0.01)
        return false;
    else
        return true;
}

//=======================================================================
// - getInnerTet(HalfEdge *edge)
//
//  This method determines which lattice Tet should take care
//  of projection the cut on the participating edge tied
//  to the current mesh warp.
//=======================================================================
Tet* CleaverMesherImp::getInnerTet(HalfEdge *edge, Vertex *warpVertex, const vec3 &warpPt)
{
    std::vector<Tet*> tets = m_bgMesh->tetsAroundEdge(edge);
    vec3 hit_pt = vec3::zero;

    Vertex *static_vertex;

    if(edge->vertex == warpVertex)
        static_vertex = edge->vertex;
    else
        static_vertex = edge->mate->vertex;

    vec3 origin = 0.5*(edge->vertex->pos() + edge->mate->vertex->pos());  //edge->cut->pos(); //static_vertex->pos(); //
    vec3 ray = warpPt - origin;

    for(unsigned int t=0; t < tets.size(); t++)
    {
        std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

        for(int f=0; f < FACES_PER_TET; f++)
        {
            std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);

            if(triangle_intersection(verts[0], verts[1], verts[2], origin, ray, hit_pt))
            {
                if(L2(edge->cut->pos() - hit_pt) > 1E-3)
                    return tets[t];
            }
        }
    }


    // if none hit, make a less picky choice
    for(unsigned int t=0; t < tets.size(); t++)
    {
        std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

        for(int f=0; f < 4; f++)
        {
            std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);

            if(triangle_intersection(verts[0], verts[1], verts[2], origin, ray, hit_pt))
            {
                return tets[t];
            }
        }
    }

    // if STILL none hit, we have a problem
    std::cerr << "WARNING: Failed to find Inner Tet for Edge" << std::endl;
    //exit(-1);

    return NULL;
}

//====================================================================================
// - getInnerTet(HalfFace *face)
//
//  This method determines which lattice Tet should take care
//  of projection the triple on the participating face tied
//  to the current lattice warp.
//====================================================================================
Tet* CleaverMesherImp::getInnerTet(HalfFace *face, Vertex *vertex, const vec3 &warpPt)
{
    vec3 dmy_pt;
    vec3 ray = normalize(warpPt - face->triple->pos());

    std::vector<Tet*> tets = m_bgMesh->tetsAroundFace(face);

    // if on boundary, return only neighbor tet  (added Mar 14/2003)
    if(tets.size() == 1)
        return tets[0];

    std::vector<Vertex*> verts_a = m_bgMesh->vertsAroundTet(tets[0]);
    std::vector<Vertex*> verts_b = m_bgMesh->vertsAroundTet(tets[1]);

    // sort them so exterior vertex is first
    for(int v=0; v < 4; v++){
        if(!face->incidentToVertex(verts_a[v]))
            swap(verts_a[0], verts_a[v]);

        if(!face->incidentToVertex(verts_b[v]))
            swap(verts_b[0], verts_b[v]);
    }

    vec3 vec_a = normalize(verts_a[0]->pos() - face->triple->pos());
    vec3 vec_b = normalize(verts_b[0]->pos() - face->triple->pos());
    vec3 n = normalize(cross(verts_a[3]->pos() - verts_a[1]->pos(), verts_a[2]->pos() - verts_a[1]->pos()));

    float dot1 = (float)dot(vec_a, ray);
    float dot2 = (float)dot(vec_b, ray);

    if(dot1 > dot2)
        return tets[0];
    else
        return tets[1];


    // if neither hit, we have a problem
    std::cerr << "Fatal Error:  Failed to find Inner Tet for Face" << std::endl;
    exit(-1);
    return NULL;
}

//========================================================================================================
// - projectTriple()
//
// Triplepoints are the endpoints of 1-d interfaces that divide 3 materials. The other endpoint
// must either be another triplepoint or a quadruplepoint. (or some other vertex to which
// one of these has snapped. When a lattice vertex is moved, the triple point must be projected
// so that it remains with the plane of a lattice face. This projection works by intersecting
// this interface edge with the new lattice face plane, generated by warping a lattice vertex.
//========================================================================================================
vec3 CleaverMesherImp::projectTriple(HalfFace *face, Vertex *quad, Vertex *warpVertex, const vec3 &warpPt)
{
    Vertex *trip = face->triple;
    std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(face);

    for(int i=0; i < 3; i++)
    {
        if(verts[i] == warpVertex)
        {
            // swap so moving vertex is first in list
            verts[i] = verts[0];
            verts[0] = warpVertex;
            break;
        }
    }

    vec3 p_0 = warpPt;
    vec3 p_1 = verts[1]->pos();
    vec3 p_2 = verts[2]->pos();
    vec3 n = normalize(cross(p_1 - p_0, p_2 - p_0));
    vec3 I_a = trip->pos();
    vec3 I_b = quad->pos();
    vec3 l = I_b - I_a;

    // Check if I_b/I_a (Q/T) interface as collapsed)
    if (length(l) < 1E-5 || dot(l,n) == 0)
        return trip->pos();

    double d = dot(p_0 - I_a, n) / dot(l,n);


    vec3 intersection = I_a + d*l;

    return intersection;
}



//===========================================================================================
//  triangle_intersect()
//
//  This method computes the intersection of a ray and triangle, in a slower way,
//  but in a way that allows an epsilon error around each triangle.
//===========================================================================================
bool triangle_intersect(Vertex *v1, Vertex *v2, Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, double &error, double EPS)
{
    double epsilon = 1E-7;

    //-------------------------------------------------
    // if v1, v2, and v3 are not unique, return FALSE
    //-------------------------------------------------
    if(v1->isEqualTo(v2) || v2->isEqualTo(v3) || v3->isEqualTo(v1))
    {
        pt = vec3(-2, -2, -2); // Debug J.R.B. 11/22/11
        return false;
    }
    else if(L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v3->pos() - v1->pos()) < epsilon)
    {
        pt = vec3(-3, -3, -3); // Debug J.R.B. 11/22/11
        return false;
    }

    //----------------------------------------------
    // compute intersection with plane, store in pt
    //----------------------------------------------
    plane_intersect(v1, v2, v3, origin, ray, pt);
    vec3 tri_pt = vec3::zero;

    //----------------------------------------------
    //      Compute Barycentric Coordinates
    //----------------------------------------------
    // Create Matrix A and solve for Inverse
    double A[3][3];
    vec3 inv1,inv2,inv3;
    vec3 p1 = v1->pos();
    vec3 p2 = v2->pos();
    vec3 p3 = v3->pos();
    vec3 p4 = p1 + normalize(cross(normalize(p3 - p1), normalize(p2 - p1)));

    // Fill Coordinate Matrix
    A[0][0] = p1.x - p4.x; A[0][1] = p2.x - p4.x; A[0][2] = p3.x - p4.x;
    A[1][0] = p1.y - p4.y; A[1][1] = p2.y - p4.y; A[1][2] = p3.y - p4.y;
    A[2][0] = p1.z - p4.z; A[2][1] = p2.z - p4.z; A[2][2] = p3.z - p4.z;

    // Solve Inverse
    double det = +A[0][0]*(A[1][1]*A[2][2]-A[2][1]*A[1][2])
                 -A[0][1]*(A[1][0]*A[2][2]-A[1][2]*A[2][0])
                 +A[0][2]*(A[1][0]*A[2][1]-A[1][1]*A[2][0]);
    double invdet = 1/det;
    inv1.x =  (A[1][1]*A[2][2]-A[2][1]*A[1][2])*invdet;
    inv2.x = -(A[1][0]*A[2][2]-A[1][2]*A[2][0])*invdet;
    inv3.x =  (A[1][0]*A[2][1]-A[2][0]*A[1][1])*invdet;

    inv1.y = -(A[0][1]*A[2][2]-A[0][2]*A[2][1])*invdet;
    inv2.y =  (A[0][0]*A[2][2]-A[0][2]*A[2][0])*invdet;
    inv3.y = -(A[0][0]*A[2][1]-A[2][0]*A[0][1])*invdet;

    inv1.z =  (A[0][1]*A[1][2]-A[0][2]*A[1][1])*invdet;
    inv2.z = -(A[0][0]*A[1][2]-A[1][0]*A[0][2])*invdet;
    inv3.z =  (A[0][0]*A[1][1]-A[1][0]*A[0][1])*invdet;

    // Multiply Inverse*Coordinate to get Lambda (Barycentric)
    vec3 lambda;
    lambda.x = inv1.x*(pt.x - p4.x) + inv1.y*(pt.y - p4.y) + inv1.z*(pt.z - p4.z);
    lambda.y = inv2.x*(pt.x - p4.x) + inv2.y*(pt.y - p4.y) + inv2.z*(pt.z - p4.z);
    lambda.z = inv3.x*(pt.x - p4.x) + inv3.y*(pt.y - p4.y) + inv3.z*(pt.z - p4.z);

    //----------------------------------------------
    //   Project to Valid Coordinate
    //----------------------------------------------
    // clamp to borders
    lambda.x = std::max(0.0, lambda.x);
    lambda.y = std::max(0.0, lambda.y);
    lambda.z = std::max(0.0, lambda.z);

    // renormalize
    lambda /= L1(lambda);

    // compute new coordinate in triangle
    tri_pt.x = lambda.x*p1.x + lambda.y*p2.x + lambda.z*p3.x;
    tri_pt.y = lambda.x*p1.y + lambda.y*p2.y + lambda.z*p3.y;
    tri_pt.z = lambda.x*p1.z + lambda.y*p2.z + lambda.z*p3.z;

    // project pt onto ray
    vec3 a = tri_pt - origin;
    vec3 b = ray;
    vec3 c = (a.dot(b) / b.dot(b)) * b;

    // find final position;
    double t = length(c);
    if(c.dot(b) < 0)
        t *= -1;

    pt = origin + t*ray;

    // how far are we from the actual triangle pt?
    error = L2(tri_pt - pt);


    //if(error != error)
    //    cerr << "TriangleIntersect2 error = NaN" << endl;

    //----------------------------------------------
    //  If Made It This Far,  Return Success
    //----------------------------------------------
    return true;
}


//================================================================================================
// - projectCut()
//
//
//================================================================================================
vec3 CleaverMesherImp::projectCut(HalfEdge *edge, Tet *tet, Vertex *warpVertex, const vec3 &warpPt)
{  
    // added feb21 because no inner tet case below seems to fail on exterior ("sometimes?")
    // WHY is it needed?
    if(edge->vertex->isExterior || edge->mate->vertex->isExterior)
        return edge->cut->pos();

    // Handle Case if No Inner Tet was FounD (like exterior of volume) TODO: Evaluate this case carefully
    if(tet == NULL){
        vec3 pt = vec3::zero;
        Vertex *static_vertex = NULL;

        if(edge->vertex == warpVertex)
            static_vertex = edge->mate->vertex;
        else
            static_vertex = edge->vertex;

        double t = length(edge->cut->pos() - static_vertex->pos()) / length(warpVertex->pos() - static_vertex->pos());
        pt = static_vertex->pos() + t*(warpPt - static_vertex->pos());
        return pt;
    }
    //----------------

    Vertex *static_vertex = NULL;
    Vertex *quad = tet->quadruple;
    Vertex *verts[15] = {0};
    m_bgMesh->getRightHandedVertexList(tet, verts);

    if(edge->vertex == warpVertex)
        static_vertex = edge->mate->vertex;
    else
        static_vertex = edge->vertex;

    vec3 static_pt = static_vertex->pos();
    vec3 pt = vec3::zero;

    //------------------------
    // Form Intersecting Ray
    //------------------------
    vec3 ray = normalize(warpPt - static_pt);
    double min_error = 10000;

    vec3   point[12];
    double error[12];
    bool   valid[12] = {0};

    std::vector<vec3> interface_verts;

    // check intersection with each triangle face
    for(int i=0; i < 12; i++)
    {
        if(completeInterfaceTable[i][0] < 0)
            break;

        Vertex *v1 = verts[completeInterfaceTable[i][0]];
        Vertex *v2 = verts[completeInterfaceTable[i][1]];
        Vertex *v3 = quad;

        if(v1->isEqualTo(edge->cut) || v2->isEqualTo(edge->cut) || v3->isEqualTo(edge->cut) ||
                L2(v1->pos() - edge->cut->pos()) < 1E-7 || L2(v2->pos() - edge->cut->pos()) < 1E-7 || L2(v3->pos() - edge->cut->pos()) < 1E-7)
        {
            valid[i] = triangle_intersect(v1,v2,v3, static_pt, ray, point[i], error[i], 1E-2);
        }
        else{
            // skip it
        }
    }


    // pick the one with smallest error
    for(int i=0; i < 12; i++)
    {
        if(valid[i] && error[i] < min_error)
        {
            pt = point[i];
            min_error = error[i];
        }
    }

    // if no intersections, don't move it at all
    if(pt == vec3::zero){
        pt = edge->cut->pos();
    }

    // Conform Point!!!
    vec3 newray = pt - static_vertex->pos();
    double t1 = newray.x / ray.x;

    if(t1 < 0 || t1 > 1)
    {
        // clamp it
        t1 = std::max(t1, 0.0);
        t1 = std::min(t1, 1.0);
        pt = static_pt + t1*ray;
    }

    return pt;
}

//============================================================
//  Snap and Warp All Edge Violations
//============================================================
void CleaverMesherImp::snapAndWarpEdgeViolations(bool verbose)
{
    //---------------------------------------------------
    //  Check for edge violations
    //---------------------------------------------------
    // first check triples violating edges
    for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
    {
        cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

        if(face->triple && face->triple->order() == TRIP)
            checkIfTripleViolatesEdges(face);
    }
    // then check quadruples violating edges
    for(unsigned int t=0; t < m_bgMesh->tets.size(); t++)
    {
        cleaver::Tet *tet = m_bgMesh->tets[t];
        if(tet->quadruple && tet->quadruple->order() == QUAD)
            checkIfQuadrupleViolatesEdges(tet);
    }

    //---------------------------------------------------
    //  Apply snapping to all remaining edge-cuts
    //---------------------------------------------------
    std::map<std::pair<int, int>, HalfEdge*>::iterator edgesIter = m_bgMesh->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while(edgesIter != m_bgMesh->halfEdges.end())
    {
        HalfEdge *edge = (*edgesIter).second;    // TODO: add  redundancy checks
        snapAndWarpForViolatedEdge(edge);        //           to reduce workload.
        edgesIter++;
    }

    std::cout << "Phase 2 Complete" << std::endl;
}

//===============================================================
//  Snap and Warp Violations Surrounding an Edge
//===============================================================
void CleaverMesherImp::snapAndWarpForViolatedEdge(HalfEdge *edge)
{
    // look at each adjacent face
    std::vector<HalfFace*> faces = m_bgMesh->facesAroundEdge(edge);

    for(unsigned int f=0; f < faces.size(); f++)
    {
        Vertex *triple = faces[f]->triple;

        if(triple->order() == TRIP &&
           triple->violating &&
           (triple->closestGeometry == edge || triple->closestGeometry == edge->mate))
        {
            snapTripleForFaceToCut(faces[f], edge->cut);
        }
    }

    // If triples went to a vertex, resolve degeneracies on that vertex
    if(edge->cut->order() == VERT)
    {
        resolveDegeneraciesAroundVertex(edge->cut->root());
    }
    // Else Triple went to the Edge-Cut
    else
    {
        resolveDegeneraciesAroundEdge(edge);
    }
}


//============================================================
//  Snap and Warp All Face Violations
//============================================================
void CleaverMesherImp::snapAndWarpFaceViolations(bool verbose)
{
    //---------------------------------------------------
    //  Apply snapping to all remaining face-triples
    //---------------------------------------------------
    for(unsigned int f=0; f < 4*m_bgMesh->tets.size(); f++)
    {
        HalfFace *face = &m_bgMesh->halfFaces[f];  // TODO: add  redundancy checks
        snapAndWarpForViolatedFace(face);         //           to reduce workload.
    }

    std::cout << "Phase 3 Complete" << std::endl;
}


//===============================================================
//  Snap and Warp Violations Surrounding a Face
//===============================================================
void CleaverMesherImp::snapAndWarpForViolatedFace(HalfFace *face)
{    

    std::vector<Tet*> tets = m_bgMesh->tetsAroundFace(face);

    for(unsigned int t=0; t < tets.size(); t++)
    {
        Vertex *quadruple = tets[t]->quadruple;

        if(quadruple->order() == QUAD && quadruple->violating && (quadruple->closestGeometry == face || quadruple->closestGeometry == face->mate))
        {
            // Snap to triple point, wherever it happens to be
            snapQuadrupleForTetToTriple(tets[t], face->triple);


            // check order of vertex now pointed to
            switch(tets[t]->quadruple->order())
            {
                case VERT:
                {
                    // If Triple_point is on a Vertex
                    resolveDegeneraciesAroundVertex(tets[t]->quadruple->root());
                    break;
                }
                case CUT:
                {
                    for(unsigned int e=0; e < EDGES_PER_FACE; e++)
                    {
                        HalfEdge *edge = face->halfEdges[e];

                        if(edge->cut->isEqualTo(tets[t]->quadruple))
                        {
                            snapQuadrupleForTetToEdge(tets[t], edge);
                            resolveDegeneraciesAroundEdge(edge);
                        }
                    }
                    break;
                }
                case TRIP:
                {
                    // If Triple-Point is on a Face, do nothing
                    break;
                }
                default:
                {
                    std::cerr << "Fatal Error - Quad order == " << tets[t]->quadruple->order() << std::endl;
                    exit(-1);
                }
            }
        }
    }
}

//================================================
// - snapCutToVertex()
//================================================
void CleaverMesherImp::snapCutForEdgeToVertex(HalfEdge *edge, Vertex* vertex)
{
    if(edge->cut->original_order() == CUT)
        edge->cut->parent = vertex;
    else{
        edge->cut = vertex;
        edge->mate->cut = vertex;
    }
}

//======================================================
// - snapTripleToVertex()
//======================================================
void CleaverMesherImp::snapTripleForFaceToVertex(HalfFace *face, Vertex* vertex)
{
    if(face->triple->original_order() == TRIP)
        face->triple->parent = vertex;
    else{
        face->triple = vertex;
        if(face->mate)
            face->mate->triple = vertex;
    }

}

//=====================================================================
// - snapTripleToCut()
//=====================================================================
void CleaverMesherImp::snapTripleForFaceToCut(HalfFace *face, Vertex *cut)
{
    if(face->triple->original_order() == TRIP)
        face->triple->parent = cut;
    else{
        face->triple = cut;
        if(face->mate)
            face->mate->triple = cut;
    }
}

//============================================================
// - snapQuadrupleToVertex()
//============================================================
void CleaverMesherImp::snapQuadrupleForTetToVertex(Tet *tet, Vertex* vertex)
{
    if(tet->quadruple->original_order() == QUAD)
        tet->quadruple->parent = vertex;
    else
        tet->quadruple = vertex;
}

//=====================================================================
// - snapQuadrupleToCut()
//
//=====================================================================
void CleaverMesherImp::snapQuadrupleForTetToCut(Tet *tet, Vertex *cut)
{
    if(tet->quadruple->original_order() == QUAD)
        tet->quadruple->parent = cut;
    else
        tet->quadruple = cut;
}

//=====================================================================
// - snapQuadrupleForTetToTriple
//=====================================================================
void CleaverMesherImp::snapQuadrupleForTetToTriple(Tet *tet, Vertex *triple)
{
    if(tet->quadruple->original_order() == QUAD)
        tet->quadruple->parent = triple;
    else
        tet->quadruple = triple;
}



//=====================================================================
// - snapQuadrupleToEdge()
//
//  This method handles the complex task of snapping a quadpoint to
// an edge. Task is complicated because it causes a degeneration of
// adjacent triple points. If these triplepoints have already snapped,
// the degeneracy propagates to the next adjacent LatticeTet around
// the edge being snapped to. Hence, this is a recursive function.
//=====================================================================
void CleaverMesherImp::snapQuadrupleForTetToEdge(Tet *tet, HalfEdge *edge)
{
    // Snap the Quad if it's not already there
    if(!tet->quadruple->isEqualTo(edge->cut)){
        snapQuadrupleForTetToCut(tet, edge->cut);
    }

    // Get Adjacent TripleFaces
    std::vector<HalfFace*> adjFaces = m_bgMesh->facesIncidentToBothTetAndEdge(tet, edge);

    for(unsigned int f=0; f < 2; f++)
    {
        // if still a triple, snap it
        if(adjFaces[f]->triple->order() == TRIP)
        {
            snapTripleForFaceToCut(adjFaces[f], edge->cut);

            Tet *opTet = m_bgMesh->oppositeTetAcrossFace(tet, adjFaces[f]);

            // if adjacent Tet quadpoint is snapped to this triple, snap it next
            if(opTet && opTet->quadruple->isEqualTo(adjFaces[f]->triple))
                snapQuadrupleForTetToEdge(opTet, edge);

        }

        // if snapped to a different edge
        else if(adjFaces[f]->triple->order() == CUT && !(adjFaces[f]->triple->isEqualTo(edge->cut)))
        {
            Tet *opTet = m_bgMesh->oppositeTetAcrossFace(tet, adjFaces[f]);

            // if adjacent Tet quadpoint is snapped to this triple, snap it next
            if(opTet && opTet->quadruple->isEqualTo(adjFaces[f]->triple))
                snapQuadrupleForTetToEdge(opTet, edge);

            snapTripleForFaceToCut(adjFaces[f], edge->cut);
        }
        // otherwise done
        else
        {
            // do nothing
        }
    }
}



//===================================================
// - resolveDegeneraciesAroundVertex()
//   TODO: This currently checks too much, even vertices
//         that were snapped a long time ago. Optimize.
//===================================================
void CleaverMesherImp::resolveDegeneraciesAroundVertex(Vertex *vertex)
{
    std::vector<HalfFace*> faces = m_bgMesh->facesAroundVertex(vertex);
    std::vector<Tet*>       tets = m_bgMesh->tetsAroundVertex(vertex);

    bool changed = true;
    while(changed)
    {
        changed = false;

        //--------------------------------------------------------------------------
        // Snap Any Triples or Cuts that MUST follow a Quadpoint
        //--------------------------------------------------------------------------
        for(unsigned int t=0; t < tets.size(); t++)
        {
            Tet *tet = tets[t];

            // If Quadpoint is snapped to Vertex
            if(tet->quadruple->isEqualTo(vertex))
            {
                // Check if any cuts exist to snap
                std::vector<HalfEdge*> edges = m_bgMesh->edgesAroundTet(tet);
                for(int e=0; e < EDGES_PER_TET; e++)
                {
                    // cut exists & spans the vertex in question
                    if(edges[e]->cut->order() == CUT && (edges[e]->vertex == vertex || edges[e]->mate->vertex == vertex))
                    {
                        snapCutForEdgeToVertex(edges[e], vertex);
                        changed = true;
                    }
                }

                // Check if any triples exist to snap
                std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tet);
                for(int f=0; f < FACES_PER_TET; f++)
                {
                    // triple exists & spans the vertex in question
                    if(faces[f]->triple->order() == TRIP)
                    {
                        std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);
                        if(verts[0] == vertex || verts[1] == vertex || verts[2] == vertex)
                        {
                            snapTripleForFaceToVertex(faces[f], vertex);
                            changed = true;
                        }
                    }
                }
            }
        }

        //------------------------------------------------------------------------------------
        // Snap Any Cuts that MUST follow a Triplepoint
        //------------------------------------------------------------------------------------
        for(unsigned int f=0; f < faces.size(); f++)
        {
            // If Triplepoint is snapped to Vertex
            if(faces[f] && faces[f]->triple->isEqualTo(vertex))
            {
                // Check if any cuts exist to snap
                for(int e=0; e < EDGES_PER_FACE; e++)
                {
                    HalfEdge *edge = faces[f]->halfEdges[e];
                    // cut exists & spans the vertex in question
                    if(edge->cut->order() == CUT && (edge->vertex == vertex || edge->mate->vertex == vertex))
                    {
                        snapCutForEdgeToVertex(edge, vertex);
                        changed = true;
                    }
                }
            }
        }


        //------------------------------------------------------------------------------------
        // Snap Any Triples that have now degenerated
        //------------------------------------------------------------------------------------
        for(unsigned int f=0; f < faces.size(); f++)
        {
            if(faces[f] && faces[f]->triple->order() == TRIP)
            {
                // count # cuts snapped to vertex
                int count = 0;
                for(int e=0; e < EDGES_PER_FACE; e++){
                    HalfEdge *edge = faces[f]->halfEdges[e];
                    count += (int) edge->cut->isEqualTo(vertex);
                }

                // if two cuts have snapped to vertex, triple degenerates
                if(count == 2)
                {
                    snapTripleForFaceToVertex(faces[f], vertex);
                    changed = true;
                }
            }
        }

        //------------------------------------------------------------------------------------
        // Snap Any Quads that have now degenerated
        //------------------------------------------------------------------------------------
        for(unsigned int t=0; t < tets.size(); t++)
        {
            if(tets[t] && tets[t]->quadruple->order() == QUAD)
            {
                std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

                // count # trips snapped to vertex
                int count = 0;
                for(int f=0; f < FACES_PER_TET; f++)
                    count += (int) faces[f]->triple->isEqualTo(vertex);

                // if 3 triples have snapped to vertex, quad degenerates
                if(count == 3)
                {
                    snapQuadrupleForTetToVertex(tets[t], vertex);
                    changed = true;
                }
            }
        }

    }
}

//=================================================
// - resolveDegeneraciesAroundEdge()
//=================================================
void CleaverMesherImp::resolveDegeneraciesAroundEdge(HalfEdge *edge)
{
    std::vector<Tet*> tets = m_bgMesh->tetsAroundEdge(edge);

    //--------------------------------------------------------------------------
    //  Pull Adjacent Triples To Quadpoint  (revise: 11/16/11 J.R.B.)
    //--------------------------------------------------------------------------
    for(unsigned int t=0; t < tets.size(); t++)
    {
        if(tets[t]->quadruple->isEqualTo(edge->cut))
        {
            snapQuadrupleForTetToEdge(tets[t], edge);
        }
    }

    //--------------------------------------------------------------------------
    // Snap Any Quads that have now degenerated onto the Edge
    //--------------------------------------------------------------------------
    for(unsigned int t=0; t < tets.size(); t++)
    {
        if(tets[t]->quadruple->order() == QUAD)
        {
            std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

            // count # triples snaped to edge
            int count = 0;
            for(int f=0; f < FACES_PER_TET; f++)
                count += (int) faces[f]->triple->isEqualTo(edge->cut);

            // if two triples have snapped to edgecut, quad degenerates
            if(count == 2)
            {
                snapQuadrupleForTetToEdge(tets[t], edge);
            }
        }
    }
}


//===============================================================

Vertex* vertexCopy(Vertex *vertex)
{
    Vertex *copy = new Vertex();
    copy->pos() = vertex->pos();
    copy->label = vertex->label;
    copy->order() = vertex->order();

    return copy;
}

Vertex** cloneVerts(Vertex *verts[15])
{
    Vertex **verts_copy = new Vertex*[15];

    for(int i=0; i < 15; i++)
    {
        if(verts[i] == NULL)
            verts_copy[i] = NULL;
        else{
            verts_copy[i] = vertexCopy(verts[i]);
        }
    }

    return verts_copy;
}



//==============================================
//  stencilTets()
//==============================================
void CleaverMesherImp::stencilBackgroundTets(bool verbose)
{
    if(verbose)
        std::cout << "Filling in Stencils..." << std::endl;

    // be safe, and remove ALL old adjacency info on tets
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        Vertex *vertex = m_bgMesh->verts[v];
        vertex->tets.clear();
        vertex->tm_v_index = -1;
    }
    m_bgMesh->verts.clear();

    int total_output = 0;
    int total_changed = 0;

    TetMesh *debugMesh;
    bool debugging = false;
    if(debugging)
        debugMesh = new TetMesh();


    //-------------------------------
    // Naively Examine All Tets
    //-------------------------------
    for(size_t t=0; t < m_bgMesh->tets.size(); t++)
    {
        // ----------------------------------------
        // Grab Handle to Current Background Tet
        // ----------------------------------------
        Tet *tet = m_bgMesh->tets[t];

        //----------------------------------------------------------
        // Ensure we don't try to stencil a tet that we just added
        //----------------------------------------------------------
        if(tet->output)
            continue;
        tet->output = true;

        //-----------------------------------
        // set parent to self
        //-----------------------------------
        int parent = tet->parent = t;

        //----------------------------------------
        // Prepare adjacency info for Stenciling
        //----------------------------------------
        Vertex *v[4];
        HalfEdge *edges[6];
        HalfFace *faces[4];
        m_bgMesh->getAdjacencyListsForTet(tet, v, edges, faces);

        bool stencil = false;
        int cut_count = 0;
        for(int e=0; e < EDGES_PER_TET; e++){
            cut_count += ((edges[e]->cut && edges[e]->cut->order()) == CUT ? 1 : 0);
            if(edges[e]->cut && edges[e]->cut->original_order() == 1)
                stencil = true;
        }

        //-- guard against failed generalization (won't guard against inconsistencies)
        for(int e=0; e < 6; e++)
        {
            if(edges[e]->cut == NULL)
            {
                std::cout << "Failed Generalization" << std::endl;
                stencil = false;
            }
        }
        for(int f=0; f < 4; f++)
        {
            if(faces[f]->triple == NULL)
            {
                std::cout << "Failed Generalization" << std::endl;
                stencil = false;
            }
        }
        if(tet->quadruple == NULL)
        {
            std::cout << "Failed Generalization" << std::endl;
            stencil = false;
        }

        //if((cut_count == 1 || cut_count == 2) && stencil == true)
        //    std::cerr << "Success for cut_count == 1 and cut_count == 2!!" << std::endl;
        if(!stencil && cut_count > 0){
            std::cerr << "Skipping ungeneralized tet with " << cut_count << " cuts." << std::endl;
            if(cut_count == 3){
                for(int f=0; f < 4; f++){
                    if(faces[f]->triple == NULL)
                        std::cerr << "Missing Triple T" << f+1 << std::endl;
                }
            }
        }

        if(stencil)
        {
            // add new stencil tets, and delete the old ones
            // 'replacing' the original tet with one of the new
            // output tets will guarantee we don't have to shift elements,
            // only add new ones.
            Vertex *verts[15];
            m_bgMesh->getRightHandedVertexList(tet, verts);

            bool first_tet = true;
            for(int st=0; st < 24; st++)
            {                                
                //---------------------------------------------
                //  Procede Only If Should Output Tets
                //---------------------------------------------
                if(stencilTable[st][0] == _O)
                    break;

                //---------------------------------------------
                //     Get Vertices
                //---------------------------------------------
                Vertex *v1 = verts[stencilTable[st][0]]->root();  // grabbing root ensures uniqueness
                Vertex *v2 = verts[stencilTable[st][1]]->root();
                Vertex *v3 = verts[stencilTable[st][2]]->root();
                Vertex *v4 = verts[stencilTable[st][3]]->root();
                Vertex *vM = verts[materialTable[st]]->root();

                //----------------------------------------------------------
                //  Ensure Tet Not Degenerate (all vertices must be unique)
                //----------------------------------------------------------
                if(v1 == v2 || v1 == v3 || v1 == v4 || v2 == v3 || v2 == v4 || v3 == v4)
                    continue;

                //----------------------------------------------------------------
                // Reconfigure Background Tet to become First Output Stencil Tet
                //----------------------------------------------------------------
                if(first_tet)
                {
                    first_tet = false;

                    //---------------------------------------
                    // Get Rid of Old Adjacency Information
                    //---------------------------------------
                    for(int v=0; v < 4; v++)
                    {
                        // check if tet is ever in there twice.. (regardless of whether it should be possible...)
                        int pc = 0;
                        for(size_t j=0; j < tet->verts[v]->tets.size(); j++)
                        {
                            if(tet->verts[v]->tets[j] == tet)
                            {
                                pc++;
                            }
                        }
                        if(pc > 1)
                        {
                            std::cout << "Vertex has a Tet stored TWICE in it. Bingo." << std::endl;
                            exit(0);
                        }

                        for(size_t j=0; j < tet->verts[v]->tets.size(); j++)
                        {
                            // Question:  Could tet be there twice?


                            // remove this tet from the list of all its vertices
                            if(tet->verts[v]->tets[j] == tet){
                                tet->verts[v]->tets.erase(tet->verts[v]->tets.begin() + j);
                                break;
                            }
                        }
                    }                    

                    //----------------------------------
                    // Insert New Defining Vertices
                    //----------------------------------
                    tet->verts[0] = v1;
                    tet->verts[1] = v2;
                    tet->verts[2] = v3;
                    tet->verts[3] = v4;
                    tet->mat_label = (int)vM->label;
                    total_changed++;

                    //----------------------------------
                    //  Repair Adjacency Information
                    //----------------------------------
                    for(int v=0; v < 4; v++)
                    {
                        if(tet->verts[v]->tm_v_index < 0)
                        {
                            tet->verts[v]->tm_v_index = m_bgMesh->verts.size();
                            m_bgMesh->verts.push_back(tet->verts[v]);
                        }
                        tet->verts[v]->tets.push_back(tet);
                    }

                }
                //---------------------------------
                //  Create New Tet + Add to List
                //---------------------------------
                else{
                    // create new ones for any extra
                    Tet *nst = m_bgMesh->createTet(v1, v2, v3, v4, (int)vM->label);
                    nst->parent = parent;
                    nst->output = true;  // so we don't come back to this output tet again
                    nst->key = tet->key;
                    total_output++;
                }
            }


            /*
            for(int v=0; v < 4; v++){
                if(tet->verts[v]->tm_v_index < 0)
                    std::cerr << "AH HA  1 !!" << std::endl;
            }
            */

        }
        else{

            // set tet to proper material
            total_changed++;
            tet->mat_label = tet->verts[0]->label;
            tet->tm_index = t;
            for(int v=0; v < 4; v++){
                if(tet->verts[v]->tm_v_index < 0){
                    tet->verts[v]->tm_v_index = m_bgMesh->verts.size();
                    m_bgMesh->verts.push_back(tet->verts[v]);
                }
                tet->verts[v]->tets.push_back(tet);
            }

            // sanity check
            for(int v=0; v < 4; v++){
                if(tet->verts[v]->tm_v_index < 0)
                    std::cerr << "AH HA  2  !!" << std::endl;
            }

        }

        // sanity check
        /*
        for(int v=0; v < 4; v++){
            if(tet->verts[v]->tm_v_index < 0)
                std::cerr << "WTF!!" << std::endl;
        }
        */
    }


    // mesh is now 'done'
    m_mesh = m_bgMesh;

    // recompute adjacency
    m_mesh->constructFaces();



    if(verbose){

        m_mesh->computeAngles();
        std::cout << "repurposed " << total_changed << " old tets." << std::endl;
        std::cout << "created " << total_output << " new tets." << std::endl;
        std::cout << "vert count: " << m_bgMesh->verts.size() << std::endl;
        std::cout << "tet  count: " << m_bgMesh->tets.size() << std::endl;
        std::cout << "Worst Angles:" << std::endl;
        std::cout << "\tmin: " << m_mesh->min_angle << std::endl;
        std::cout << "\tmax: " << m_mesh->max_angle << std::endl;

    }

    // set state
    m_bStencilsDone = true;

    if(verbose)
        std::cout << " done." << std::endl;

}

void CleaverMesherImp::topologicalCleaving()
{
    //----------------------------------------------------------
    // Precondition: A background mesh adapted to feature size
    //----------------------------------------------------------

    // Build adjacency incase it hasn't already
    buildAdjacency(true);

    // Sample The Volume
    //std::cout << "Sampling Volume for Topology" << std::endl;
    sampleVolume();

    // Compute Alphas
    //std::cout << "Computing Alphas for Topology" << std::endl;
    computeAlphas();

    // Compute Topological Interfaces
    computeTopologicalInterfaces(true);
    //computeInterfaces();

    // Generalize Tets
    generalizeTopologicalTets(true);
    //generalizeTets();

    // Snap & Warp
    //snapAndWarpViolations();

    // Stencil Background Tets
    //std::cout << "Stenciling background tets" << std::endl;
    stencilBackgroundTets(true);

    //std::cout << "resetting background mesh properties" << std::endl;
    resetMeshProperties();

    // just incase adjacency is now bad
    //buildAdjacency(true);

    // correct state of the program
    m_bBackgroundMeshCreated = true;
    m_bAdjacencyBuilt = false;
    m_bSamplingDone = false;
    m_bAlphasComputed = false;
    m_bInterfacesComputed = false;
    m_bGeneralized = false;
    m_bSnapsAndWarpsDone = false;
    m_bStencilsDone = false;
    m_bComplete = false;

}


//==========================================
// This method puts the background mesh into
// a state as if it had just been created.
// That means no cuts/triples/etc.
//==========================================
void CleaverMesherImp::resetMeshProperties()
{
    for(unsigned int v=0; v < m_bgMesh->verts.size(); v++)
    {
        Vertex *vert = m_bgMesh->verts[v];
        vert->closestGeometry = NULL;
        vert->conformedEdge = NULL;
        vert->conformedFace = NULL;
        vert->conformedVertex = NULL;
        vert->parent = NULL;
        vert->order() = VERT;
        vert->pos_next() = vert->pos();
        vert->violating = false;
        vert->warped = false;
        vert->phantom = false;
        vert->halfEdges.clear();
    }

    for(unsigned int i=0; i < m_bgMesh->tets.size(); i++)
    {
        Tet *tet = m_bgMesh->tets[i];
        tet->quadruple = NULL;
        tet->output = false;
    }
}

//=================================================
// Temporary Experimental Methods
// TODO:  The background meshers should
//        report the own times. The sizing
// field creator should report its own time.
//=================================================
void CleaverMesher::setSizingFieldTime(double time)
{
    m_pimpl->m_sizing_field_time = time;
}

void CleaverMesher::setBackgroundTime(double time)
{
    m_pimpl->m_background_time = time;
}

void CleaverMesher::setCleavingTime(double time)
{
    m_pimpl->m_cleaving_time = time;
}

double CleaverMesher::getSizingFieldTime() const
{
    return m_pimpl->m_sizing_field_time;
}

double CleaverMesher::getBackgroundTime() const
{
    return m_pimpl->m_background_time;
}

double CleaverMesher::getCleavingTime() const
{
    return m_pimpl->m_cleaving_time;
}

}
