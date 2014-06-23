#include "OctreeMesher.h"

#include <map>
#include <cmath>

#include "vec3.h"
#include "Octree.h"
#include "SizingFieldOracle.h"

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
}



//------------------ private implementation
class OctreeMesherImp
{
public:
    OctreeMesherImp(const AbstractScalarField *sizing_field = NULL);
    ~OctreeMesherImp();

    void createOracle();
    void createOctree();
    void balanceOctree();

    void adaptCell(OTCell *cell);

    const AbstractScalarField *m_sizing_field;
    const SizingFieldOracle   *m_sizing_oracle;

    cleaver::TetMesh *m_mesh;

    Octree *m_tree;
    std::map<vec3, Vertex*, vec3order> m_vertex_tracker;
};

OctreeMesherImp::OctreeMesherImp(const cleaver::AbstractScalarField *sizing_field) :
    m_mesh(NULL), m_tree(NULL), m_sizing_field(sizing_field), m_sizing_oracle(NULL)
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
    m_tree = new Octree(bounds);

    // breadth first creation
    adaptCell(m_tree->root());
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

    double LFS = m_sizing_oracle->getMinLFS(cell->xLocCode, cell->yLocCode, cell->zLocCode, cell->level);

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
    m_pimpl->balanceOctree();

    // create background verts

    // create background tets

    // cleanup

}

cleaver::TetMesh* OctreeMesher::getMesh()
{
    return m_pimpl->m_mesh;
}

}
