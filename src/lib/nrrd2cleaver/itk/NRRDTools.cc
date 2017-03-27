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

#include <cstdio>
#include <NRRDTools.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkNrrdImageIOFactory.h>
#include <itkMetaImageIOFactory.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkApproximateSignedDistanceMapImageFilter.h>
#include <sstream>

//typedefs needed.
typedef float PixelType;
typedef itk::Image< PixelType, 3 > ImageType;
typedef itk::ImageFileReader< ImageType > ReaderType;
typedef itk::ImageFileWriter< ImageType > WriterType;
typedef itk::ThresholdImageFilter< ImageType >  ThreshType;
typedef itk::MinimumMaximumImageCalculator <ImageType>
ImageCalculatorFilterType;
typedef itk::MultiplyImageFilter<ImageType, ImageType, ImageType>
MultiplyImageFilterType;
typedef itk::SubtractImageFilter <ImageType, ImageType >
SubtractImageFilterType;
typedef itk::DiscreteGaussianImageFilter<
  ImageType, ImageType >  GaussianBlurType;
typedef itk::ApproximateSignedDistanceMapImageFilter
<ImageType, ImageType> DMapType;
std::vector<cleaver::AbstractScalarField*>
NRRDTools::segmentationToIndicatorFunctions(std::string filename, double sigma) {
  // read file using ITK
  if (filename.find(".nrrd") != std::string::npos) {
    itk::NrrdImageIOFactory::RegisterOneFactory();
  } else if (filename.find(".mha") != std::string::npos) {
    itk::MetaImageIOFactory::RegisterOneFactory();
  }
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(filename);
  reader->Update();
  ImageType::Pointer image = reader->GetOutput();
  //determine the number of labels in the segmentations
  ImageCalculatorFilterType::Pointer imageCalculatorFilter
    = ImageCalculatorFilterType::New();
  imageCalculatorFilter->SetImage(reader->GetOutput());
  imageCalculatorFilter->Compute();
  auto maxLabel = static_cast<size_t>(imageCalculatorFilter->GetMaximum());
  auto minLabel = static_cast<size_t>(imageCalculatorFilter->GetMinimum());
  std::vector<cleaver::AbstractScalarField*> fields;
  //extract images from each label for an indicator function
  for (size_t i = minLabel, num = 0; i <= maxLabel; i++, num++) {
    //pull out this label
    ThreshType::Pointer thresh = ThreshType::New();
    thresh->SetInput(image);
    thresh->SetOutsideValue(0);
    thresh->ThresholdOutside(static_cast<double>(i) - 0.001,
      static_cast<double>(i) + 0.001);
    thresh->Update();
    //change the values to be from 0 to 1
    MultiplyImageFilterType::Pointer multiplyImageFilter =
      MultiplyImageFilterType::New();
    multiplyImageFilter->SetInput(thresh->GetOutput());
    multiplyImageFilter->SetConstant(1. / static_cast<double>(i));
    multiplyImageFilter->Update();
    //do some blurring
    GaussianBlurType::Pointer blur = GaussianBlurType::New();
    blur->SetInput(multiplyImageFilter->GetOutput());
    blur->SetVariance(sigma * sigma);
    blur->Update();
    //find the average value between
    ImageCalculatorFilterType::Pointer calc =
      ImageCalculatorFilterType::New();
    calc->SetImage(blur->GetOutput());
    calc->Compute();
    float mx = calc->GetMaximum();
    float mn = calc->GetMinimum();
    auto md = (mx + mn) / 2.f;
    //create a distance map with that minimum value as the levelset
    DMapType::Pointer dm = DMapType::New();
    dm->SetInput(blur->GetOutput());
    dm->SetInsideValue(md + 0.1f);
    dm->SetOutsideValue(md -0.1f);
    dm->Update();
    //MultiplyImageFilterType::Pointer mult =
    //  MultiplyImageFilterType::New();
    //mult->SetInput(blur->GetOutput());
    //mult->SetConstant(-20. / (mx - mn));
    //mult->Update();
    /*SubtractImageFilterType::Pointer subtractFilter
      = SubtractImageFilterType::New();
    subtractFilter->SetInput1(mult->GetOutput());
    subtractFilter->SetConstant2(1.);
    subtractFilter->Update();*/
    //convert the image to a cleaver "abstract field"
    auto img = dm->GetOutput();
    auto region = img->GetLargestPossibleRegion();
    auto numPixel = region.GetNumberOfPixels();
    float *data = new float[numPixel];
    auto x = region.GetSize()[0], y = region.GetSize()[1], z = region.GetSize()[2];
    fields.push_back(new cleaver::FloatField(data, x, y, z));
    auto beg = filename.find_last_of("/") + 1;
    auto name = filename.substr(beg, filename.size() - beg);
    auto fin = name.find_last_of(".");
    name = name.substr(0, fin);
    std::stringstream ss;
    ss << name << i;
    fields[num]->setName(ss.str());
    itk::ImageRegionConstIterator<ImageType> imageIterator(img, region);
    size_t pixel = 0;
    while (!imageIterator.IsAtEnd()) {
      // Get the value of the current pixel
      float val = static_cast<float>(imageIterator.Get());
      ((cleaver::FloatField*)fields[num])->data()[pixel++] = -val;
      ++imageIterator;
    }
    auto spacing = img->GetSpacing();
    ((cleaver::FloatField*)fields[num])->setScale(
      cleaver::vec3(spacing[0], spacing[1], spacing[2]));
    //NRRDTools::saveNRRDFile(fields[num], "a" + std::to_string(num));
  }
  return fields;
}

