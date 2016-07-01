#include "Octree.h"
#include "BoundingBox.h"
#include "Util.h"
#include <cstring>

namespace cleaver
{

#ifndef nullptr
#define nullptr 0
#endif

const unsigned int MAX_LEVELS = 12;
const int DIR_OFFSETS[18][3] = {
    {-1, 0, 0},  // - x
    {+1, 0, 0},  // + x
    { 0,-1, 0},  // - y
    { 0,+1, 0},  // + y
    { 0, 0,-1},  // - z
    { 0, 0,+1},  // + z

    {-1,-1, 0}, // -x,-y   // bottom left    6
    {+1,-1, 0}, // +x,-y   // bottom right   7
    {-1,+1, 0}, // -x,+y   // upper left     8
    {+1,+1, 0}, // +x,+y   // upper right    9

    {-1, 0,-1}, // -x,-z   // back left     10
    {+1, 0,-1}, // +x,-z   // back right    11
    {-1, 0,+1}, // -x,+z   // front left    12
    {+1, 0,+1}, // +x,+z   // front right   13

    { 0,-1,-1}, // -y,-z   // bottom back   14
    { 0,+1,-1}, // +y,-z   // upper back    15
    { 0,-1,+1}, // -y,+z   // bottom front  16
    { 0,+1,+1}  // +y,+z   // upper front   17
};

OTCell::OTCell() : parent(nullptr), celltype(Unknown)
{
    memset(children, 0, 8*sizeof(OTCell*));
}

// Octree Cell Deconstructor
OTCell::~OTCell()
{
    // delete my children
    for(int i=0; i < 8; i++)
    {
        if(children[i])
        {
            delete children[i];
        }
    }
}

bool OTCell::hasChildren()
{
    return (this->children[0] ? true : false);
}

void OTCell::subdivide()
{
    if(level == 0)
        return;

    for(int i=0; i < 8; i++)
    {
        if(!this->children[i])
        {            
            OTCell *child = new OTCell();
            child->level = level - 1;
            child->xLocCode = xLocCode | (((i & _001) >> 0) << child->level);
            child->yLocCode = yLocCode | (((i & _010) >> 1) << child->level);
            child->zLocCode = zLocCode | (((i & _100) >> 2) << child->level);
            child->parent = this;            
            this->children[i] = child;

            // child bounding box is exactly half the size
            child->bounds.size = 0.5f*bounds.size;

            // origin depends on which child
            child->bounds.origin = bounds.origin + vec3(((i & _001) >> 0)*child->bounds.size.x,
                                                        ((i & _010) >> 1)*child->bounds.size.y,
                                                        ((i & _100) >> 2)*child->bounds.size.z);
        }
    }
}

int OTCell::index()
{
    if(this->parent == nullptr)
        return 0;


    unsigned int branchBit = 1 << level;
    unsigned int childIndex =  (((xLocCode & branchBit) >> level) << 0)
                             + (((yLocCode & branchBit) >> level) << 1)
                             + (((zLocCode & branchBit) >> level) << 2);
    return childIndex;
}

Octree::Octree(const BoundingBox &bounds) : m_bounds(bounds)
{
    double max_size = std::max(std::max(m_bounds.size.x, m_bounds.size.y), m_bounds.size.z);
    m_bounds.size.x = m_bounds.size.y = m_bounds.size.z = max_size;

    this->nLevels = MAX_LEVELS + 1;
    this->rootLevel = this->nLevels - 1;
    this->maxVal = static_cast<unsigned int>(pow2(rootLevel));  // todo: replace with safer int version
    this->maxCode = maxVal - 1;

    m_root = new OTCell();
    m_root->level = rootLevel;
    m_root->xLocCode = 0;
    m_root->yLocCode = 0;
    m_root->zLocCode = 0;
    m_root->bounds = m_bounds;
}

Octree::~Octree()
{
    if(m_root)
        delete m_root;
}

OTCell* Octree::root() const
{
    return m_root;
}

OTCell* Octree::getCell(int xLocCode, int yLocCode, int zLocCode) const
{
    // if outside the tree, return nullptr
    if(xLocCode < 0 || yLocCode < 0 || zLocCode < 0)
        return nullptr;
    if(xLocCode > (int)maxCode || (int)yLocCode > (int)maxCode || zLocCode > (int)maxCode)
        return nullptr;

    // branch to appropriate cell
    OTCell *pCell = this->root();
    unsigned int nextLevel = this->rootLevel - 1;

    while(pCell && pCell->level > 0){
        unsigned int childBranchBit = 1 << nextLevel;
        unsigned int childIndex =  (((xLocCode & childBranchBit) >> nextLevel) << 0)
                                 + (((yLocCode & childBranchBit) >> nextLevel) << 1)
                                 + (((zLocCode & childBranchBit) >> nextLevel) << 2);
        --nextLevel;
        pCell = (pCell->children[childIndex]);
    }

    // return desired cell (or nullptr)
    return pCell;
}

OTCell* Octree::getNeighbor(const OTCell *cell, int dir) const
{
    int shift = 1 << cell->level;

    int xLocCode = cell->xLocCode + DIR_OFFSETS[dir][0]*shift;
    int yLocCode = cell->yLocCode + DIR_OFFSETS[dir][1]*shift;
    int zLocCode = cell->zLocCode + DIR_OFFSETS[dir][2]*shift;

    return getCell(xLocCode,yLocCode,zLocCode);
}

OTCell* Octree::getNeighborAtLevel(const OTCell *cell, int dir, int level) const
{
    unsigned int shift = 1 << cell->level;

    int xLocCode = cell->xLocCode + DIR_OFFSETS[dir][0]*shift;
    int yLocCode = cell->yLocCode + DIR_OFFSETS[dir][1]*shift;
    int zLocCode = cell->zLocCode + DIR_OFFSETS[dir][2]*shift;

    if(xLocCode < 0 || yLocCode < 0 || zLocCode < 0)
        return nullptr;
    else if(xLocCode >= (int)maxCode || yLocCode >= (int)maxCode || zLocCode >= (int)maxCode)
        return nullptr;

    // branch to appropriate cell
    OTCell *pCell = this->root();
    unsigned int nextLevel = this->rootLevel - 1;

    while(pCell && (int)pCell->level > level){
        unsigned int childBranchBit = 1 << nextLevel;
        unsigned int childIndex = ((xLocCode  & childBranchBit) >> (nextLevel))
                                + (((yLocCode  & childBranchBit) >> (nextLevel)) << 1)
                                + (((zLocCode & childBranchBit) >> nextLevel) << 2);
        --nextLevel;
        pCell = (pCell->children[childIndex]);
    }

    // return desired cell (or nullptr)
    return pCell;
}

void Octree::getLeavesUnderCell(OTCell *cell, std::vector<OTCell*> &leaves)
{
    // non-leaf nodes
    if(cell->hasChildren())
    {
        for(int i=0; i < 8; i++)
            getLeavesUnderCell(cell->children[i], leaves);

        return;
    }

    leaves.push_back(cell);
}

std::vector<OTCell*> Octree::getAllLeaves()
{
    std::vector<OTCell*> leaves;

    getLeavesUnderCell(m_root, leaves);

    return leaves;
}

std::list<OTCell*> Octree::collectChildrenAtLevel(OTCell* pCell, unsigned int level) const{
    std::list<OTCell*> kids;
    if (pCell != 0) {
        if (pCell->level > level) {
            for (int i=0; i<8; i++) {
                std::list<OTCell*> sub_kids = collectChildrenAtLevel(pCell->children[i], level);
                kids.insert(kids.end(), sub_kids.begin(), sub_kids.end());
            }
        } else if (pCell->level == level) {
            kids.insert(kids.end(), pCell);
        }
    }

    return kids;

}

//-------------------------------------------------------
// This function will create all branches for the
// tree down to a specific level the x,y,z location is
//-------------------------------------------------------
OTCell* Octree::addCellAtLevel(int x, int y, int z, unsigned int level)
{
    unsigned int xLocCode = (unsigned int)x; // (x * this->maxVal);
    unsigned int yLocCode = (unsigned int)y; // (y * this->maxVal);
    unsigned int zLocCode = (unsigned int)z; // (z * this->maxVal);

    // figure out where this cell should go
    OTCell *pCell = this->root();
    unsigned int nextLevel = this->rootLevel - 1;
    unsigned int n = nextLevel + 1;
    unsigned int childBranchBit;
    unsigned int childIndex;

    while(n-- && pCell->level > level){
         childBranchBit = 1 << nextLevel;
         childIndex = ((xLocCode & childBranchBit) >> (nextLevel))
                    + (((yLocCode & childBranchBit) >> (nextLevel)) << 1)
                    + (((zLocCode & childBranchBit) >> (nextLevel)) << 2);
         --nextLevel;

        if(!(pCell->children[childIndex])){
            OTCell *newCell = new OTCell();
            newCell->level = pCell->level - 1;
            newCell->xLocCode = pCell->xLocCode | (childBranchBit & xLocCode);
            newCell->yLocCode = pCell->yLocCode | (childBranchBit & yLocCode);
            newCell->zLocCode = pCell->zLocCode | (childBranchBit & zLocCode);
            newCell->parent = pCell;
            pCell->children[childIndex] = newCell;
        }

        pCell = pCell->children[childIndex];
    }

    // return newly created leaf-cell, or existing one
    return pCell;
}

OTCell* Octree::getCellAtLevel(int x, int y, int z, unsigned int level)
{
    unsigned int xLocCode = (unsigned int)x; // (x * this->maxVal);
    unsigned int yLocCode = (unsigned int)y; // (y * this->maxVal);
    unsigned int zLocCode = (unsigned int)z; // (z * this->maxVal);

    // figure out where this cell is
    OTCell *pCell = this->root();
    unsigned int nextLevel = this->rootLevel - 1;
    unsigned int n = nextLevel + 1;
    unsigned int childBranchBit;
    unsigned int childIndex;

    while(n-- && pCell->level > level){
         childBranchBit = 1 << nextLevel;
         childIndex = ((xLocCode & childBranchBit) >> (nextLevel))
                    + (((yLocCode & childBranchBit) >> (nextLevel)) << 1)
                    + (((zLocCode & childBranchBit) >> (nextLevel)) << 2);
         --nextLevel;

        pCell = pCell->children[childIndex];
    }

    // return the cell
    return pCell;
}

}
