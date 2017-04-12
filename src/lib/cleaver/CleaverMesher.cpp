#include "CleaverMesher.h"
#include "CleaverMesherImpl.h"
#include "StencilTable.h"
#include "TetMesh.h"
#include "OctreeMesher.h"
#include "ScalarField.h"
#include "BoundingBox.h"
#include "Plane.h"
#include "Matrix3x3.h"
#include "Volume.h"
#include "Timer.h"
#include "InterfaceCalculator.h"
#include "SizingFieldCreator.h"
#include "SizingFieldOracle.h"
#include "LinearInterfaceCalculator.h"
#include "TopologicalInterfaceCalculator.h"
#include "LinearViolationChecker.h"
#include "Status.h"
#include "vec3.h"
#include <queue>
#include <stack>
#include <map>
#include <cmath>
#include <cstdlib>


namespace cleaver
{
  static const double DEFAULT_ALPHA_LONG = 0.357;
  static const double DEFAULT_ALPHA_SHORT = 0.203;

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

    m_volume                 = nullptr;
    m_sizingField            = nullptr;
    m_sizingOracle           = nullptr;
    m_bgMesh                 = nullptr;
    m_mesh                   = nullptr;
    m_interfaceCalculator    = nullptr;
    m_violationChecker       = nullptr;

