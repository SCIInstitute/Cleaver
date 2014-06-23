//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Tetrahedral Mesher
// -- Octree for Lattice
//
// Author Jonathan Bronson (bronson@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2011, 2012, Jonathan Bronson
//  Scientific Computing & Imaging Institute
//  University of Utah
//
//  Permission is  hereby  granted, free  of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files  ( the "Software" ),  to  deal in  the  Software without
//  restriction, including  without limitation the rights to  use,
//  copy, modify,  merge, publish, distribute, sublicense,  and/or
//  sell copies of the Software, and to permit persons to whom the
//  Software is  furnished  to do  so,  subject  to  the following
//  conditions:
//
//  The above  copyright notice  and  this permission notice shall
//  be included  in  all copies  or  substantial  portions  of the
//  Software.
//
//  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
//  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
//  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
//  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//  USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------
//-------------------------------------------------------------------


#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <list>
#include "BoundingBox.h"

namespace cleaver
{
    class BoundingBox;
    class Octree;
    class OTCell;

    enum OctantLetters { LLF, RLF, LUF, RUF, LLB, RLB, LUB, RUB };            // X Y Z
    enum OctantNumbers { _000, _001, _010, _011, _100, _101, _110, _111 };    // X Y Z

class OTCell
{
public:

    enum CellType { Unknown, Inside, Outside, Staddles };

    OTCell();
    ~OTCell();

    bool hasChildren();
    void subdivide();
    int index();

    unsigned int xLocCode;  // X Locational Code
    unsigned int yLocCode;  // Y Locational Code
    unsigned int zLocCode;  // Z Locational Code
    unsigned int level;     // Cell level in hierarchy (smallest cell is level 0)
    unsigned int depths[18];
    CellType celltype;
    OTCell* parent;         // Pointer to parent cell
    OTCell* children[8];    // Pointers to child cells
    BoundingBox bounds;
    double minLFS;

};

class Octree
{
public:
    Octree(const BoundingBox &bounds);
    ~Octree();

    OTCell* root() const;
    OTCell* getCell(int xLocCode, int yLocCode, int zLocCode) const;
    OTCell* getNeighbor(const OTCell *cell, int dir) const;
    OTCell* getNeighborAtLevel(const OTCell *cell, int dir, int level) const;
    int getNumberofLevels() const{ return nLevels; }
    int getMaximumValue() const{ return maxVal; }
    int getMaximumCode() const{ return maxCode; }
    void getLeavesUnderCell(OTCell *cell, std::vector<OTCell*> &leaves);
    std::vector<OTCell*> getAllLeaves();
    std::list<OTCell*> collectChildrenAtLevel(OTCell *pCell, unsigned int level) const;
    OTCell* addCellAtLevel(int x, int y, int z, unsigned int level);
    OTCell* getCellAtLevel(int x, int y, int z, unsigned int level);


private:

    unsigned int nLevels;   // Number of Possible Levels in the Octree
    unsigned int rootLevel; // Level of root cell (nLevels - 1)
    unsigned int maxVal;    // For converting to positions
    unsigned int maxCode;   // For Locating Cells
    OTCell *m_root;
    BoundingBox m_bounds;
};

}

#endif // OCTREE_H
