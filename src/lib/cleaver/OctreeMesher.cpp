#include "OctreeMesher.h"

#include "vec3.h"
#include "Octree.h"
#include "SizingFieldOracle.h"

#include <cmath>
#include <map>
#include <queue>
#include <stack>

namespace cleaver
{

namespace{
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
bool SPLIT_ACROSS_CELLS = false;

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

} // namespace



//------------------ private implementation
class OctreeMesherImp
{
public:
    OctreeMesherImp(const AbstractScalarField *sizing_field = nullptr);
    ~OctreeMesherImp();

    void createOracle();
    void createOctree();
    void balanceOctree();
    void createBackgroundVerts();
    void createBackgroundTets();
    void cleanup();

    void adaptCell(OTCell *cell);
    Vertex* vertexForPosition(const vec3 &pos, bool create=true);
    int heightForPath(OTCell *cell, int path, int depth = 0);

    const AbstractScalarField *m_sizing_field;
    const SizingFieldOracle   *m_sizing_oracle;



    cleaver::TetMesh *m_mesh;

    Octree *m_tree;
    std::map<vec3, Vertex*, vec3order> m_vertex_tracker;
    std::map<vec3,    vec3, vec3order> m_warp_tracker;
};

OctreeMesherImp::OctreeMesherImp(const cleaver::AbstractScalarField *sizing_field) :
    m_mesh(nullptr), m_tree(nullptr), m_sizing_field(sizing_field), m_sizing_oracle(nullptr)
{
}

OctreeMesherImp::~OctreeMesherImp()
{
    if(m_tree)
        delete m_tree;

    if(m_sizing_oracle)
        delete m_sizing_oracle;
}

//============================================
// - createOracle()
//============================================
void OctreeMesherImp::createOracle()
{
    const BoundingBox bounds = m_sizing_field->bounds();
    m_sizing_oracle = new SizingFieldOracle(m_sizing_field, bounds);
}

//============================================
// - createOctree()
//============================================
void OctreeMesherImp::createOctree()
{
    // Get Bounds
    BoundingBox bounds = m_sizing_field->bounds();

    // Create Octree
    if(m_tree)
      delete m_tree;
    m_tree = new Octree(bounds);

    // breadth first creation
    adaptCell(m_tree->root());
}

//======================================================
// - balanceOctreeNew()
//======================================================
void OctreeMesherImp::balanceOctree()
{
  // first create reverse breadth first list of leaves
  std::queue<OTCell*> q;
  std::stack<OTCell*> s;

  q.push(m_tree->root());

  while (!q.empty())
  {
    OTCell *cell = q.front();

    // ignore bottom leaves, they can't split
    if (cell->level == 0)
    {
      q.pop();
      continue;
    }

    // if there are children, enqueue them
    if (cell->hasChildren())
    {
      for (int i = 0; i < 8; i++)
        q.push(cell->children[i]);
    }
    // else put this leaf onto stack
    else
      s.push(cell);

    // done with this node
    q.pop();
  }

  // now have reverse breadth first list of leaves
  while (!s.empty())
  {
    OTCell *cell = s.top();

    // ignore bottom leaves, they can't split
    if (cell->level == 0)
    {
      s.pop();
      continue;
    }

    // if no children check, if branch needed
    if (!cell->hasChildren())
    {
      // look in all directions, excluding diagonals (need to subdivide?)
      for (int i = 0; i < 18; i++)
      {
        OTCell *neighbor = m_tree->getNeighborAtLevel(cell, i, cell->level);

        if (neighbor && heightForPath(neighbor, heightPairs[i]) > 2)
        {
          cell->subdivide();
          break;
        }
      }
    }

    // done with this node
    s.pop();

    // if there are children now, push them on stack
    if (cell->hasChildren())
    {
      for (int i = 0; i < 8; i++)
        s.push(cell->children[i]);
    }
  }

}


//====================================================
// - heightForPath()
//====================================================

int OctreeMesherImp::heightForPath(OTCell *cell, int path, int depth)
{
  int height = 1;
  depth++;
  if (depth == 3)
    return height;

  if (cell->children[0]) {

    int max_height = 0;
    for (int i = 0; i < 4; i++)
      max_height = std::max(max_height, heightForPath(cell->children[heightPaths[path][i]], path, depth));

    height += max_height;
  }

  return height;
}


//============================================
// - adaptCell()
//============================================
void OctreeMesherImp::adaptCell(OTCell *cell)
{
  if(!cell)
    return;

  BoundingBox domainBounds = m_sizing_field->bounds();

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
         cell->bounds.maxCorner().y <= max_y &
         cell->bounds.maxCorner().z <= max_z)
 {
   cell->celltype = OTCell::Inside;
 }
 // otherwise it straddles
 else
   cell->celltype = OTCell::Straddles;