    m_sizing_field_time = 0;
    m_background_time   = 0;
    m_cleaving_time     = 0;
    m_alpha_init        = 0.4;
  }

  CleaverMesherImp::~CleaverMesherImp(){}

  // -- state getters --
  bool CleaverMesher::backgroundMeshCreated() const { return m_pimpl->m_bBackgroundMeshCreated; }
  bool CleaverMesher::adjacencyBuilt()        const { return m_pimpl->m_bAdjacencyBuilt; }
  bool CleaverMesher::samplingDone()          const { return m_pimpl->m_bSamplingDone; }
  bool CleaverMesher::alphasComputed()        const { return m_pimpl->m_bAlphasComputed; }
  bool CleaverMesher::interfacesComputed()    const { return m_pimpl->m_bInterfacesComputed; }
  bool CleaverMesher::generalized()           const { return m_pimpl->m_bGeneralized; }
  bool CleaverMesher::snapsAndWarpsDone()     const { return m_pimpl->m_bSnapsAndWarpsDone; }
  bool CleaverMesher::stencilsDone()          const { return m_pimpl->m_bStencilsDone; }
  bool CleaverMesher::completed()             const { return m_pimpl->m_bComplete; }


  CleaverMesher::~CleaverMesher() {
    // cleanup();
    delete m_pimpl;
  }

  CleaverMesher::CleaverMesher() : m_pimpl(new CleaverMesherImp)
  {
    m_alpha_long = DEFAULT_ALPHA_LONG;
    m_alpha_short = DEFAULT_ALPHA_SHORT;
    m_regular = false;
  }

  void CleaverMesher::createTetMesh(bool verbose)
  {
    m_pimpl->createBackgroundMesh(verbose);
    m_pimpl->buildAdjacency(verbose);
    m_pimpl->sampleVolume(verbose);
    m_pimpl->computeAlphas(verbose);
    m_pimpl->computeInterfaces(verbose);
    m_pimpl->generalizeTets(verbose);
    m_pimpl->snapAndWarpViolations(verbose);
    m_pimpl->stencilBackgroundTets(verbose);
  }

  // TODO(jonbronson): This method should not be necessary. Investigate cases
  // where it is actually changing mesh and track down root cause.
  size_t CleaverMesher::fixVertexWindup(bool verbose) {
    return m_pimpl->m_mesh->fixVertexWindup(verbose);
  }

  TetMesh* CleaverMesher::getBackgroundMesh() const
  {
    return m_pimpl->m_bgMesh;
  }

  TetMesh* CleaverMesher::getTetMesh() const
  {
    return m_pimpl->m_mesh;
  }


  Volume* CleaverMesher::getVolume() const
  {
    return m_pimpl->m_volume;
  }

  void CleaverMesher::cleanup()
  {
    if (m_pimpl->m_bgMesh)
      delete m_pimpl->m_bgMesh;
    m_pimpl->m_bgMesh = nullptr;
  }

  void CleaverMesher::setVolume(const Volume *volume)
  {
    cleanup();
    m_pimpl->m_volume = const_cast<Volume*>(volume);
  }

  void CleaverMesher::setTopologyMode(TopologyMode mode)
  {
    m_pimpl->m_topologyMode = mode;
  }

  void CleaverMesher::setAlphaInit(double alpha)
  {
    m_pimpl->m_alpha_init = alpha;
  }

  //================================================
  // createBackgroundMesh()
  //================================================
  TetMesh* CleaverMesherImp::createBackgroundMesh(bool verbose)
  {
    m_sizingField = m_volume->getSizingField();

    // Ensure We Have Sizing Field
    if (!m_sizingField) {
      throw std::runtime_error("Error: Sizing field missing from Volume.");
    }

    // Create the Octree Mesh
    OctreeMesher octreeMesher(m_sizingField);
    octreeMesher.createMesh();
    m_bgMesh = octreeMesher.getMesh();

    // set state
    m_bBackgroundMeshCreated = true;

    return m_bgMesh;
  }

  void CleaverMesherImp::setBackgroundMesh(TetMesh *mesh)
  {
    if (m_bgMesh)
      delete m_bgMesh;
    m_bgMesh = mesh;
    m_bBackgroundMeshCreated = true;
  }

  //========================================
  // - createBackgroundMesh()
  //========================================
  TetMesh* CleaverMesher::createBackgroundMesh(bool verbose)
  {
    cleaver::Timer timer;
    timer.start();
    TetMesh* m = m_pimpl->createBackgroundMesh(verbose);
    timer.stop();
    setBackgroundTime(timer.time());
    return m;
  }

  void CleaverMesher::setBackgroundMesh(TetMesh *m)
  {
    m_pimpl->setBackgroundMesh(m);
  }

  //==================================
  // - buildAdjacency();
  //==================================
  void CleaverMesher::buildAdjacency(bool verbose)
  {
    m_pimpl->buildAdjacency(verbose);
  }

  //================================
  // - SampleVolume()
  //================================
  void CleaverMesher::sampleVolume(bool verbose)
  {
    m_pimpl->sampleVolume(verbose);
  }

  //=================================
  // - computeAlphas()
  //=================================
  void CleaverMesher::computeAlphas(bool verbose)
  {
    m_pimpl->computeAlphas(verbose, m_regular, m_alpha_long, m_alpha_short);
  }

  //=====================================
  // - computeInterfaces()
  //=====================================
  void CleaverMesher::computeInterfaces(bool verbose)
  {
    m_pimpl->computeInterfaces(verbose);
  }

  //=====================================
  // - generalizeTets()
  //=====================================
  void CleaverMesher::generalizeTets(bool verbose)
  {
    m_pimpl->generalizeTets(verbose);
  }

  //================================
  // - snapAndWarp()
  //================================
  void CleaverMesher::snapsAndWarp(bool verbose)
  {
    m_pimpl->snapAndWarpViolations(verbose);
  }

  //===============================
  // - stencilTets()
  //===============================
  void CleaverMesher::stencilTets(bool verbose)
  {
    m_pimpl->stencilBackgroundTets(verbose);
  }

  //=================================================
  // - buildAdjacency()
  //=================================================
  void CleaverMesherImp::buildAdjacency(bool verbose)
  {
    if (verbose)
      std::cout << "Building Adjacency..." << std::flush;

    m_bgMesh->constructFaces();
    m_bgMesh->constructBottomUpIncidences(verbose);

    // set state
    m_bAdjacencyBuilt = true;

    if (verbose)
      std::cout << " done." << std::endl;
  }

  //=================================================
  // - sampleVolume()
  //=================================================
  void CleaverMesherImp::sampleVolume(bool verbose)
  {
    if (verbose)
      std::cout << "Sampling Volume..." << std::flush;

    Status status(m_bgMesh->verts.size());
    // Sample Each Background Vertex
    for (unsigned int v = 0; v < m_bgMesh->verts.size(); v++)
    {
      if (verbose) {
        status.printStatus();
      }
      // Get Vertex
      cleaver::Vertex *vertex = m_bgMesh->verts[v];

      // Grab Material Label
      vertex->label = m_volume->maxAt(vertex->pos());

      // added feb 20 to attempt boundary conforming
      if (!m_volume->bounds().contains(vertex->pos())) {
        vertex->isExterior = true;
        vertex->label = m_volume->numberOfMaterials();
      } else {
        vertex->isExterior = false;
      }
    }

    m_bgMesh->material_count = m_volume->numberOfMaterials();

    // set state
    m_bSamplingDone = true;

    if (verbose) {
      status.done();
      std::cout << " done." << std::endl;
    }
  }

  //================================================
  // - setAlphas()
  //================================================
  void CleaverMesher::setAlphas(double l, double s) {
    m_alpha_long = l;
    m_alpha_short = s;
  }

  //================================================
  // - setRegular()
  //================================================
  void CleaverMesher::setRegular(bool reg) {
    m_regular = reg;
  }

  //================================================
  // - computeAlphas()
  //================================================
  void CleaverMesherImp::computeAlphas(bool verbose,
    bool regular,
    double alp_long,
    double alp_short)
  {
    if (verbose)
      std::cout << "Computing Violation Alphas..." << std::flush;

    // set state
    m_bAlphasComputed = true;

    //---------------------------------------------------
    // set alpha_init for all edges in background mesh
    //---------------------------------------------------
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator edge_iter;

    for (edge_iter = m_bgMesh->halfEdges.begin(); edge_iter != m_bgMesh->halfEdges.end(); edge_iter++)
    {
      HalfEdge *half_edge = (*edge_iter).second;
      if (regular) {
        half_edge->alpha = (float)(half_edge->m_long_edge ? alp_long : alp_short);
      } else {
        half_edge->alpha = (float)m_alpha_init;
      }
      half_edge->alpha_length = (float)(half_edge->alpha*length(half_edge->vertex->pos() - half_edge->mate->vertex->pos()));

      if (regular) {
        half_edge->alpha = half_edge->alpha_length;
      }
    }

    computeAlphasSafely(verbose);

    if (verbose)
      std::cout << " done." << std::endl;
  }


  //=============================================================
  // - computeSafeAlphaLength()
  //=============================================================
  float CleaverMesherImp::computeSafeAlphaLength(Tet *tet, int v)
  {
    double xi = std::max(0.0, std::min(0.5 - m_alpha_init, 0.5));

    // construct normal plane
    Vertex *verts[3];   int idx = 0;
    for (int vid = 0; vid < 4; vid++)
    {
      if (vid == v)
        continue;
      verts[idx++] = tet->verts[vid];
    }

    Plane plane = Plane::throughPoints(verts[0]->pos(),
      verts[1]->pos(),
      verts[2]->pos());
    vec3 n = normalize(plane.n);

    vec3 ray = tet->verts[v]->pos() - verts[0]->pos();
    double altitude = std::abs(dot(n, ray));
    double safe_length = (0.5 - xi)*altitude;

    return safe_length;
  }


  //=============================================================
  // - updateAlphaLengthAroundVertex()
  //=============================================================
  void CleaverMesherImp::updateAlphaLengthAroundVertex(Vertex *vertex, float alpha_length)
  {
    std::vector<HalfEdge*> adj_edges = m_bgMesh->edgesAroundVertex(vertex);

    for (size_t e = 0; e < adj_edges.size(); e++)
    {
      HalfEdge *edge = adj_edges[e];
      if (alpha_length < edge->alphaLengthForVertex(vertex))
      {
        edge->setAlphaLengthForVertex(vertex, alpha_length);
      }
    }
  }

  //================================================
  // - makeTetAlphaSafe ()
  //================================================
  void CleaverMesherImp::makeTetAlphaSafe(Tet *tet)
  {
    // make each Vertex safe
    for (int v = 0; v < VERTS_PER_TET; v++)
    {
      // determine safe alpha
      float safe_alpha_length = computeSafeAlphaLength(tet, v);

      // set safe alpha for all edges around vertex
      updateAlphaLengthAroundVertex(tet->verts[v], safe_alpha_length);
      updateAlphaLengthAroundVertex(tet->verts[(v + 1) % 4], safe_alpha_length);
      updateAlphaLengthAroundVertex(tet->verts[(v + 2) % 4], safe_alpha_length);
      updateAlphaLengthAroundVertex(tet->verts[(v + 3) % 4], safe_alpha_length);
    }
  }

  //================================================
  // - computeAlphasSafely ()
  //================================================
  void CleaverMesherImp::computeAlphasSafely(bool verbose)
  {
    // make each tet safe
    for (size_t t = 0; t < m_bgMesh->tets.size(); t++)
    {
      makeTetAlphaSafe(m_bgMesh->tets[t]);
    }
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

    // TODO(jonbronson):  Inject these dependencies and use
    // setters to set the mesh and volume.
    if (m_interfaceCalculator)
      delete m_interfaceCalculator;
    m_interfaceCalculator = new LinearInterfaceCalculator(m_bgMesh, m_volume);

    if (m_violationChecker)
      delete m_violationChecker;
    m_violationChecker = new LinearViolationChecker(m_bgMesh);


    if (verbose)
      std::cout << "Computing Cuts..." << std::flush;

    {// DEBUG TEST
      std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
      while (iter != m_bgMesh->halfEdges.end())
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
    while (iter != m_bgMesh->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*iter).second;

      if (!edge->evaluated) {
        m_interfaceCalculator->computeCutForEdge(edge);
        if (edge->cut)
          cut_count++;
      }

      iter++;
    }

    if (verbose) {
      std::cout << " done. [" << cut_count << "]" << std::endl;
      std::cout << "Computing Triples..." << std::flush;
    }

    {// DEBUG TEST
      for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
      {
        cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];
        face->evaluated = false;
      }
    }

    //--------------------------------------
    // Compute Triples One Face At A Time
    //--------------------------------------
    for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
    {
      cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

      if (!face->evaluated) {
        m_interfaceCalculator->computeTripleForFace(face);
        if (face->triple) {
          m_violationChecker->checkIfTripleViolatesVertices(face);
          triple_count++;
        }
      }
    }

    if (verbose) {
      std::cout << " done. [" << triple_count << "]" << std::endl;
      std::cout << "Computing Quadruples..." << std::flush;
    }

    //-------------------------------------
    // Compute Quadruples One Tet At A Time
    //-------------------------------------
    for (unsigned int t = 0; t < m_bgMesh->tets.size(); t++)
    {
      cleaver::Tet *tet = m_bgMesh->tets[t];

      //if(!tet->evaluated){
      m_interfaceCalculator->computeQuadrupleForTet(tet);
      if (tet->quadruple)
        quadruple_count++;
      // }
    }

    if (verbose)
      std::cout << " done. [" << quadruple_count << "]" << std::endl;

    // set state
    m_bInterfacesComputed = true;


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

    // TODO(jonbronson):  Inject this dependenchy and use
    // setters to set the mesh and volume.
    if (m_interfaceCalculator)
      delete m_interfaceCalculator;
    m_interfaceCalculator = new TopologicalInterfaceCalculator(m_bgMesh, m_volume);


    if (verbose)
      std::cout << "Computing Topological Cuts..." << std::flush;

    {// DEBUG TEST
      std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_bgMesh->halfEdges.begin();
      while (iter != m_bgMesh->halfEdges.end())
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
    while (iter != m_bgMesh->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*iter).second;

      if (!edge->evaluated) {
        m_interfaceCalculator->computeCutForEdge(edge);
        if (edge->cut)
          cut_count++;
      }

      iter++;
    }


    if (verbose) {
      std::cout << " done. [" << cut_count << "]" << std::endl;
      //std::cout << "Computing Topological Triples..." << std::flush;
    }

    {// DEBUG TEST
      for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
      {
        cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];
        face->evaluated = false;
      }
    }

    //--------------------------------------
    // Compute Triples One Face At A Time
    //--------------------------------------
    for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
    {
      cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

      if (!face->evaluated) {
        //m_interfaceCalculator->computeTripleForFace(face);
        if (face->triple)
          triple_count++;
      }
    }

    if (verbose) {
      std::cout << " done. [" << triple_count << "]" << std::endl;
      std::cout << "Computing Quadruples..." << std::flush;
    }


    //-------------------------------------
    // Compute Quadruples One Tet At A Time
    //-------------------------------------
    for (unsigned int t = 0; t < m_bgMesh->tets.size(); t++)
    {
      cleaver::Tet *tet = m_bgMesh->tets[t];

      //if(!tet->evaluated){
      m_interfaceCalculator->computeQuadrupleForTet(tet);
      if (tet->quadruple)
        quadruple_count++;
      // }
    }


    if (verbose)
      std::cout << " done. [" << quadruple_count << "]" << std::endl;

    // set state
    m_bInterfacesComputed = true;

  }

  //====================================================================
  //   plane_intersect()
  //====================================================================
  bool plane_intersect(Vertex *v1, Vertex *v2, Vertex *v3, vec3 origin, vec3 ray, vec3 &pt, float epsilon = 1E-4)
  {
    //-------------------------------------------------
    // if v1, v2, and v3 are not unique, return FALSE
    //-------------------------------------------------
    if (v1->isEqualTo(v2) || v2->isEqualTo(v3) || v1->isEqualTo(v3))
      return false;
    else if (L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v1->pos() - v3->pos()) < epsilon)
      return false;


    vec3 p1 = origin;
    vec3 p2 = origin + ray;
    vec3 p3 = v1->pos();

    vec3 n = normalize(cross(normalize(v3->pos() - v1->pos()), normalize(v2->pos() - v1->pos())));

    double top = n.dot(p3 - p1);
    double bot = n.dot(p2 - p1);

    double t = top / bot;

    pt = origin + t*ray;

    if (pt != pt)
      return false;
    else
      return true;
  }

  //---------------------------------------------------
  //  generalizeTets()
  //---------------------------------------------------
  void CleaverMesherImp::generalizeTets(bool verbose)
  {
    if (verbose)
      std::cout << "Generalizing Tets..." << std::flush;

    //--------------------------------------
    // Loop over all tets that contain cuts
    //--------------------------------------
    // (For Now, Looping over ALL tets)

    Status status(m_bgMesh->tets.size());
    for (unsigned int t = 0; t < m_bgMesh->tets.size(); t++)
    {
      if (verbose) {
        status.printStatus();
      }
      cleaver::Tet *tet = m_bgMesh->tets[t];

      //------------------------------
      // if no quad, start generalization
      //------------------------------
      if (tet && !tet->quadruple)
      {
        // look up generalization
        Vertex *verts[4];
        HalfEdge *edges[6];
        HalfFace *faces[4];

        m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

        int cut_count = 0;
        for (int e = 0; e < 6; e++)
          cut_count += (edges[e]->cut && (edges[e]->cut->order() == Order::CUT) ? 1 : 0);


        //------------------------------
        // determine virtual edge cuts
        //------------------------------
        for (int e = 0; e < 6; e++) {
          if (edges[e]->cut == nullptr)
          {
            // always go towards the smaller index
            if (edges[e]->vertex->tm_v_index < edges[e]->mate->vertex->tm_v_index)
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
        for (int f = 0; f < 4; f++)
        {
          if (faces[f]->triple == nullptr)
          {
            HalfEdge *e[3];
            Vertex   *v[3];
            m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

            int v_count = 0;
            int v_e = 0;
            for (int i = 0; i < 3; i++)
            {
              if (e[i]->cut->order() != Order::CUT)
              {
                v_count++;
                v_e = i;   // save index to virtual edge
              }
            }

            // move to edge virtual cut went to
            if (v_count == 1)
            {
              for (int i = 0; i < 3; i++)
              {
                // skip edge that has virtual cut
                if (i == v_e)
                  continue;

                if (e[i]->vertex == e[v_e]->cut || e[i]->mate->vertex == e[v_e]->cut)
                {
                  faces[f]->triple = e[i]->cut;
                  break;
                }
              }
            }
            // move to minimal index vertex
            else if (v_count == 3)
            {
              if ((v[0]->tm_v_index < v[1]->tm_v_index) && v[0]->tm_v_index < v[2]->tm_v_index)
                faces[f]->triple = v[0];
              else if ((v[1]->tm_v_index < v[2]->tm_v_index) && v[1]->tm_v_index < v[0]->tm_v_index)
                faces[f]->triple = v[1];
              else
                faces[f]->triple = v[2];
            } else
            {
              std::cerr << "HUGE PROBLEM: virtual count = " << v_count << std::endl;
              for (int j = 0; j < 3; j++) {
                if (v[j]->isExterior)
                  std::cout << "But it's Exterior!" << std::endl;
              }

              exit(8);
            }

            // copy info to mate face if it exists
            if (faces[f]->mate)
              faces[f]->mate->triple = faces[f]->triple;
          }
        }

        //------------------------------
        // determine virtual quadruple
        //------------------------------
        if (cut_count == 3)
        {
          if (faces[0]->triple == faces[1]->triple ||
            faces[0]->triple == faces[2]->triple ||
            faces[0]->triple == faces[3]->triple)
            tet->quadruple = faces[0]->triple;
          else if (faces[1]->triple == faces[2]->triple ||
            faces[1]->triple == faces[3]->triple)
            tet->quadruple = faces[1]->triple;
          else if (faces[2]->triple == faces[3]->triple)
            tet->quadruple = faces[2]->triple;
        } else if (cut_count == 4)
        {
          for (int f = 0; f < 4; f++)
          {
            if (faces[f]->triple->order() < Order::TRIP && (faces[(f + 1) % 4]->triple == faces[f]->triple ||
              faces[(f + 2) % 4]->triple == faces[f]->triple ||
              faces[(f + 3) % 4]->triple == faces[f]->triple))
            {
              tet->quadruple = faces[f]->triple;
              break;
            }
          }
        } else if (cut_count == 5)
        {
          for (int f = 0; f < 4; f++)
          {
            if (faces[f]->triple->order() == Order::TRIP)
            {
              tet->quadruple = faces[f]->triple;
              break;
            }
          }
        } else // 0
        {
          for (int f = 0; f < 4; f++)
          {
            if (faces[f]->triple->order() < Order::TRIP)
            {
              tet->quadruple = faces[f]->triple;
              break;
            }
          }
        }

        if (tet->quadruple == nullptr)
        {
          std::cerr << "Generalization Failed!!" << std::endl;
          std::cerr << "problem tet contains " << cut_count << " cuts." << std::endl;
        }

        if (tet->quadruple->order() < Order::VERT)
          std::cerr << "GOT YA!" << std::endl;

      }
    }

    // set state
    m_bGeneralized = true;

    if (verbose) {
      status.done();
      std::cout << " done." << std::endl;
    }
  }

  //=======================================
  // generalizeTopologicalTets()
  //=======================================
  void CleaverMesherImp::generalizeTopologicalTets(bool verbose)
  {
    if (verbose)
      std::cout << "Generalizing Tets..." << std::flush;

    //--------------------------------------
    // Loop over all tets that contain cuts
    //--------------------------------------
    // (For Now, Looping over ALL tets)

    for (unsigned int t = 0; t < m_bgMesh->tets.size(); t++)
    {
      cleaver::Tet *tet = m_bgMesh->tets[t];
      bool case_is_v2 = false;

      //------------------------------
      // if no quad, start generalization
      //------------------------------
      {
        // look up generalization
        Vertex *verts[4];
        HalfEdge *edges[6];
        HalfFace *faces[4];

        m_bgMesh->getAdjacencyListsForTet(tet, verts, edges, faces);

        int cut_count = 0;
        for (int e = 0; e < 6; e++)
          cut_count += ((edges[e]->cut && (edges[e]->cut->order() == Order::CUT)) ? 1 : 0);  // to do, this bug probably appaers in regular
                                                                                      // generalization function, fix it!
      //------------------------------
      // determine virtual edge cuts
      //------------------------------
        for (int e = 0; e < 6; e++) {
          if (edges[e]->cut == nullptr)
          {
            // always go towards the smaller index
            if (edges[e]->vertex->tm_v_index < edges[e]->mate->vertex->tm_v_index)
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
        for (int f = 0; f < 4; f++)
        {
          if (faces[f]->triple == nullptr)
          {
            HalfEdge *e[3];
            Vertex   *v[3];
            m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

            int virtual_count = 0;
            int v_e = 0;
            for (int i = 0; i < 3; i++)
            {
              if (e[i]->cut->order() != Order::CUT)
              {
                virtual_count++;
                v_e = i;   // save index to virtual edge
              }
            }
            // 3 cuts, but no triple? Make one
            if (virtual_count == 0)
            {
              std::cout << "got a 3-cuts but no triple case" << std::endl;
              exit(19);
              // technically this case should never happen, but if
              // it does, let's just make a triple point in the center
              faces[f]->triple = new Vertex(m_volume->numberOfMaterials());
              faces[f]->triple->pos() = (1.0 / 3.0)*(v[0]->pos() + v[1]->pos() + v[2]->pos());
              faces[f]->triple->order() = Order::TRIP;
              faces[f]->triple->lbls[v[0]->label] = true;
              faces[f]->triple->lbls[v[1]->label] = true;
              faces[f]->triple->lbls[v[2]->label] = true;
              faces[f]->triple->violating = false;
              faces[f]->triple->closestGeometry = nullptr;
              if (faces[f]->mate)
                faces[f]->mate->triple = faces[f]->triple;
            }
            // move triple to edge virtual cut went to
            else if (virtual_count == 1)
            {
              for (int i = 0; i < 3; i++)
              {
                // skip edge that has virtual cut
                if (i == v_e)
                  continue;

                if (e[i]->vertex == e[v_e]->cut || e[i]->mate->vertex == e[v_e]->cut)
                {
                  faces[f]->triple = e[i]->cut;
                  if (faces[f]->mate)
                    faces[f]->mate->triple = e[i]->cut;
                  break;
                }
              }
              if (faces[f]->triple == nullptr) {
                std::cerr << "WTF??" << std::endl;
                exit(12412);
              }
            }
            // move triple to the vertex opposite the real cut
            else if (virtual_count == 2)
            {
              //if(cut_count == 1 || cut_count == 2 || cut_count == 3){
              for (int i = 0; i < 3; i++) {
                if (e[i]->cut && e[i]->cut->order() == Order::CUT)
                {
                  Vertex *v1 = e[i]->vertex;
                  Vertex *v2 = e[i]->mate->vertex;
                  Vertex *op;

                  HalfEdge *en1 = e[(i + 1) % 3];
                  HalfEdge *en2 = e[(i + 2) % 3];
                  if (en1->vertex != v1 && en1->vertex != v2)
                    op = en1->vertex;
                  else
                    op = en1->mate->vertex;

                  faces[f]->triple = e[i]->cut;
                  if (faces[f]->mate)
                    faces[f]->mate->triple = e[i]->cut;
                  if (faces[f]->triple == nullptr) {
                    std::cerr << "triple block with vcount=2 failed!" << std::endl;
                    exit(12321);
                  }

                  break;
                }
              }
              case_is_v2 = true;
            }
            // move triple to minimal index vertex
            else if (virtual_count == 3)
            {
              if ((v[0]->tm_v_index < v[1]->tm_v_index) && v[0]->tm_v_index < v[2]->tm_v_index)
                faces[f]->triple = v[0];
              else if ((v[1]->tm_v_index < v[2]->tm_v_index) && v[1]->tm_v_index < v[0]->tm_v_index)
                faces[f]->triple = v[1];
              else
                faces[f]->triple = v[2];
            }

            // copy info to mate face if it exists
            if (faces[f]->mate)
              faces[f]->mate->triple = faces[f]->triple;
          }

          if (cut_count == 2 && faces[f]->triple == nullptr)
          {
            std::cerr << "Finding a  triple failed for 2-cut tet" << std::endl;
            exit(12412);
          }
        }

        //------------------------------
        // determine virtual quadruple
        //------------------------------
        if (tet->quadruple == nullptr)
        {

          if (cut_count == 1)
          {
            // put quad on the cut edge
            for (int e = 0; e < 6; e++) {
              if (edges[e]->cut && edges[e]->cut->order() == Order::CUT)
                tet->quadruple = edges[e]->cut;
            }
          } else if (cut_count == 2)
          {
            // put quad on the cut that was chosen by triple
            for (int f = 0; f < 4; f++) {
              int count = 0;
              HalfEdge *e[3];
              Vertex   *v[3];
              m_bgMesh->getAdjacencyListsForFace(faces[f], v, e);

              for (int i = 0; i < 3; i++)
              {
                if (e[i]->cut->order() == Order::CUT)
                {
                  count++;
                }
              }
              // found the face
              if (count == 2) {
                tet->quadruple = faces[f]->triple;
                if (tet->quadruple == nullptr) {
                  std::cerr << "PROBLEM!" << std::endl;
                  exit(12412);
                }
                break;
              }
            }
            if (tet->quadruple == nullptr) {
              //std::cerr << "looks like 2 cuts aren't on incident edges" << std::endl;
              //exit(1412);

              // put on either cut, both should have 2 triples on them
              tet->quadruple = faces[0]->triple;

            }
          } else if (cut_count == 3)
          {
            if (faces[0]->triple == faces[1]->triple ||
              faces[0]->triple == faces[2]->triple ||
              faces[0]->triple == faces[3]->triple)
              tet->quadruple = faces[0]->triple;
            else if (faces[1]->triple == faces[2]->triple ||
              faces[1]->triple == faces[3]->triple)
              tet->quadruple = faces[1]->triple;
            else if (faces[2]->triple == faces[3]->triple)
              tet->quadruple = faces[2]->triple;
          } else if (cut_count == 4)
          {
            bool found = false;
            for (int f = 0; f < 4; f++)
            {
              if (faces[f]->triple->order() < Order::TRIP && (faces[(f + 1) % 4]->triple == faces[f]->triple ||
                faces[(f + 2) % 4]->triple == faces[f]->triple ||
                faces[(f + 3) % 4]->triple == faces[f]->triple))
              {
                tet->quadruple = faces[f]->triple;
                found = true;
                break;
              }
            }
            if (!found) {
              // there's no 2 triples co-located, select any of them
              tet->quadruple = faces[0]->triple;
            }
          } else if (cut_count == 5)
          {
            bool found = false;
            for (int f = 0; f < 4; f++)
            {
              if (faces[f]->triple->order() == Order::TRIP)
              {
                tet->quadruple = faces[f]->triple;
                found = true;
                break;
              }
            }
            if (!found) {
              // there's no real triple, select any of them
              tet->quadruple = faces[0]->triple;
            }
          } else // 0
          {
            for (int f = 0; f < 4; f++)
            {
              if (faces[f]->triple && faces[f]->triple->order() < Order::TRIP)
              {
                tet->quadruple = faces[f]->triple;
                break;
              }
            }
          }
        }



        if (!(cut_count == 0 ||
          cut_count == 1 ||
          cut_count == 2 ||
          cut_count == 3 ||
          cut_count == 4 ||
          cut_count == 5 ||
          cut_count == 6))
        {
          tet->quadruple = nullptr;
        }

        if (cut_count == 3) {

          bool around_v1 = false;
          bool around_v2 = false;
          bool around_v3 = false;
          bool around_v4 = false;

          if (edges[0]->cut->order() == Order::CUT && edges[1]->cut->order() == Order::CUT && edges[2]->cut->order() == Order::CUT)
            around_v1 = true;
          if (edges[0]->cut->order() == Order::CUT && edges[3]->cut->order() == Order::CUT && edges[4]->cut->order() == Order::CUT)
            around_v2 = true;
          if (edges[1]->cut->order() == Order::CUT && edges[3]->cut->order() == Order::CUT && edges[5]->cut->order() == Order::CUT)
            around_v3 = true;
          if (edges[2]->cut->order() == Order::CUT && edges[4]->cut->order() == Order::CUT && edges[5]->cut->order() == Order::CUT)
            around_v4 = true;
        }



        if (tet->quadruple == nullptr)
        {
          std::cerr << "Generalization Failed!!" << std::endl;
          std::cerr << "problem tet contains " << cut_count << " cuts." << std::endl;

          std::cerr << "v1, v2, v3, v4, ";
          for (int e = 0; e < 6; e++) {
            if (edges[e]->cut && edges[e]->cut->order() == Order::CUT)
              std::cerr << "c" << e + 1 << ", ";
          }
          for (int f = 0; f < 4; f++) {
            if (faces[f]->triple && faces[f]->triple->order() == Order::TRIP)
              std::cerr << "t" << t + 1 << ", ";
          }
          std::cerr << std::endl;
          for (int f = 0; f < 4; f++) {
            if (!faces[f]->triple)
              std::cout << "Missing triple T" << f + 1 << std::endl;
          }
        }
        for (int f = 0; f < 4; f++)
        {
          if (faces[f]->triple == nullptr) {
            std::cerr << "Generalization Failed!!" << std::endl;
            std::cerr << "missing a triple" << std::endl;
          }
        }
        for (int e = 0; e < 6; e++)
        {
          if (edges[e]->cut == nullptr) {
            std::cerr << "Generalization Failed!!" << std::endl;
            std::cerr << "missing a cut" << std::endl;
          }
        }

      }
    }


    // set state
    m_bGeneralized = true;

    if (verbose)
      std::cout << " done." << std::endl;
  }

  //=======================================
  //    Executes all Snaps and Warps
  //=======================================
  void CleaverMesherImp::snapAndWarpViolations(bool verbose)
  {
    if (verbose)
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

    if (verbose)
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
    if (verbose) {
      std::cout << "preparing to examine " << m_bgMesh->verts.size() << " verts" << std::endl;
    }
    Status status(m_bgMesh->verts.size());
    for (unsigned int v = 0; v < m_bgMesh->verts.size(); v++)
    {
      if (verbose) {
        status.printStatus();
      }
      Vertex *vertex = m_bgMesh->verts[v];            // TODO: add check for vertex->hasAdjacentCuts
      snapAndWarpForViolatedVertex(vertex);           //       to reduce workload significantly.
    }
    if (verbose) {
      status.done();
      std::cout << "Phase 1 Complete" << std::endl;
    }
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

    for (int i = 0; i < 3; i++)
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
    vec3 inv1, inv2, inv3;
    vec3 v1 = warpPt;
    vec3 v2 = verts[1]->pos();
    vec3 v3 = verts[2]->pos();
    vec3 v4 = v1 + normalize(cross(normalize(v3 - v1), normalize(v2 - v1)));

    // Fill Coordinate Matrix
    A[0][0] = v1.x - v4.x; A[0][1] = v2.x - v4.x; A[0][2] = v3.x - v4.x;
    A[1][0] = v1.y - v4.y; A[1][1] = v2.y - v4.y; A[1][2] = v3.y - v4.y;
    A[2][0] = v1.z - v4.z; A[2][1] = v2.z - v4.z; A[2][2] = v3.z - v4.z;

    // Solve Inverse
    double det = +A[0][0] * (A[1][1] * A[2][2] - A[2][1] * A[1][2])
                - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
                + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    double invdet = 1 / det;
    inv1.x =  (A[1][1] * A[2][2] - A[2][1] * A[1][2])*invdet;
    inv2.x = -(A[1][0] * A[2][2] - A[1][2] * A[2][0])*invdet;
    inv3.x =  (A[1][0] * A[2][1] - A[2][0] * A[1][1])*invdet;

    inv1.y = -(A[0][1] * A[2][2] - A[0][2] * A[2][1])*invdet;
    inv2.y =  (A[0][0] * A[2][2] - A[0][2] * A[2][0])*invdet;
    inv3.y = -(A[0][0] * A[2][1] - A[2][0] * A[0][1])*invdet;

    inv1.z =  (A[0][1] * A[1][2] - A[0][2] * A[1][1])*invdet;
    inv2.z = -(A[0][0] * A[1][2] - A[1][0] * A[0][2])*invdet;
    inv3.z =  (A[0][0] * A[1][1] - A[1][0] * A[0][1])*invdet;

    // Multiply Inverse*Coordinate to get Lambda (Barycentric)
    vec3 lambda;
    lambda.x = inv1.x*(triple.x - v4.x) + inv1.y*(triple.y - v4.y) + inv1.z*(triple.z - v4.z);
    lambda.y = inv2.x*(triple.x - v4.x) + inv2.y*(triple.y - v4.y) + inv2.z*(triple.z - v4.z);
    lambda.z = inv3.x*(triple.x - v4.x) + inv3.y*(triple.y - v4.y) + inv3.z*(triple.z - v4.z);

    //--------------------------------------------------------------
    // Is any coordinate negative?
    // If so, make it 0, adjust other weights but keep ratio
    //--------------------------------------------------------------
    if (lambda.x < EPS) {
      lambda.x = 0;

      for (int i = 0; i < EDGES_PER_FACE; i++) {
        if (edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[2])) {
          trip->conformedEdge = edges[i];
          break;
        }
      }
    } else if (lambda.y < EPS) {
      lambda.y = 0;

      for (int i = 0; i < EDGES_PER_FACE; i++) {
        if (edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[2])) {
          trip->conformedEdge = edges[i];
          break;
        }
      }
    } else if (lambda.z < EPS) {
      lambda.z = 0;

      for (int i = 0; i < EDGES_PER_FACE; i++) {
        if (edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[1])) {
          trip->conformedEdge = edges[i];
          break;
        }
      }
    } else
    {
      trip->conformedEdge = nullptr;
    }

    lambda /= L1(lambda);

    // Compute New Triple Coordinate
    triple.x = lambda.x*v1.x + lambda.y*v2.x + lambda.z*v3.x;
    triple.y = lambda.x*v1.y + lambda.y*v2.y + lambda.z*v3.y;
    triple.z = lambda.x*v1.z + lambda.y*v2.z + lambda.z*v3.z;

    if (triple == vec3::zero || triple != triple)
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

    quad->conformedFace = nullptr;
    quad->conformedEdge = nullptr;
    quad->conformedVertex = nullptr;

    for (int i = 0; i < 4; i++)
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
    vec3 inv1, inv2, inv3;
    vec3 v1 = warpPt;
    vec3 v2 = verts[1]->pos();
    vec3 v3 = verts[2]->pos();
    vec3 v4 = verts[3]->pos();

    // Fill Coordinate Matrix
    A[0][0] = v1.x - v4.x; A[0][1] = v2.x - v4.x; A[0][2] = v3.x - v4.x;
    A[1][0] = v1.y - v4.y; A[1][1] = v2.y - v4.y; A[1][2] = v3.y - v4.y;
    A[2][0] = v1.z - v4.z; A[2][1] = v2.z - v4.z; A[2][2] = v3.z - v4.z;

    // Solve Inverse
    double det = +A[0][0] * (A[1][1] * A[2][2] - A[2][1] * A[1][2])
                - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
                + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    double invdet = 1 / det;
    inv1.x =  (A[1][1] * A[2][2] - A[2][1] * A[1][2])*invdet;
    inv2.x = -(A[1][0] * A[2][2] - A[1][2] * A[2][0])*invdet;
    inv3.x =  (A[1][0] * A[2][1] - A[2][0] * A[1][1])*invdet;

    inv1.y = -(A[0][1] * A[2][2] - A[0][2] * A[2][1])*invdet;
    inv2.y =  (A[0][0] * A[2][2] - A[0][2] * A[2][0])*invdet;
    inv3.y = -(A[0][0] * A[2][1] - A[2][0] * A[0][1])*invdet;

    inv1.z =  (A[0][1] * A[1][2] - A[0][2] * A[1][1])*invdet;
    inv2.z = -(A[0][0] * A[1][2] - A[1][0] * A[0][2])*invdet;
    inv3.z =  (A[0][0] * A[1][1] - A[1][0] * A[0][1])*invdet;

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

    if (lambda.x < EPS) {

      // two negatives
      if (lambda.y < EPS) {

        lambda.x = 0;
        lambda.y = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[2]) && edges[i]->incidentToVertex(verts[3])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      } else if (lambda.z < EPS) {

        lambda.x = 0;
        lambda.z = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[3])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      } else if (lambda_w < EPS) {

        lambda.x = 0;
        lambda_w = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[1]) && edges[i]->incidentToVertex(verts[2])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      }
      // one negative
      else {

        lambda.x = 0;

        for (int i = 0; i < 4; i++)
        {
          if (!faces[i]->incidentToVertex(verts[0])) {
            quad->conformedFace = faces[i];
            //cout << "Conformed to Face" << endl;
            break;
          }
        }
      }

    } else if (lambda.y < EPS) {
      // two negatives
      if (lambda.z < EPS) {

        lambda.y = 0;
        lambda.z = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[3])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      } else if (lambda_w < EPS) {

        lambda.y = 0;
        lambda_w = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[2])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      }
      // one negative
      else {

        lambda.y = 0;

        for (int i = 0; i < 4; i++)
        {
          if (!faces[i]->incidentToVertex(verts[1])) {
            quad->conformedFace = faces[i];
            //cout << "Conformed to Face" << endl;
            break;
          }
        }
      }
    } else if (lambda.z < EPS) {
      // two negatives
      if (lambda_w < EPS) {

        lambda.z = 0;
        lambda_w = 0;

        for (int i = 0; i < 6; i++) {
          if (edges[i]->incidentToVertex(verts[0]) && edges[i]->incidentToVertex(verts[1])) {
            quad->conformedEdge = edges[i];
            //cout << "conformed to Edge" << endl;
            break;
          }
        }
      }
      // one negative
      else {

        lambda.z = 0;

        for (int i = 0; i < 4; i++)
        {
          if (!faces[i]->incidentToVertex(verts[2])) {
            quad->conformedFace = faces[i];
            //cout << "Conformed to Face" << endl;
            break;
          }
        }
      }
    } else if (lambda_w < EPS)
    {
      // one negative
      lambda_w = 0;

      for (int i = 0; i < 4; i++)
      {
        if (!faces[i]->incidentToVertex(verts[3])) {
          quad->conformedFace = faces[i];
          //cout << "Conformed to Face" << endl;
          break;
        }
      }
    } else
    {
      quad->conformedFace = nullptr;
      quad->conformedEdge = nullptr;
      quad->conformedVertex = nullptr;
    }

    if (quad->conformedVertex != nullptr) {
      std::cerr << "unhandled exception: quad->conformedVertex != nullptr" << std::endl;
      exit(-1);
    }

    double L1 = lambda.x + lambda.y + lambda.z + lambda_w;
    lambda /= L1;
    if ((fabs(lambda.x - 0.5) > 0.5 + EPS) ||
      (fabs(lambda.y - 0.5) > 0.5 + EPS) ||
      (fabs(lambda.z - 0.5) > 0.5 + EPS) ||
      (fabs((1.0 - (lambda.x + lambda.y + lambda.z)) - 0.5) > 0.5 + EPS)) {
      std::cout << "WARNING : Quadruple point failed to snap into the tet!" << std::endl;
    }

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

    for (unsigned int e = 0; e < incidentEdges.size(); e++)
    {
      HalfEdge *edge = incidentEdges[e];
      if (edge->cut->order() == Order::CUT)
      {
        if (edge->cut->violating && edge->cut->closestGeometry == vertex)
          viol_edges.push_back(edge);
        else
          part_edges.push_back(edge);
      }
    }

    //---------------------------------------------------------
    // Add Participating & Violating TriplePoints   (Faces)
    //---------------------------------------------------------
    std::vector<HalfFace*> incidentFaces = m_bgMesh->facesAroundVertex(vertex);

    for (unsigned int f = 0; f < incidentFaces.size(); f++)
    {
      HalfFace *face = incidentFaces[f];

      if (face->triple->order() == Order::TRIP)
      {
        if (face->triple->violating && face->triple->closestGeometry == vertex)
          viol_faces.push_back(face);
        else
          part_faces.push_back(face);
      }
    }

    //---------------------------------------------------------
    // Add Participating & Violating QuaduplePoints   (Tets)
    //---------------------------------------------------------
    std::vector<Tet*> incidentTets = m_bgMesh->tetsAroundVertex(vertex);

    for (unsigned int t = 0; t < incidentTets.size(); t++)
    {
      Tet *tet = incidentTets[t];
      if (tet->quadruple->order() == Order::QUAD)
      {
        if (tet->quadruple->violating && tet->quadruple->closestGeometry == vertex)
          viol_tets.push_back(tet);
        else
          part_tets.push_back(tet);
      }
    }

    //-----------------------------------------
    // If no violations, move to next vertex
    //-----------------------------------------
    if (viol_edges.empty() && viol_faces.empty() && viol_tets.empty())
    {
      return;
    }


    //-----------------------------------------
    // Compute Warp Point
    //-----------------------------------------
    vec3 warp_point = vec3::zero;

    // If 1 Quadpoint is Violating, take quad position
    if (viol_tets.size() == 1)
    {
      warp_point = viol_tets[0]->quadruple->pos();
    }
    // Else If 1 Triplepoint is Violating
    else if (viol_faces.size() == 1)
    {
      warp_point = viol_faces[0]->triple->pos();
    }
    // Otherwise Take Center of Mass
    else
    {
      for (unsigned int i = 0; i < viol_edges.size(); i++)
        warp_point += viol_edges[i]->cut->pos();

      for (unsigned int i = 0; i < viol_faces.size(); i++)
        warp_point += viol_faces[i]->triple->pos();

      for (unsigned int i = 0; i < viol_tets.size(); i++)
        warp_point += viol_tets[i]->quadruple->pos();

      warp_point /= viol_edges.size() + viol_faces.size() + viol_tets.size();
    }

    //---------------------------------------
    //  Conform Quadruple Pt (if it exists)
    //---------------------------------------
    for (unsigned int t = 0; t < part_tets.size(); t++)
    {
      conformQuadruple(part_tets[t], vertex, warp_point);
    }


    //---------------------------------------------------------
    // Project Any TriplePoints That Survive On A Warped Face
    //---------------------------------------------------------

    // WARNING: Comparing edges must also compare against MATE edges.
    //          Added checks here for safety, but issue might exist
    //          elsewhere. Keep an eye out.
    for (unsigned int f = 0; f < part_faces.size(); f++)
    {
      HalfFace *face = part_faces[f];
      Tet  *innerTet = getInnerTet(face, vertex, warp_point);
      Vertex     *q = innerTet->quadruple;

      // conform triple if it's also a quad (would not end up in part_quads list)
      if (q->isEqualTo(face->triple)) {
        conformQuadruple(innerTet, vertex, warp_point);
      }
      // coincide with Quad if it conformed to the face
      else if (q->order() == Order::QUAD && q->conformedFace->sameAs(face))
      {
        part_faces[f]->triple->pos_next() = q->pos_next();
        part_faces[f]->triple->conformedEdge = nullptr;
      }
      // coincide with Quad if it conformed to one of faces edges
      else if (q->order() == Order::QUAD && (q->conformedEdge->sameAs(face->halfEdges[0]) ||
        q->conformedEdge->sameAs(face->halfEdges[1]) ||
        q->conformedEdge->sameAs(face->halfEdges[2])))
      {
        part_faces[f]->triple->pos_next() = q->pos_next();
        part_faces[f]->triple->conformedEdge = q->conformedEdge;
      }
      // otherwise intersect lattice face with Q-T interface
      else {
        part_faces[f]->triple->pos_next() = projectTriple(part_faces[f], q, vertex, warp_point);
        conformTriple(part_faces[f], vertex, warp_point);
      }
    }


    //------------------------------------------------------
    // Project Any Cutpoints That Survived On A Warped Edge
    //------------------------------------------------------
    for (unsigned int e = 0; e < part_edges.size(); e++)
    {
      HalfEdge *edge = part_edges[e];
      Tet *innertet = getInnerTet(edge, vertex, warp_point);

      std::vector<HalfFace*> faces = m_bgMesh->facesAroundEdge(edge);

      bool handled = false;
      for (unsigned int f = 0; f < faces.size(); f++)
      {
        // if triple conformed to this edge, use it's position
        if (faces[f]->triple->order() == Order::TRIP && faces[f]->triple->conformedEdge->sameAs(part_edges[e])) {
          part_edges[e]->cut->pos_next() = faces[f]->triple->pos_next();
          handled = true;
          break;
        }
      }

      if (handled && edge->cut->pos_next() == vec3::zero)
        std::cerr << "Conformed Cut Problem!" << std::endl;

      // TODO: What about conformedVertex like quadpoint?
      // otherwise compute projection with innerTet
      if (!handled)
        edge->cut->pos_next() = projectCut(edge, innertet, vertex, warp_point);


      if (edge->cut->pos_next() == vec3::zero)
        std::cerr << "Cut Projection Problem!" << std::endl;

    }


    //------------------------------------
    //   Update Vertices
    //------------------------------------
    vertex->pos() = warp_point;
    vertex->warped = true;

    // move remaining cuts and check for violation
    for (unsigned int e = 0; e < part_edges.size(); e++)
    {
      HalfEdge *edge = part_edges[e];
      edge->cut->pos() = edge->cut->pos_next();
      m_violationChecker->checkIfCutViolatesVertices(edge);
    }
    // move remaining triples and check for violation
    for (unsigned int f = 0; f < part_faces.size(); f++)
    {
      HalfFace *face = part_faces[f];
      face->triple->pos() = face->triple->pos_next();
      m_violationChecker->checkIfTripleViolatesVertices(face);
    }
    // move remaining quadruples and check for violation
    for (unsigned int t = 0; t < part_tets.size(); t++) {
      Tet *tet = part_tets[t];
      tet->quadruple->pos() = tet->quadruple->pos_next();
      m_violationChecker->checkIfQuadrupleViolatesVertices(tet);
    }

    //------------------------------------------------------
    // Delete cuts of the same interface type
    //------------------------------------------------------
    // TODO:  Incorporate Triple and Quadpoint Material Values Checks
    for (unsigned int e = 0; e < part_edges.size(); e++) {

      // check if same as one of the violating interface types
      bool affected = false;

      for (unsigned int c = 0; c < viol_edges.size() && !affected; c++)
      {
        bool same = true;
        for (int m = 0; m < m_volume->numberOfMaterials(); m++) {
          if (part_edges[e]->cut->lbls[m] != viol_edges[c]->cut->lbls[m]) {
            same = false;
            break;
          }
        }
        affected = same;
      }

      //-----------------------
      // If Affected, Snap it
      //-----------------------
      if (affected)
      {
        snapCutForEdgeToVertex(part_edges[e], vertex);
      }
    }


    //------------------------------------------------------------------
    //  Delete Cut if Projection Makes it Violate New Vertex Location
    //------------------------------------------------------------------
    for (unsigned int e = 0; e < part_edges.size(); e++)
    {
      Vertex *cut = part_edges[e]->cut;

      if (cut->order() == Order::CUT && cut->violating)
      {
        // if now violating this vertex, snap to it
        if (cut->closestGeometry == vertex)
          snapCutForEdgeToVertex(part_edges[e], vertex);

        // else if violating an already warped vertex, snap to it
        else if (((Vertex*)cut->closestGeometry)->warped)
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
    for (unsigned int f = 0; f < part_faces.size(); f++)
    {
      Vertex *triple = part_faces[f]->triple;

      if (triple->order() == Order::TRIP && triple->violating)
      {
        // if now violating this vertex, snap to it
        if (triple->closestGeometry == vertex)
        {
          snapTripleForFaceToVertex(part_faces[f], vertex);
        }
        // else if violating an already warped vertex, snap to it
        else if (((Vertex*)triple->closestGeometry)->warped)
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
    for (unsigned int t = 0; t < part_tets.size(); t++)
    {
      Vertex *quadruple = part_tets[t]->quadruple;

      // TODO: This should follow same logic as Triple Deletion Above.
      if (quadruple->order() == Order::QUAD && quadruple->violating && quadruple->closestGeometry == vertex)
      {
        snapQuadrupleForTetToVertex(part_tets[t], vertex);
      }
    }

    //------------------------
    // Delete All Violations
    //------------------------
    // 1) cuts
    for (unsigned int e = 0; e < viol_edges.size(); e++)
      snapCutForEdgeToVertex(viol_edges[e], vertex);


    // 2) triples
    for (unsigned int f = 0; f < viol_faces.size(); f++)
      snapTripleForFaceToVertex(viol_faces[f], vertex);


    // 3) quadruples
    for (unsigned int t = 0; t < viol_tets.size(); t++)
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
    if (v1 == v2 || v2 == v3 || v1 == v3)
      return false;
    else if (L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v1->pos() - v3->pos()) < epsilon)
      return false;

    //----------------------------------------------
    // compute intersection with plane, store in pt
    //----------------------------------------------
    vec3 e1 = v1->pos() - v3->pos();
    vec3 e2 = v2->pos() - v3->pos();

    ray = normalize(ray);
    vec3 r1 = ray.cross(e2);
    double denom = e1.dot(r1);

    if (fabs(denom) < epsilon)
      return false;

    double inv_denom = 1.0 / denom;
    vec3 s = origin - v3->pos();
    double b1 = s.dot(r1) * inv_denom;

    if (b1 < (0.0 - epsilon2) || b1 >(1.0 + epsilon2))
      return false;

    vec3 r2 = s.cross(e1);
    double b2 = ray.dot(r2) * inv_denom;

    if (b2 < (0.0 - epsilon2) || (b1 + b2) >(1.0 + 2 * epsilon2))
      return false;

    double t = e2.dot(r2) * inv_denom;
    pt = origin + t*ray;


    if (t < 0.01)
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

    if (edge->vertex == warpVertex)
      static_vertex = edge->vertex;
    else
      static_vertex = edge->mate->vertex;

    vec3 origin = 0.5*(edge->vertex->pos() + edge->mate->vertex->pos());  //edge->cut->pos(); //static_vertex->pos(); //
    vec3 ray = warpPt - origin;

    for (unsigned int t = 0; t < tets.size(); t++)
    {
      std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

      for (int f = 0; f < FACES_PER_TET; f++)
      {
        std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);

        if (triangle_intersection(verts[0], verts[1], verts[2], origin, ray, hit_pt))
        {
          if (L2(edge->cut->pos() - hit_pt) > 1E-3)
            return tets[t];
        }
      }
    }


    // if none hit, make a less picky choice
    for (unsigned int t = 0; t < tets.size(); t++)
    {
      std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

      for (int f = 0; f < 4; f++)
      {
        std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);

        if (triangle_intersection(verts[0], verts[1], verts[2], origin, ray, hit_pt))
        {
          return tets[t];
        }
      }
    }

    // if STILL none hit, we have a problem
    std::cerr << "WARNING: Failed to find Inner Tet for Edge" << std::endl;
    //exit(-1);

    return nullptr;
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

    // if on boundary, return only neighbor tet  (added Mar 14/2013)
    if (tets.size() == 1)
      return tets[0];

    std::vector<Vertex*> verts_a = m_bgMesh->vertsAroundTet(tets[0]);
    std::vector<Vertex*> verts_b = m_bgMesh->vertsAroundTet(tets[1]);

    // sort them so exterior vertex is first
    for (int v = 0; v < 4; v++) {
      if (!face->incidentToVertex(verts_a[v]))
        swap(verts_a[0], verts_a[v]);

      if (!face->incidentToVertex(verts_b[v]))
        swap(verts_b[0], verts_b[v]);
    }

    vec3 vec_a = normalize(verts_a[0]->pos() - face->triple->pos());
    vec3 vec_b = normalize(verts_b[0]->pos() - face->triple->pos());
    vec3 n = normalize(cross(verts_a[3]->pos() - verts_a[1]->pos(), verts_a[2]->pos() - verts_a[1]->pos()));

    float dot1 = (float)dot(vec_a, ray);
    float dot2 = (float)dot(vec_b, ray);

    if (dot1 > dot2)
      return tets[0];
    else
      return tets[1];


    // if neither hit, we have a problem
    std::cerr << "Fatal Error:  Failed to find Inner Tet for Face" << std::endl;
    exit(-1);
    return nullptr;
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

    for (int i = 0; i < 3; i++)
    {
      if (verts[i] == warpVertex)
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
    if (length(l) < 1E-5 || dot(l, n) == 0)
      return trip->pos();

    double d = dot(p_0 - I_a, n) / dot(l, n);


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
    if (v1->isEqualTo(v2) || v2->isEqualTo(v3) || v3->isEqualTo(v1))
    {
      pt = vec3(-2, -2, -2); // Debug J.R.B. 11/22/11
      return false;
    } else if (L2(v1->pos() - v2->pos()) < epsilon || L2(v2->pos() - v3->pos()) < epsilon || L2(v3->pos() - v1->pos()) < epsilon)
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
    vec3 inv1, inv2, inv3;
    vec3 p1 = v1->pos();
    vec3 p2 = v2->pos();
    vec3 p3 = v3->pos();
    vec3 p4 = p1 + normalize(cross(normalize(p3 - p1), normalize(p2 - p1)));

    // Fill Coordinate Matrix
    A[0][0] = p1.x - p4.x; A[0][1] = p2.x - p4.x; A[0][2] = p3.x - p4.x;
    A[1][0] = p1.y - p4.y; A[1][1] = p2.y - p4.y; A[1][2] = p3.y - p4.y;
    A[2][0] = p1.z - p4.z; A[2][1] = p2.z - p4.z; A[2][2] = p3.z - p4.z;

    // Solve Inverse
    double det = +A[0][0] * (A[1][1] * A[2][2] - A[2][1] * A[1][2])
      - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
      + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    double invdet = 1 / det;
    inv1.x = (A[1][1] * A[2][2] - A[2][1] * A[1][2])*invdet;
    inv2.x = -(A[1][0] * A[2][2] - A[1][2] * A[2][0])*invdet;
    inv3.x = (A[1][0] * A[2][1] - A[2][0] * A[1][1])*invdet;

    inv1.y = -(A[0][1] * A[2][2] - A[0][2] * A[2][1])*invdet;
    inv2.y = (A[0][0] * A[2][2] - A[0][2] * A[2][0])*invdet;
    inv3.y = -(A[0][0] * A[2][1] - A[2][0] * A[0][1])*invdet;

    inv1.z = (A[0][1] * A[1][2] - A[0][2] * A[1][1])*invdet;
    inv2.z = -(A[0][0] * A[1][2] - A[1][0] * A[0][2])*invdet;
    inv3.z = (A[0][0] * A[1][1] - A[1][0] * A[0][1])*invdet;

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
    if (c.dot(b) < 0)
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
    if (edge->vertex->isExterior || edge->mate->vertex->isExterior)
      return edge->cut->pos();

    // Handle Case if No Inner Tet was FounD (like exterior of volume) TODO: Evaluate this case carefully
    if (tet == nullptr) {
      vec3 pt = vec3::zero;
      Vertex *static_vertex = nullptr;

      if (edge->vertex == warpVertex)
        static_vertex = edge->mate->vertex;
      else
        static_vertex = edge->vertex;

      double t = length(edge->cut->pos() - static_vertex->pos()) / length(warpVertex->pos() - static_vertex->pos());
      pt = static_vertex->pos() + t*(warpPt - static_vertex->pos());
      return pt;
    }
    //----------------

    Vertex *static_vertex = nullptr;
    Vertex *quad = tet->quadruple;
    Vertex *verts[15] = { 0 };
    m_bgMesh->getRightHandedVertexList(tet, verts);

    if (edge->vertex == warpVertex)
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
    bool   valid[12] = { 0 };

    std::vector<vec3> interface_verts;

    // check intersection with each triangle face
    for (int i = 0; i < 12; i++)
    {
      if (completeInterfaceTable[i][0] < 0)
        break;

      Vertex *v1 = verts[completeInterfaceTable[i][0]];
      Vertex *v2 = verts[completeInterfaceTable[i][1]];
      Vertex *v3 = quad;

      if (v1->isEqualTo(edge->cut) || v2->isEqualTo(edge->cut) || v3->isEqualTo(edge->cut) ||
        L2(v1->pos() - edge->cut->pos()) < 1E-7 || L2(v2->pos() - edge->cut->pos()) < 1E-7 || L2(v3->pos() - edge->cut->pos()) < 1E-7)
      {
        valid[i] = triangle_intersect(v1, v2, v3, static_pt, ray, point[i], error[i], 1E-2);
      } else {
        // skip it
      }
    }


    // pick the one with smallest error
    for (int i = 0; i < 12; i++)
    {
      if (valid[i] && error[i] < min_error)
      {
        pt = point[i];
        min_error = error[i];
      }
    }

    // if no intersections, don't move it at all
    if (pt == vec3::zero) {
      pt = edge->cut->pos();
    }

    // Conform Point!!!
    vec3 newray = pt - static_vertex->pos();
    double t1 = newray.x / ray.x;

    if (t1 < 0 || t1 > 1)
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
    Status status(m_bgMesh->tets.size() * 5 + m_bgMesh->halfEdges.size());
    for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
    {
      if (verbose) {
        status.printStatus();
      }
      cleaver::HalfFace *face = &m_bgMesh->halfFaces[f];

      if (face->triple && face->triple->order() == Order::TRIP)
        m_violationChecker->checkIfTripleViolatesEdges(face);
    }
    // then check quadruples violating edges
    for (unsigned int t = 0; t < m_bgMesh->tets.size(); t++)
    {
      if (verbose) {
        status.printStatus();
      }
      cleaver::Tet *tet = m_bgMesh->tets[t];
      if (tet->quadruple && tet->quadruple->order() == Order::QUAD)
        m_violationChecker->checkIfQuadrupleViolatesEdges(tet);
    }

    //---------------------------------------------------
    //  Apply snapping to all remaining edge-cuts
    //---------------------------------------------------
    std::map<std::pair<int, int>, HalfEdge*>::iterator edgesIter = m_bgMesh->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while (edgesIter != m_bgMesh->halfEdges.end())
    {
      if (verbose) {
        status.printStatus();
      }
      HalfEdge *edge = (*edgesIter).second;    // TODO: add  redundancy checks
      snapAndWarpForViolatedEdge(edge);        //           to reduce workload.
      edgesIter++;
    }
    if (verbose) {
      status.done();
      std::cout << "Phase 2 Complete" << std::endl;
    }
  }

  //===============================================================
  //  Snap and Warp Violations Surrounding an Edge
  //===============================================================
  void CleaverMesherImp::snapAndWarpForViolatedEdge(HalfEdge *edge)
  {
    // look at each adjacent face
    std::vector<HalfFace*> faces = m_bgMesh->facesAroundEdge(edge);

    for (unsigned int f = 0; f < faces.size(); f++)
    {
      Vertex *triple = faces[f]->triple;

      if (triple->order() == Order::TRIP &&
        triple->violating &&
        (triple->closestGeometry == edge || triple->closestGeometry == edge->mate))
      {
        snapTripleForFaceToCut(faces[f], edge->cut);
      }
    }

    // If triples went to a vertex, resolve degeneracies on that vertex
    if (edge->cut->order() == Order::VERT)
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
    Status status(4 * m_bgMesh->tets.size());
    for (unsigned int f = 0; f < 4 * m_bgMesh->tets.size(); f++)
    {
      if (verbose) {
        status.printStatus();
      }
      HalfFace *face = &m_bgMesh->halfFaces[f];  // TODO: add  redundancy checks
      snapAndWarpForViolatedFace(face);         //           to reduce workload.
    }
    if (verbose) {
      status.done();
      std::cout << "Phase 3 Complete" << std::endl;
    }
  }


  //===============================================================
  //  Snap and Warp Violations Surrounding a Face
  //===============================================================
  void CleaverMesherImp::snapAndWarpForViolatedFace(HalfFace *face)
  {

    std::vector<Tet*> tets = m_bgMesh->tetsAroundFace(face);

    for (unsigned int t = 0; t < tets.size(); t++)
    {
      Vertex *quadruple = tets[t]->quadruple;

      if (quadruple->order() == Order::QUAD && quadruple->violating && (quadruple->closestGeometry == face || quadruple->closestGeometry == face->mate))
      {
        // Snap to triple point, wherever it happens to be
        snapQuadrupleForTetToTriple(tets[t], face->triple);


        // check order of vertex now pointed to
        switch (tets[t]->quadruple->order())
        {
        case Order::VERT:
        {
          // If Triple_point is on a Vertex
          resolveDegeneraciesAroundVertex(tets[t]->quadruple->root());
          break;
        }
        case Order::CUT:
        {
          for (unsigned int e = 0; e < EDGES_PER_FACE; e++)
          {
            HalfEdge *edge = face->halfEdges[e];

            if (edge->cut->isEqualTo(tets[t]->quadruple))
            {
              snapQuadrupleForTetToEdge(tets[t], edge);
              resolveDegeneraciesAroundEdge(edge);
            }
          }
          break;
        }
        case Order::TRIP:
        {
          // If Triple-Point is on a Face, do nothing
          break;
        }
        default:
        {
          std::cerr << "Fatal Error - Quad order == " << (int)tets[t]->quadruple->order() << std::endl;
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
    if (edge->cut->original_order() == Order::CUT)
      edge->cut->parent = vertex;
    else {
      edge->cut = vertex;
      edge->mate->cut = vertex;
    }
  }

  //======================================================
  // - snapTripleToVertex()
  //======================================================
  void CleaverMesherImp::snapTripleForFaceToVertex(HalfFace *face, Vertex* vertex)
  {
    if (face->triple->original_order() == Order::TRIP)
      face->triple->parent = vertex;
    else {
      face->triple = vertex;
      if (face->mate)
        face->mate->triple = vertex;
    }

  }

  //=====================================================================
  // - snapTripleToCut()
  //=====================================================================
  void CleaverMesherImp::snapTripleForFaceToCut(HalfFace *face, Vertex *cut)
  {
    if (face->triple->original_order() == Order::TRIP)
      face->triple->parent = cut;
    else {
      face->triple = cut;
      if (face->mate)
        face->mate->triple = cut;
    }
  }

  //============================================================
  // - snapQuadrupleToVertex()
  //============================================================
  void CleaverMesherImp::snapQuadrupleForTetToVertex(Tet *tet, Vertex* vertex)
  {
    if (tet->quadruple->original_order() == Order::QUAD)
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
    if (tet->quadruple->original_order() == Order::QUAD)
      tet->quadruple->parent = cut;
    else
      tet->quadruple = cut;
  }

  //=====================================================================
  // - snapQuadrupleForTetToTriple
  //=====================================================================
  void CleaverMesherImp::snapQuadrupleForTetToTriple(Tet *tet, Vertex *triple)
  {
    if (tet->quadruple->original_order() == Order::QUAD)
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
    if (!tet->quadruple->isEqualTo(edge->cut)) {
      snapQuadrupleForTetToCut(tet, edge->cut);
    }

    // Get Adjacent TripleFaces
    std::vector<HalfFace*> adjFaces = m_bgMesh->facesIncidentToBothTetAndEdge(tet, edge);

    for (unsigned int f = 0; f < 2; f++)
    {
      // if still a triple, snap it
      if (adjFaces[f]->triple->order() == Order::TRIP)
      {
        snapTripleForFaceToCut(adjFaces[f], edge->cut);

        Tet *opTet = m_bgMesh->oppositeTetAcrossFace(tet, adjFaces[f]);

        // if adjacent Tet quadpoint is snapped to this triple, snap it next
        if (opTet && opTet->quadruple->isEqualTo(adjFaces[f]->triple))
          snapQuadrupleForTetToEdge(opTet, edge);

      }

      // if snapped to a different edge
      else if (adjFaces[f]->triple->order() == Order::CUT && !(adjFaces[f]->triple->isEqualTo(edge->cut)))
      {
        Tet *opTet = m_bgMesh->oppositeTetAcrossFace(tet, adjFaces[f]);

        // if adjacent Tet quadpoint is snapped to this triple, snap it next
        if (opTet && opTet->quadruple->isEqualTo(adjFaces[f]->triple))
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
    while (changed)
    {
      changed = false;

      //--------------------------------------------------------------------------
      // Snap Any Triples or Cuts that MUST follow a Quadpoint
      //--------------------------------------------------------------------------
      for (unsigned int t = 0; t < tets.size(); t++)
      {
        Tet *tet = tets[t];

        // If Quadpoint is snapped to Vertex
        if (tet->quadruple->isEqualTo(vertex))
        {
          // Check if any cuts exist to snap
          std::vector<HalfEdge*> edges = m_bgMesh->edgesAroundTet(tet);
          for (int e = 0; e < EDGES_PER_TET; e++)
          {
            // cut exists & spans the vertex in question
            if (edges[e]->cut->order() == Order::CUT && (edges[e]->vertex == vertex || edges[e]->mate->vertex == vertex))
            {
              snapCutForEdgeToVertex(edges[e], vertex);
              changed = true;
            }
          }

          // Check if any triples exist to snap
          std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tet);
          for (int f = 0; f < FACES_PER_TET; f++)
          {
            // triple exists & spans the vertex in question
            if (faces[f]->triple->order() == Order::TRIP)
            {
              std::vector<Vertex*> verts = m_bgMesh->vertsAroundFace(faces[f]);
              if (verts[0] == vertex || verts[1] == vertex || verts[2] == vertex)
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
      for (unsigned int f = 0; f < faces.size(); f++)
      {
        // If Triplepoint is snapped to Vertex
        if (faces[f] && faces[f]->triple->isEqualTo(vertex))
        {
          // Check if any cuts exist to snap
          for (int e = 0; e < EDGES_PER_FACE; e++)
          {
            HalfEdge *edge = faces[f]->halfEdges[e];
            // cut exists & spans the vertex in question
            if (edge->cut->order() == Order::CUT && (edge->vertex == vertex || edge->mate->vertex == vertex))
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
      for (unsigned int f = 0; f < faces.size(); f++)
      {
        if (faces[f] && faces[f]->triple->order() == Order::TRIP)
        {
          // count # cuts snapped to vertex
          int count = 0;
          for (int e = 0; e < EDGES_PER_FACE; e++) {
            HalfEdge *edge = faces[f]->halfEdges[e];
            count += (int)edge->cut->isEqualTo(vertex);
          }

          // if two cuts have snapped to vertex, triple degenerates
          if (count == 2)
          {
            snapTripleForFaceToVertex(faces[f], vertex);
            changed = true;
          }
        }
      }

      //------------------------------------------------------------------------------------
      // Snap Any Quads that have now degenerated
      //------------------------------------------------------------------------------------
      for (unsigned int t = 0; t < tets.size(); t++)
      {
        if (tets[t] && tets[t]->quadruple->order() == Order::QUAD)
        {
          std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

          // count # trips snapped to vertex
          int count = 0;
          for (int f = 0; f < FACES_PER_TET; f++)
            count += (int)faces[f]->triple->isEqualTo(vertex);

          // if 3 triples have snapped to vertex, quad degenerates
          if (count == 3)
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
    for (unsigned int t = 0; t < tets.size(); t++)
    {
      if (tets[t]->quadruple->isEqualTo(edge->cut))
      {
        snapQuadrupleForTetToEdge(tets[t], edge);
      }
    }

    //--------------------------------------------------------------------------
    // Snap Any Quads that have now degenerated onto the Edge
    //--------------------------------------------------------------------------
    for (unsigned int t = 0; t < tets.size(); t++)
    {
      if (tets[t]->quadruple->order() == Order::QUAD)
      {
        std::vector<HalfFace*> faces = m_bgMesh->facesAroundTet(tets[t]);

        // count # triples snaped to edge
        int count = 0;
        for (int f = 0; f < FACES_PER_TET; f++)
          count += (int)faces[f]->triple->isEqualTo(edge->cut);

        // if two triples have snapped to edgecut, quad degenerates
        if (count == 2)
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

    for (int i = 0; i < 15; i++)
    {
      if (verts[i] == nullptr)
        verts_copy[i] = nullptr;
      else {
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
    if (verbose)
      std::cout << "Filling in Stencils..." << std::endl;

    // be safe, and remove ALL old adjacency info on tets
    Status status(m_bgMesh->verts.size() + m_bgMesh->tets.size());
    for (unsigned int v = 0; v < m_bgMesh->verts.size(); v++)
    {
      if (verbose) {
        status.printStatus();
      }
      Vertex *vertex = m_bgMesh->verts[v];
      vertex->tets.clear();
      vertex->tm_v_index = -1;
    }
    m_bgMesh->verts.clear();

    int total_output = 0;
    int total_changed = 0;

    TetMesh *debugMesh;
    bool debugging = false;
    if (debugging)
      debugMesh = new TetMesh();


    //-------------------------------
    // Naively Examine All Tets
    //-------------------------------
    for (size_t t = 0; t < m_bgMesh->tets.size(); t++)
    {
      if (verbose) {
        status.printStatus();
      }
      // ----------------------------------------
      // Grab Handle to Current Background Tet
      // ----------------------------------------
      Tet *tet = m_bgMesh->tets[t];

      //----------------------------------------------------------
      // Ensure we don't try to stencil a tet that we just added
      //----------------------------------------------------------
      if (tet->output)
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
      for (int e = 0; e < EDGES_PER_TET; e++) {
        cut_count += ((edges[e]->cut && edges[e]->cut->order() == Order::CUT) ? 1 : 0);
        if (edges[e]->cut && edges[e]->cut->original_order() == Order::CUT)
          stencil = true;
      }

      //-- guard against failed generalization (won't guard against inconsistencies)
      for (int e = 0; e < 6; e++)
      {
        if (edges[e]->cut == nullptr)
        {
          std::cout << "Failed Generalization" << std::endl;
          stencil = false;
        }
      }
      for (int f = 0; f < 4; f++)
      {
        if (faces[f]->triple == nullptr)
        {
          std::cout << "Failed Generalization" << std::endl;
          stencil = false;
        }
      }
      if (tet->quadruple == nullptr)
      {
        std::cout << "Failed Generalization" << std::endl;
        stencil = false;
      }

      //if((cut_count == 1 || cut_count == 2) && stencil == true)
      //    std::cerr << "Success for cut_count == 1 and cut_count == 2!!" << std::endl;
      if (!stencil && cut_count > 0) {
        std::cerr << "Skipping ungeneralized tet with " << cut_count << " cuts." << std::endl;
        if (cut_count == 3) {
          for (int f = 0; f < 4; f++) {
            if (faces[f]->triple == nullptr)
              std::cerr << "Missing Triple T" << f + 1 << std::endl;
          }
        }
      }

      if (stencil)
      {
        // add new stencil tets, and delete the old ones
        // 'replacing' the original tet with one of the new
        // output tets will guarantee we don't have to shift elements,
        // only add new ones.
        Vertex *verts[15];
        m_bgMesh->getRightHandedVertexList(tet, verts);

        bool first_tet = true;
        for (int st = 0; st < 24; st++)
        {
          //---------------------------------------------
          //  Procede Only If Should Output Tets
          //---------------------------------------------
          if (stencilTable[st][0] == _O)
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
          if (v1 == v2 || v1 == v3 || v1 == v4 || v2 == v3 || v2 == v4 || v3 == v4)
            continue;

          //----------------------------------------------------------------
          // Reconfigure Background Tet to become First Output Stencil Tet
          //----------------------------------------------------------------
          if (first_tet)
          {
            first_tet = false;

            //---------------------------------------
            // Get Rid of Old Adjacency Information
            //---------------------------------------
            for (int v = 0; v < 4; v++)
            {
              // check if tet is ever in there twice.. (regardless of whether it should be possible...)
              int pc = 0;
              for (size_t j = 0; j < tet->verts[v]->tets.size(); j++)
              {
                if (tet->verts[v]->tets[j] == tet)
                {
                  pc++;
                }
              }
              if (pc > 1)
              {
                std::cout << "Vertex has a Tet stored TWICE in it. Bingo." << std::endl;
                exit(0);
              }

              for (size_t j = 0; j < tet->verts[v]->tets.size(); j++)
              {
                // Question:  Could tet be there twice?


                // remove this tet from the list of all its vertices
                if (tet->verts[v]->tets[j] == tet) {
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
            for (int v = 0; v < 4; v++)
            {
              if (tet->verts[v]->tm_v_index < 0)
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
          else {
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

      } else {

        // set tet to proper material
        total_changed++;
        tet->mat_label = tet->verts[0]->label;
        tet->tm_index = t;
        for (int v = 0; v < 4; v++) {
          if (tet->verts[v]->tm_v_index < 0) {
            tet->verts[v]->tm_v_index = m_bgMesh->verts.size();
            m_bgMesh->verts.push_back(tet->verts[v]);
          }
          tet->verts[v]->tets.push_back(tet);
        }

        // sanity check
        for (int v = 0; v < 4; v++) {
          if (tet->verts[v]->tm_v_index < 0)
            std::cerr << "AH HA  2  !!" << std::endl;
        }

      }
    }
    if (verbose) {
      status.done();
    }

    // mesh is now 'done'
    m_mesh = m_bgMesh;

    // recompute adjacency
    m_mesh->constructFaces();



    if (verbose) {

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

    if (verbose)
      std::cout << " done." << std::endl;

  }

  void CleaverMesherImp::topologicalCleaving()
  {
    //----------------------------------------------------------
    // Precondition: A background mesh adapted to feature size
    //----------------------------------------------------------

    // Build adjacency incase it hasn't already
    buildAdjacency(false);

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
    for (unsigned int v = 0; v < m_bgMesh->verts.size(); v++)
    {
      Vertex *vert = m_bgMesh->verts[v];
      vert->closestGeometry = nullptr;
      vert->conformedEdge = nullptr;
      vert->conformedFace = nullptr;
      vert->conformedVertex = nullptr;
      vert->parent = nullptr;
      vert->order() = Order::VERT;
      vert->pos_next() = vert->pos();
      vert->violating = false;
      vert->warped = false;
      vert->phantom = false;
      vert->halfEdges.clear();
    }

    for (unsigned int i = 0; i < m_bgMesh->tets.size(); i++)
    {
      Tet *tet = m_bgMesh->tets[i];
      tet->quadruple = nullptr;
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
