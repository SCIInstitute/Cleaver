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
# Find the native BZ2 includes and library
#
# BZ2_INCLUDE_DIR - where to find bzlib.h, etc.
# BZ2_LIBRARIES   - List of fully qualified libraries to link against when using bzlib.
# BZ2_FOUND       - Do not attempt to use zlib if "no" or undefined.

find_path(BZ2_INCLUDE_DIR bzlib.h
  /usr/local/include
  /usr/include
)

find_library(BZ2_LIBRARY bz2
  /usr/lib
  /usr/local/lib
)

if(BZ2_INCLUDE_DIR)
  if(BZ2_LIBRARY)
    set( BZ2_LIBRARIES ${BZ2_LIBRARY} )
    set( BZ2_FOUND "YES" )
  endif()
endif()

mark_as_advanced(
  BZ2_LIBRARY
  BZ2_INCLUDE_DIR
  )
