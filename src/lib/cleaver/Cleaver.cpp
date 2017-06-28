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

#include "Cleaver.h"
#include "CleaverMesher.h"
#include "TetMesh.h"
#include "ScalarField.h"
#include "Volume.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace std;

namespace cleaver
{
  const std::string VersionNumber = "2.3";
  const std::string VersionDate = __DATE__;
  const std::string Version = std::string("Cleaver") + " " + VersionNumber + " " + VersionDate;

  TetMesh* cleaveMeshToVolume(const Volume *volume, TetMesh *bgMesh, bool verbose)
  {
    CleaverMesher mesher;
    mesher.setVolume(volume);

    mesher.setBackgroundMesh(bgMesh);
    mesher.buildAdjacency();
    mesher.sampleVolume();
    mesher.computeAlphas();
    mesher.computeInterfaces();
    mesher.generalizeTets();
    mesher.snapsAndWarp();
    mesher.stencilTets();

    return mesher.getTetMesh();
  }

  TetMesh* createMeshFromVolume(const Volume *volume, bool verbose)
  {
    CleaverMesher mesher;
    mesher.setVolume(volume);

    mesher.createTetMesh(verbose);

    return mesher.getTetMesh();
  }

  Volume* createFloatFieldVolumeFromVolume(Volume *base_volume)
  {
    int width = base_volume->width();
    int height = base_volume->height();
    int depth = base_volume->depth();

    std::vector<AbstractScalarField*> fields;

    for (int m = 0; m < base_volume->numberOfMaterials(); m++)
    {
      float *data = new float[width*height*depth];

      for (int d = 0; d < depth; d++) {
        for (int h = 0; h < height; h++) {
          for (int w = 0; w < width; w++) {
            data[d*width*height + h*width + w] = (float)base_volume->valueAt(w + 0.5, h + 0.5, d + 0.5, m);
          }
        }
      }

      ScalarField<float> *floatField = new ScalarField<float>(data, width, height, depth);
      floatField->setName(base_volume->getMaterial(m)->name() + "_asFloat");
      fields.push_back(floatField);
    }

    cleaver::Volume *floatVolume = new Volume(fields);
    floatVolume->setName(base_volume->name());

    return floatVolume;
  }

  ScalarField<float>* createFloatFieldFromScalarField(AbstractScalarField *scalarField)
  {
    int w = (int)(scalarField->bounds().size.x);
    int h = (int)(scalarField->bounds().size.y);
    int d = (int)(scalarField->bounds().size.z);

    int wh = w*h;
    int whd = wh*d;

    float *data = new float[whd];

    for (int k = 0; k < d; k++)
    {
      for (int j = 0; j < h; j++)
      {
        for (int i = 0; i < w; i++)
        {
          data[k*wh + j*w + i] = (float)scalarField->valueAt(i + 0.5, j + 0.5, k + 0.5);
        }
      }
    }

    ScalarField<float> *floatField = new ScalarField<float>(data, w, h, d);
    floatField->setBounds(scalarField->bounds());

    return floatField;
  }

  ScalarField<double>* createDoubleFieldFromScalarField(AbstractScalarField *scalarField)
  {
    int w = (int)(scalarField->bounds().size.x);
    int h = (int)(scalarField->bounds().size.y);
    int d = (int)(scalarField->bounds().size.z);

    int wh = w*h;
    int whd = wh*d;

    double *data = new double[whd];

    for (int k = 0; k < d; k++)
    {
      for (int j = 0; j < h; j++)
      {
        for (int i = 0; i < w; i++)
        {
          data[k*wh + j*w + i] = scalarField->valueAt(i + 0.5, j + 0.5, k + 0.5);
        }
      }
    }

    ScalarField<double> *doubleField = new ScalarField<double>(data, w, h, d);
    doubleField->setBounds(scalarField->bounds());

    return doubleField;
  }

  void stripExteriorTets(TetMesh *mesh, const Volume *volume, bool verbose)
  {
    // exterior material is equal to material count
    mesh->stripMaterial(volume->numberOfMaterials(), verbose);
  }

}
