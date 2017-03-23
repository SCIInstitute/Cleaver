/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <teem/biff.h>
#include <teem/nrrd.h>

int
main(int argc, char *argv[]) {
  char *me, *err;
  int enc, form;

  me = argv[0];
  if (2 == argc) {
    if (!strcmp(argv[1], "--version")) {
      printf("Teem version %s (%s)\n",
             airTeemVersion, airTeemReleaseDate);
      exit(0);
    } else if (!strcmp(argv[1], "--help")) {
      char par1[] = "\n Usage: nrrdSanity\n ";
      char par2[] = "\t\t\t\t"
        "nrrdSanity calls the nrrdSanity() check to verify the correctness "
        "of all the information (set at compile-time) about the architecture, "
        "such as endianness, 32/64-bit, and the size of various types, as "
        "well as running sanity checks on the global default (nrrdDefault*) "
        "and state (nrrdState*) variables. ";
      char par3[] = "\t\t\t\t"
        "As a convenience, nrrdSanity also list the availability of the "
        "different formats and data encodings (for Nrrd files) supported "
        "by this build.\n ";
      _hestPrintStr(stdout, 1, 0, 78, par1, AIR_FALSE);
      _hestPrintStr(stdout, 1, 0, 78, par2, AIR_FALSE);
      _hestPrintStr(stdout, 1, 0, 78, par3, AIR_FALSE);
      exit(0);
    } else {
      fprintf(stderr, "%s: unexpected arguments; "
              "\"%s --help\" for more information\n", me, me);
      exit(1);
    }
  }
  /* else it was run with no arguments */
  if (!nrrdSanity()) {
    printf("%s: nrrd sanity check FAILED:\n%s\n", me, err = biffGet(NRRD));
    free(err);
    return 1;
  }
  else {
    printf("%s: nrrd sanity check passed.\n", me);
    printf("\n");
    printf("%s: encodings supported in this build:\n", me);
    for (enc=nrrdEncodingTypeUnknown+1; enc<nrrdEncodingTypeLast; enc++) {
      printf("%s: %s\n", airEnumStr(nrrdEncodingType, enc),
             nrrdEncodingArray[enc]->available() ? "yes" : "not available");
    }
    printf("%s: formats supported in this build:\n", me);
    for (form=nrrdFormatTypeUnknown+1; form<nrrdFormatTypeLast; form++) {
      printf("%s: %s\n", airEnumStr(nrrdFormatType, form),
             nrrdFormatArray[form]->available() ? "yes" : "not available");
    }
  }

  return 0;
}
