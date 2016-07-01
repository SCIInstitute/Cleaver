#include "SizingFieldOracle.h"
#include <cstdlib>
#include "ScalarField.h"

namespace cleaver
{

SizingFieldOracle::SizingFieldOracle(const AbstractScalarField *sizingField, const BoundingBox &bounds) :
    m_sizingField(sizingField), m_bounds(bounds)
{
    m_constructionType = Fast;

    if(sizingField)
        createOctree();

    //sanityTest1();
    //sanityTest2();
}

void recurseCheck1(OTCell *cell, Octree *tree)
{
    if(!cell->hasChildren())
        return;

    OTCell *pCell = tree->getCellAtLevel(cell->xLocCode, cell->yLocCode, cell->zLocCode, cell->level);

    if(pCell != cell)
    {
        std::cout << "bad cell query in sizing field oracle sanity check 1" << std::endl;
        exit(1);
    }

    double minLFS = pCell->minLFS;

    int equal_count = 0;
    int smaller_count = 0;
    int larger_count = 0;

    for(int i=0; i < 8; i++)
    {
        double childLFS = pCell->children[i]->minLFS;

        if(childLFS == minLFS)
            equal_count++;
        else if(childLFS < minLFS)
            smaller_count++;
        else if(childLFS > minLFS)
            larger_count++;
    }

    if(equal_count < 1){
        std::cout << "PROBLEM! A Cell's minLFS is not one of it's children" << std::endl;
        exit(1);
    }
    if(smaller_count > 0){
        std::cout << "PROBLEM! A Cell's child has a smaller LFS" << std::endl;
        exit(1);
    }

    // call recursively on children
    for(int i=0; i < 8; i++)
    {
        recurseCheck1(pCell->children[i], tree);
    }


    // if successful
}

//====================================
// - Sanity Test1()
//

// This method checks all cell queries and verifies that they are consistent.
// Each cell's minLFS should be contained as one of it's children.
// It should only be used for debugging purposes.
//====================================
void SizingFieldOracle::sanityTest1()
{
    recurseCheck1(m_tree->root(), m_tree);

    std::cout  << "Sanity Check for Octree Consistency Passed!" << std::endl;
}


void recurseCheck2(OTCell *cell, Octree *tree, const cleaver::AbstractScalarField *field)
{
    // query cell using tree call
    OTCell *pCell = tree->getCellAtLevel(cell->xLocCode, cell->yLocCode, cell->zLocCode, cell->level);

    BoundingBox &cellBounds = pCell->bounds;

    // loop over the sizing field region that's within the bounds of this cell
    int min_x = (int)cellBounds.minCorner().x;   int max_x = (int)cellBounds.maxCorner().x;
    int min_y = (int)cellBounds.minCorner().y;   int max_y = (int)cellBounds.maxCorner().y;
    int min_z = (int)cellBounds.minCorner().z;   int max_z = (int)cellBounds.maxCorner().z;

    double minLFS = pCell->minLFS;
    double minFound = -1;

    int equal_count = 0;
    int smaller_count = 0;
    int larger_count = 0;

    for(int k=min_z; (k + 0.5) < max_z; k++)
    {
        for(int j=min_y; (j + 0.5) < max_y; j++)
        {
            for(int i=min_x; (i + 0.5) < max_x; i++)
            {
                double LFS = field->valueAt(i+0.5, j+0.5, k+0.5);

                if(LFS < minFound || minFound == -1)
                    minFound = LFS;

                if(LFS == minLFS)
                    equal_count++;
                else if(LFS < minLFS)
                    smaller_count++;
                else if(LFS > minLFS)
                    larger_count++;
            }
        }
    }

    if(equal_count < 1){
        std::cout << "PROBLEM! At depth " << pCell->level << ", A Cell's minLFS is not conatined within its bounds" << std::endl;
        std::cout << "MinLFS = " << minLFS << std::endl;
        std::cout << "MinFound = " << minFound << std::endl;
        exit(1);
    }
    if(smaller_count > 0){
        std::cout << "PROBLEM! A Cell bounds a region with a smaller LFS than its minLFS" << std::endl;
        exit(1);
    }


    // call recursively on children
    if(pCell->hasChildren())
    {
        for(int i=0; i < 8; i++)
        {
            recurseCheck2(pCell->children[i], tree, field);
        }
    }
}

//====================================
// - Sanity Test2()
//
// This method checks all cell queries and verifies that they
// are consistent with the sizing field itself. Each cell's
// minLFS should be contained in the sizing field in the
// bounds of the given cell. This method is slow and
// should only be used for debugging purposes.
//====================================
void SizingFieldOracle::sanityTest2()
{

    recurseCheck2(m_tree->root(), m_tree, m_sizingField);

    std::cout  << "Sanity Check for Octree Consistency Passed!" << std::endl;
}


void SizingFieldOracle::setSizingField(const AbstractScalarField *sizingField)
{
    m_sizingField = sizingField;
}

void SizingFieldOracle::setBoundingBox(const BoundingBox &bounds)
{
    m_bounds = bounds;
}

//======================================
// - createOctree()
//======================================
void SizingFieldOracle::createOctree()
{
    if(!m_sizingField)
        return;

    // Create Octree
    m_tree = new Octree(m_bounds);

    // breadth first creation
    adaptCell(m_tree->root());
    //std::cout<<"MinSF = "<<m_tree->root()->minLFS<<std::endl;
    //printTree(m_tree->root(), 4);
}

//============================================
// - adaptCell()
//============================================
double SizingFieldOracle::adaptCell(OTCell *cell)
{
//    if(!cell)
//        return 1e10;    

    BoundingBox domainBounds = m_bounds;

    int max_x = (int)domainBounds.maxCorner().x;
    int max_y = (int)domainBounds.maxCorner().y;
    int max_z = (int)domainBounds.maxCorner().z;

    // if cell is completely outside, done
    if(cell->bounds.minCorner().x >= max_x ||
       cell->bounds.minCorner().y >= max_y ||
       cell->bounds.minCorner().z >= max_z)
    {
        cell->celltype = OTCell::Outside;
        cell->minLFS = 1e10;
        return 1e10;
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

//    vec3 tx = vec3((bounds.center().x / volume->bounds().size.x)*sizingField.bounds().size.x,
//                   (bounds.center().y / volume->bounds().size.y)*sizingField.bounds().size.y,
//                   (bounds.center().z / volume->bounds().size.z)*sizingField.bounds().size.z);

    //if(bounds.size.x>0.5)   // JRB  - Changed to 1, since should not go below voxel level
    //                        // TODO: Really, this check should be at the scale of the resolution
                              // of the sizing field. If the sizing field is computed at 3x, this
                              // value should be 0.333333. If 2x, 0.5, etc.
                              // This is still problematic, since octree scale will never match non power of 2 scales...
                              // We need to discuss this.

    // TODO:   What happens if sizing field is not a FloatField ??
    float voxel_scale = (float) ((cleaver::ScalarField<float>*)m_sizingField)->scale().x;

    //if(bounds.size.x > 0.5)
    if(bounds.size.x > voxel_scale)    
        cell->subdivide();

    double min=1e10;
    if(cell->hasChildren())
    {
        for(int i=0; i < 8; i++)
        {
            cell->children[i]->minLFS = adaptCell(cell->children[i]);
            if(cell->children[i]->minLFS < min)
                min = cell->children[i]->minLFS;
        }
        cell->minLFS = min;
    }
    else
    {
        vec3 corner[8], integral;
        for(int i=0; i<8; i++)
            corner[i] = bounds.origin;

        corner[1][0] += bounds.size[0];
        corner[2][1] += bounds.size[1];
        corner[3][2] += bounds.size[2];
        corner[4][0] += bounds.size[0];
        corner[4][1] += bounds.size[1];
        corner[5][1] += bounds.size[1];
        corner[5][2] += bounds.size[2];
        corner[6][0] += bounds.size[0];
        corner[6][2] += bounds.size[2];
        corner[7] += bounds.size;

        integral = bounds.origin+bounds.size;
        for(int i=0; i<3; i++)
            integral[i] = (int)integral[i];

        //adding 0.5 to the corners
        for(int j=0; j<3; j++)
        {
            integral[j]+=0.5;
            for(int i=0; i<8; i++)
                corner[i][j]+=0.5;
        }

        min = cell->minLFS = m_sizingField->valueAt(corner[0]);
        for(int i=1; i<8; i++)
        {
            double temp = cell->minLFS = m_sizingField->valueAt(corner[i]);
            if(temp<min)
                min=temp;
        }
        if(bounds.contains(integral))
        {
            double temp = cell->minLFS = m_sizingField->valueAt(integral);
            if(temp<min)
                min=temp;
        }

        //min = cell->minLFS = m_sizingField->valueAt(bounds.center()+0.5);
        if(min < bounds.size[0])
        {
            cell->subdivide();
            min=1e10;
            for(int i=0; i < 8; i++)
            {
                cell->children[i]->minLFS = adaptCell(cell->children[i]);
                if(cell->children[i]->minLFS < min)
                    min = cell->children[i]->minLFS;
            }
            cell->minLFS = min;
        }
    }
    //printf("%lf\n", min);
    return min;
}

void SizingFieldOracle::printTree(OTCell *myCell, int n)
{
    unsigned int i;
    //printf("Level:%d\n", myCell->level);
    if(myCell->level < (unsigned int)n)
        return;
    for(i=0; i<12-myCell->level; i++)
        std::cout << "\t";
    std::cout << myCell->minLFS << std::endl;
    if(myCell->hasChildren())
    {
        for(int i=0; i < 8; i++)
            printTree(myCell->children[i], n);
    }

}


double SizingFieldOracle::getMinLFS(int xLocCode, int yLocCode, int zLocCode, int level) const
{
    OTCell *pCell = m_tree->getCellAtLevel(xLocCode, yLocCode, zLocCode, level);
    if(pCell!=nullptr)
        return pCell->minLFS;
    else
        return 1e10;
}


} // namespace Cleaver
