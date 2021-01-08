#
# Teem: Tools to process and visualize scientific data and images              
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# (LGPL) as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also
# include exceptions to the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

#
# Checks whether the 22nd bit of a 32-bit quiet-NaN is 1 (1) or 0 (0).  This 
# distinction is needed in handling of IEEE floating point special values.  
# This quantity is independent of endian-ness.
#
# VARIABLE - variable to store the result to
#

macro(TEST_QNANHIBIT VARIABLE LOCAL_TEST_DIR)
  if("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
    try_run(${VARIABLE} HAVE_${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${LOCAL_TEST_DIR}/TestQnanhibit.c
      OUTPUT_VARIABLE OUTPUT)
    message(STATUS "Check the value of the 22nd bit of a 32-bit quiet-NaN")
    if(HAVE_${VARIABLE})
      if(${VARIABLE} LESS 0)
        message(ERROR " A test (qnanhibit.c) necessary for NrrdIO configuration returned error code. NrrdIO may not properly handle NaN's.")
      endif()
      if(${VARIABLE})
        file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
                    "Value of the 22nd bit of a 32-bit quiet-NaN is 1")
        message(STATUS "Check the value of the 22nd bit of a 32-bit quiet-NaN - 1")
      else()
        file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
                    "Value of the 22nd bit of a 32-bit quiet-NaN is 0")
        message(STATUS "Check the value of the 22nd bit of a 32-bit quiet-NaN - 0")
      endif()
    else()
      file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
        "\tFailed to compile a test (TestQnanhibit.c) necessary to configure for proper handling of IEEE floating point NaN's.\n")
      message(STATUS "Failed to compile a test (TestQnanhibit.c) necessary to configure for proper handling of IEEE floating point NaN's")
    endif()
    file(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log "TestQnanhibit.c produced following output:\n${OUTPUT}\n\n")
  endif()
endmacro()