  BoundingBox bounds = cell->bounds;

    /*
    vec3 tx = vec3((bounds.center().x / m_volume->bounds().size.x)*m_sizingField->bounds().size.x,
                   (bounds.center().y / m_volume->bounds().size.y)*m_sizingField->bounds().size.y,
                   (bounds.center().z / m_volume->bounds().size.z)*m_sizingField->bounds().size.z);

    float LFS = m_sizingField->valueAt(tx);
    */

  double LFS = m_sizing_oracle->getMinLFS(cell->xLocCode, cell->yLocCode, cell->zLocCode, cell->level);

  if(LFS < bounds.size.x)
    cell->subdivide();

  if(cell->hasChildren()){
    for(int i=0; i < 8; i++)
    {
      adaptCell(cell->children[i]);
    }
  }
}

//============================================
// - createBackgroundVerts()
//============================================
void OctreeMesherImp::createBackgroundVerts()
{
  std::queue<OTCell*> q;

  q.push(m_tree->root());
  while (!q.empty())
  {
    // Grab Cell and Bounds
    OTCell *cell = q.front();


    // if children, queue them instead
    if (cell->children[0]) {
      for (int i = 0; i < 8; i++)
        q.push(cell->children[i]);
    }
    // otherwise, save verts
    else {
      BoundingBox bounds = cell->bounds;
      vertexForPosition(bounds.minCorner());
      vertexForPosition(bounds.minCorner() + vec3(bounds.size.x,             0,             0));
      vertexForPosition(bounds.minCorner() + vec3(bounds.size.x,             0, bounds.size.z));
      vertexForPosition(bounds.minCorner() + vec3(            0,             0, bounds.size.z));
      vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y,             0));
      vertexForPosition(bounds.minCorner() + vec3(bounds.size.x, bounds.size.y,             0));
      vertexForPosition(bounds.maxCorner());
      vertexForPosition(bounds.minCorner() + vec3(            0, bounds.size.y, bounds.size.z));
      Vertex *center = vertexForPosition(bounds.center());

      center->dual = true;
    }

    q.pop();
  }
}

