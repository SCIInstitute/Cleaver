//-------------------------------------------------------------------
//
//  Copyright (C) 2015
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

#include <SegmentationTools.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkNrrdImageIOFactory.h>
#include <itkMetaImageIOFactory.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkApproximateSignedDistanceMapImageFilter.h>

namespace SegmentationTools {
  int getNumMats(std::string file) {
    return 0;
  }

  std::vector<cleaver::AbstractScalarField*> createIndicatorFunctions(std::string filename) {
    if (filename.find(".nrrd") != std::string::npos) {
      itk::NrrdImageIOFactory::RegisterOneFactory();
    } else if (filename.find(".mha") != std::string::npos) {
      itk::MetaImageIOFactory::RegisterOneFactory();
    }
    typedef float PixelType;
    typedef itk::Image< PixelType, 3 > ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    typedef itk::ImageFileWriter< ImageType > WriterType;
    // read file using ITK
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(filename);
    reader->Update();
    ImageType::Pointer image = reader->GetOutput();

    typedef itk::MinimumMaximumImageCalculator <ImageType>
      ImageCalculatorFilterType;

    ImageCalculatorFilterType::Pointer imageCalculatorFilter
      = ImageCalculatorFilterType::New();
    imageCalculatorFilter->SetImage(reader->GetOutput());
    imageCalculatorFilter->Compute();
    auto maxLabel = static_cast<size_t>(imageCalculatorFilter->GetMaximum());
    auto minLabel = static_cast<size_t>(imageCalculatorFilter->GetMinimum());
    typedef itk::ThresholdImageFilter< ImageType >  ThreshType;
    std::vector<cleaver::AbstractScalarField*> fields;
    for (size_t i = minLabel, num = 0; i <= maxLabel; i++, num++) {
      ThreshType::Pointer thresh = ThreshType::New();
      thresh->SetInput(image);
      thresh->SetOutsideValue(0);
      thresh->ThresholdOutside(static_cast<double>(i) - 0.001, 
        static_cast<double>(i) + 0.001);
      thresh->Update();
      itk::DiscreteGaussianImageFilter<ImageType, ImageType>::Pointer blur
        = itk::DiscreteGaussianImageFilter<ImageType, ImageType>::New();
      blur->SetInput(thresh->GetOutput());
      blur->SetVariance(1.);
      blur->Update();
      itk::ApproximateSignedDistanceMapImageFilter<ImageType, ImageType>::Pointer P
        = itk::ApproximateSignedDistanceMapImageFilter<ImageType, ImageType>::New();
      P->SetInput(blur->GetOutput());
      P->SetInsideValue(i);
      P->SetOutsideValue(0);
      P->Update();
      auto img = P->GetOutput();
      auto region = img->GetLargestPossibleRegion();
      auto numPixel = region.GetNumberOfPixels();
      float *data = new float[numPixel];
      auto x = region.GetSize()[0], y = region.GetSize()[1], z = region.GetSize()[2];
      fields.push_back(new cleaver::FloatField(data, x, y, z));
      auto beg = filename.find_last_of("/") + 1;
      auto name = filename.substr(beg, filename.size() - beg);
      auto fin = name.find_last_of(".");
      name = name.substr(0, fin);
      fields[num]->setName(name + std::to_string(i));
      itk::ImageRegionConstIterator<ImageType> imageIterator(img, region);
      size_t pixel = 0;
      while (!imageIterator.IsAtEnd()) {
        // Get the value of the current pixel
        float val = static_cast<float>(imageIterator.Get());
        ((cleaver::FloatField*)fields[num])->data()[pixel++] = val; 
        ++imageIterator;
      }
      ((cleaver::FloatField*)fields[num])->setScale(cleaver::vec3(1.,1.,1.));
    }
    return fields;
  }
}