std::vector<cleaver::AbstractScalarField*>
NRRDTools::loadNRRDFiles(std::vector<std::string> files,
  double sigma) {
  std::vector<cleaver::AbstractScalarField*> fields;
  size_t num = 0;
  for (auto file : files) {
    // read file using ITK
    if (file.find(".nrrd") != std::string::npos) {
      itk::NrrdImageIOFactory::RegisterOneFactory();
    } else if (file.find(".mha") != std::string::npos) {
      itk::MetaImageIOFactory::RegisterOneFactory();
    }
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(file);
    reader->Update();
    //do some blurring
    GaussianBlurType::Pointer blur = GaussianBlurType::New();
    blur->SetInput(reader->GetOutput());
    blur->SetVariance(sigma * sigma);
    blur->Update();
    ImageType::Pointer img = blur->GetOutput();
    //convert the image to a cleaver "abstract field"
    auto region = img->GetLargestPossibleRegion();
    auto numPixel = region.GetNumberOfPixels();
    float *data = new float[numPixel];
    auto x = region.GetSize()[0], y = region.GetSize()[1], z = region.GetSize()[2];
    fields.push_back(new cleaver::FloatField(data, x, y, z));
    auto beg = file.find_last_of("/") + 1;
    auto name = file.substr(beg, file.size() - beg);
    fields[num]->setName(name);
    itk::ImageRegionConstIterator<ImageType> imageIterator(img, region);
    size_t pixel = 0;
    while (!imageIterator.IsAtEnd()) {
      // Get the value of the current pixel
      float val = static_cast<float>(imageIterator.Get());
      ((cleaver::FloatField*)fields[num])->data()[pixel++] = val;
      ++imageIterator;
    }
    ((cleaver::FloatField*)fields[num])->setScale(cleaver::vec3(1., 1., 1.));
    num++;
  }
  return fields;
}

void NRRDTools::saveNRRDFile(const cleaver::FloatField *field, const std::string &name) {
  auto dims = field->bounds().size;
  ImageType::Pointer img = ImageType::New();
  itk::Index<3> start; start.Fill(0);
  ImageType::SizeType size;
  size[0] = static_cast<size_t>(dims[0]);
  size[1] = static_cast<size_t>(dims[1]);
  size[2] = static_cast<size_t>(dims[2]);
  ImageType::RegionType region(start, size);
  img->SetRegions(region);
  img->Allocate();
  img->FillBuffer(0);
  for (size_t i = 0; i < dims[0]; i++) {
    for (size_t j = 0; j < dims[1]; j++) {
      for (size_t k = 0; k < dims[2]; k++) {
        ImageType::IndexType pixelIndex;
        pixelIndex[0] = i;
        pixelIndex[1] = j;
        pixelIndex[2] = k;
        auto data = ((cleaver::FloatField*)field)->data();
        img->SetPixel(pixelIndex, data[i + size[0] * j + size[0] * size[1] * k]);
      }
    }
  }
  WriterType::Pointer write = WriterType::New();
  write->SetFileName(name + ((name.find_first_of(".nrrd") == std::string::npos) ? ".nrrd" : ""));
  write->SetInput(img);
  write->Update();
}
