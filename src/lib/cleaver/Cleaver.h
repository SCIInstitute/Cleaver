//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Tetrahedral Mesher
// -- Mesher Class
//
//  Author: Jonathan Bronson (bronson@sci.utah.edu)
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

#ifndef CLEAVER_H
#define CLEAVER_H

#include <vector>
#include "ScalarField.h"
#include "Volume.h"
#include "TetMesh.h"
#include <string>

namespace cleaver
{
    TetMesh* cleaveMeshToVolume(const Volume *volume, TetMesh *bgMesh, bool verbose = false);
    TetMesh* createMeshFromVolume(const Volume *volume, bool verbose = false);
    Volume*  createFloatFieldVolumeFromVolume(Volume *volume);    
    ScalarField<float>* createFloatFieldFromScalarField(AbstractScalarField *scalarField);
    ScalarField<double>* createDoubleFieldFromScalarField(AbstractScalarField *scalarField);
    void stripExteriorTets(TetMesh *mesh, const Volume *volume, bool verbose = false);

    extern const std::string VersionNumber;
    extern const std::string VersionDate;
    extern const std::string Version;

}

#endif // CLEAVER_H