//============================================
// - createBackgroundVerts()
//============================================
void OctreeMesherImp::createBackgroundTets()
{
  std::queue<OTCell*> q;

  q.push(m_tree->root());
  while (!q.empty())
  {
    // Grab Cell and Bounds
    OTCell *cell = q.front();

    if (true || cell->celltype != OTCell::Outside) {

      // if there are children, enqueue them
      if (cell->children[0])
      {
        for (int i = 0; i < 8; i++)
          q.push(cell->children[i]);
        q.pop();
        continue;
      }

      BoundingBox bounds = cell->bounds;

      // get original boundary positions
      vec3 original_positions[9];
      original_positions[0] = bounds.minCorner();
      original_positions[1] = bounds.minCorner() + vec3(bounds.size.x,             0,             0);
      original_positions[2] = bounds.minCorner() + vec3(bounds.size.x, bounds.size.y,             0);
      original_positions[3] = bounds.minCorner() + vec3(            0, bounds.size.y,             0);
      original_positions[4] = bounds.minCorner() + vec3(            0,             0, bounds.size.z);
      original_positions[5] = bounds.minCorner() + vec3(bounds.size.x,             0, bounds.size.z);
      original_positions[6] = bounds.maxCorner();
      original_positions[7] = bounds.minCorner() + vec3(            0, bounds.size.y, bounds.size.z);
      original_positions[8] = bounds.center();

      // Determine Ordered Verts
      Vertex* verts[9] = { 0 };
      for (int i = 0; i < 9; i++)
        verts[i] = vertexForPosition(original_positions[i]);


      // Collect face neighbors
      OTCell* fn[6] = { 0 };
      for (int f = 0; f < 6; f++)
        fn[f] = m_tree->getNeighborAtLevel(cell, f, cell->level);

      Vertex* c1 = verts[8];

      vec3 original_c1 = original_positions[8];

      // create tets for each face
      for (int f = 0; f < 6; f++)
      {
        // no neighbor? We're on boundary
        if (!fn[f])
        {
          // grab vertex in middle of face on boundary
          Vertex *b = vertexForPosition(0.25*(verts[FACE_VERTICES[f][0]]->pos() +
            verts[FACE_VERTICES[f][1]]->pos() +
            verts[FACE_VERTICES[f][2]]->pos() +
            verts[FACE_VERTICES[f][3]]->pos()));

          bool split = false;

          // look at 4 lattice tets, does edge spanning boundary have a middle vertex?
          for (int e = 0; e < 4; e++)
          {
            Vertex *v1 = verts[FACE_VERTICES[f][(e + 0) % 4]];
            Vertex *v2 = verts[FACE_VERTICES[f][(e + 1) % 4]];

            vec3 original_v1 = original_positions[FACE_VERTICES[f][(e + 0) % 4]];
            vec3 original_v2 = original_positions[FACE_VERTICES[f][(e + 1) % 4]];

            //Vertex * m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
            Vertex * m = vertexForPosition(0.5*(original_v1 + original_v2), false);

            if (m) {
              split = true;
              break;
            }
          }

          // if there are any splits, output 2 quadrisected BCC tets for each
          // face that needs it and a biseceted BCC tet on the edges without splits
          if (split)
          {
            for (int e = 0; e < 4; e++)
            {
              Vertex *v1 = verts[FACE_VERTICES[f][(e + 0) % 4]];
              Vertex *v2 = verts[FACE_VERTICES[f][(e + 1) % 4]];

              vec3 original_v1 = original_positions[FACE_VERTICES[f][(e + 0) % 4]];
              vec3 original_v2 = original_positions[FACE_VERTICES[f][(e + 1) % 4]];

              //Vertex * m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
              Vertex * m = vertexForPosition(0.5*(original_v1 + original_v2), false);

              // if edge is split
              if (m) {
                // create 2 quadrisected tets (3-->red)
                m_mesh->createTet(c1, v1, m, b, 3);
                m_mesh->createTet(c1, m, v2, b, 3);
              } else
              {
                // create bisected BCC tet  (2-->yellow)
                m_mesh->createTet(c1, v1, v2, b, 2);
              }
            }
          }
          // otherwise, output 2 pyramids
          else {

            Vertex *v1 = verts[FACE_VERTICES[f][0]];
            Vertex *v2 = verts[FACE_VERTICES[f][1]];
            Vertex *v3 = verts[FACE_VERTICES[f][2]];
            Vertex *v4 = verts[FACE_VERTICES[f][3]];

            // output 2 pyramids
            // the exterior shared diagonal must adjoin the corner and the center of pCell's parent.
            if (FACE_DIAGONAL_BIT[f][cell->index()])
            {
              m_mesh->createTet(c1, v1, v2, v3, 5);
              m_mesh->createTet(c1, v3, v4, v1, 5);
            } else
            {
              m_mesh->createTet(c1, v2, v3, v4, 5);
              m_mesh->createTet(c1, v4, v1, v2, 5);
            }
          }
        }

        // same level?
        else if (fn[f]->level == cell->level && !fn[f]->hasChildren())
        {
          // only output if in positive side (to avoid duplicate tet when neighbor cell is examined)
          if (f % 2 == 1) {

            // look at 4 lattice tets, does edge spanning cells have a middle vertex?
            for (int e = 0; e < 4; e++)
            {
              Vertex* v1 = verts[FACE_VERTICES[f][(e + 0) % 4]];
              Vertex* v2 = verts[FACE_VERTICES[f][(e + 1) % 4]];

              vec3 original_v1 = original_positions[FACE_VERTICES[f][(e + 0) % 4]];
              vec3 original_v2 = original_positions[FACE_VERTICES[f][(e + 1) % 4]];

              //Vertex*  m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
              Vertex*  m = vertexForPosition(0.5*(original_v1 + original_v2), false);
              Vertex* c2 = vertexForPosition(fn[f]->bounds.center(), false);

              vec3 original_c2 = fn[f]->bounds.center();
              //Vertex* b = vertexForPosition(0.5f*(original_c1 + original_c2));
              Vertex* b = vertexForPosition(0.25f*original_positions[FACE_VERTICES[f][(e + 0) % 4]] +
                0.25f*original_positions[FACE_VERTICES[f][(e + 1) % 4]] +
                0.25f*original_positions[FACE_VERTICES[f][(e + 2) % 4]] +
                0.25f*original_positions[FACE_VERTICES[f][(e + 3) % 4]]);

              // if yes, output 2 bisected BCC tets
              if (m)
              {
                if (SPLIT_ACROSS_CELLS) {
                  m_mesh->createTet(c1, v1, m, b, 1);
                  m_mesh->createTet(v1, m, b, c2, 1);

                  m_mesh->createTet(c1, m, v2, b, 1);
                  m_mesh->createTet(m, v2, b, c2, 1);
                } else {
                  m_mesh->createTet(c1, v1, m, c2, 1);
                  m_mesh->createTet(c1, m, v2, c2, 1);
                }
              } else {
                // output 1 normal BCC tet
                if (SPLIT_ACROSS_CELLS) {
                  m_mesh->createTet(c1, v1, v2, b, 0);
                  m_mesh->createTet(v1, v2, c2, b, 0);
                } else {
                  m_mesh->createTet(c1, v1, v2, c2, 0);
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
          for (int e = 0; e < 4; e++)
          {
            Vertex* v1 = verts[FACE_VERTICES[f][(e + 0) % 4]];
            Vertex* v2 = verts[FACE_VERTICES[f][(e + 1) % 4]];

            vec3 original_v1 = original_positions[FACE_VERTICES[f][(e + 0) % 4]];
            vec3 original_v2 = original_positions[FACE_VERTICES[f][(e + 1) % 4]];

            //Vertex*  m = vertexForPosition(0.5*(v1->pos() + v2->pos()), false);
            Vertex*  m = vertexForPosition(0.5*(original_v1 + original_v2), false);

            // output 2 quadrisected tets
            // MODIFIED on Sun 17th
            //if(m){
            m_mesh->createTet(c1, v1, m, b, 4);
            m_mesh->createTet(c1, m, v2, b, 4);
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


//============================================================================
// - cleanup()
//============================================================================
void OctreeMesherImp::cleanup()
{
  // free vertex tracker list
  m_vertex_tracker.clear();

  // clean up sizing oracle
  delete m_sizing_oracle;
  m_sizing_oracle = nullptr;
}


//============================================================================
// - vertexForPosition()
//
//  This method takes the given coordinate and looks up a MAP to find background
//  cell vertex that has already been created for this position. IF no such
//  vertex is found, a new one is created, added to the map, and returned.
//  If create is set to false, no vertex is created if one is missing
//============================================================================
Vertex* OctreeMesherImp::vertexForPosition(const vec3 &position, bool create)
{
  vec3 pos = position;
  // if this point has been warped, use the warped position
  {
    std::map<vec3, vec3, vec3order>::iterator res = m_warp_tracker.find(position);
    if (res != m_warp_tracker.end())
    {
      pos = res->second;
    }
  }


  Vertex *vertex = nullptr;

  std::map<vec3, Vertex*, vec3order>::iterator res = m_vertex_tracker.find(pos);

  //std::cout << "checking position: " << position.toString() << std::endl;

  // create new one if necessary
  if (res == m_vertex_tracker.end())
  {
    if (create)
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


//---------------------- public interface ----------------------
OctreeMesher::OctreeMesher(const cleaver::AbstractScalarField *sizing_field) :
    m_pimpl(new OctreeMesherImp(sizing_field))
{
}

OctreeMesher::~OctreeMesher()
{
}

void OctreeMesher::setSizingField(const cleaver::AbstractScalarField *sizing_field)
{
    m_pimpl->m_sizing_field = sizing_field;
    // todo: consider doing some validation here, return error message
}

void OctreeMesher::createMesh()
{
    // create sizing oracle
    m_pimpl->createOracle();

    // create Octree
    m_pimpl->createOctree();

    // balance Octree
    // TODO(jonbronson): This WORKS but looks like we really do need bottom up
    // not top down. A FIX however, is to store only leaves, and prune them as
    // they split.
    m_pimpl->balanceOctree();

    // TODO(jonbronson): Move into pimpl method.
    // initialize an empty mesh
    if (m_pimpl->m_mesh)
      delete m_pimpl->m_mesh;
    m_pimpl->m_mesh = new TetMesh();

    // visit each cell once, ensure
    // vertices are stored
    m_pimpl->createBackgroundVerts();

    // visit each child once, post-order
    // output tets to fill each hex cell
    m_pimpl->createBackgroundTets();

    // cleanup
    m_pimpl->cleanup();

}

cleaver::TetMesh* OctreeMesher::getMesh()
{
    return m_pimpl->m_mesh;
}




} // namespace cleaver
