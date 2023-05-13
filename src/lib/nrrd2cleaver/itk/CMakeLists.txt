#-------------------------------------------------------------------
#
#  Copyright (C) 2015
#  Scientific Computing & Imaging Institute
#  University of Utah
#
#  Permission is  hereby  granted, free  of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files  ( the "Software" ),  to  deal in  the  Software without
#  restriction, including  without limitation the rights to  use,
#  copy, modify,  merge, publish, distribute, sublicense,  and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is  furnished  to do  so,  subject  to  the following
#  conditions:
#
#  The above  copyright notice  and  this permission notice shall
#  be included  in  all copies  or  substantial  portions  of the
#  Software.
#
#  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
#  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
#  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
#  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
#  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
#  USE OR OTHER DEALINGS IN THE SOFTWARE.
#-------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10.2)
project(ITK_NRRD2Cleaver)

#Find ITK
find_package(ITK COMPONENTS ITKCommon ITKIONRRD ITKIOMeta
  ITKThresholding ITKSmoothing ITKDistanceMap ITKIOImageBase REQUIRED)
if (ITK_FOUND)
  include (${ITK_USE_FILE})
endif()
message(STATUS "** ITK_DIR: ${ITK_DIR}")

include_directories(${CLEAVER2_SOURCE_DIR}/lib)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/nrrd2cleaver)

add_library(nrrd2cleaver NRRDTools.cpp)

target_link_libraries(nrrd2cleaver ${ITK_LIBRARIES} cleaver)
