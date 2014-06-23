#  Copyright Olivier Parcollet 2010.
#  Distributed under the Boost Software License, Version 1.0.
#      (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

#
# This module looks for cblas.h
#

# transfor CPATH into something cmake likes...
STRING (REPLACE ":" ";" CPATH "$ENV{CPATH}")

# Should I search for an mkl ?
STRING(REGEX MATCH "mkl" is_mkl ${LAPACK_LIBS})

IF (is_mkl)
 MESSAGE(STATUS "You are using mkl ")
 SET (CBLAS_SEARCH_NAME "mkl_cblas.h")
 add_definitions( -DBOOST_NUMERIC_BINDINGS_BLAS_MKL)

ELSE (is_mkl)
 MESSAGE(STATUS "You are not using mkl")
 SET (CBLAS_SEARCH_NAME "cblas.h")
ENDIF (is_mkl)

SET(TRIAL_PATHS
 /usr/include
 /usr/local/include
 /opt/local/include
 ${CPATH}
 /System/Library/Frameworks/vecLib.framework/Versions/A/Headers/
 ${MKL_INCLUDE_DIR}
 )

#MESSAGE(STATUS "Searching for CBLAS as ${CBLAS_SEARCH_NAME} in ${TRIAL_PATHS} ")

FIND_PATH(CBLAS_INCLUDE_DIR NAMES ${CBLAS_SEARCH_NAME}  PATHS ${TRIAL_PATHS} DOC "Include for cblas")

mark_as_advanced(CBLAS_INCLUDE_DIR)

IF (NOT CBLAS_INCLUDE_DIR)
 MESSAGE(FATAL_ERROR "I can not find the cblas headers (cblas.h or mkl_cblas.h) !")
ELSE (NOT CBLAS_INCLUDE_DIR)
 MESSAGE(STATUS "cblas header found at ${CBLAS_INCLUDE_DIR}")
ENDIF (NOT CBLAS_INCLUDE_DIR)


Find_package(PackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CBLAS DEFAULT_MSG CBLAS_INCLUDE_DIR )
