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

#include <ctype.h>
#include <errno.h>

#include <teem/air.h>
#include <teem/hest.h>

char *info = ("Converts from DOS text files to normal (converting LF-CR pairs "
              "to just CR), or, with the \"-r\" option, convert back to DOS, "
              "for whatever sick and twisted reason you'd have to do that. "
              "Can also handle legacy MAC text files (only LF). "
              "Unlike the simple sed or perl scripts for this purpose, "
              "this program is careful to be idempotent.  Also, this makes "
              "an effort to not meddle with binary files (on which this may "
              "be mistakenly invoked).  A message is printed "
              "to stderr for all the files actually modified. ");

#define CR 10
#define LF 13
#define BAD_PERC 5.0

typedef union {
  char **c;
  void **v;
} _undosU;

void
undosConvert(char *me, char *name, int reverse, int mac,
             int quiet, int noAction) {
  airArray *mop;
  FILE *fin, *fout;
  char *data=NULL;
  airArray *dataArr;
  unsigned int ci;
  int car, numBad, willConvert;
  _undosU uu;

  mop = airMopNew();
  if (!airStrlen(name)) {
    fprintf(stderr, "%s: empty filename\n", me);
    airMopError(mop); return;
  }

  /* -------------------------------------------------------- */
  /* open input file  */
  fin = airFopen(name, stdin, "rb");
  if (!fin) {
    if (!quiet) {
      fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", 
              me, name, strerror(errno));
    }
    airMopError(mop); return;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* create buffer */
  uu.c = &data;
  dataArr = airArrayNew(uu.v, NULL, sizeof(char), AIR_STRLEN_HUGE);
  if (!dataArr) {
    if (!quiet) {
      fprintf(stderr, "%s: internal allocation error #1\n", me);
    }
    airMopError(mop); return;
  }
  airMopAdd(mop, dataArr, (airMopper)airArrayNuke, airMopAlways);

  /* -------------------------------------------------------- */
  /* read input file, testing for binary-ness along the way */
  numBad = 0;
  car = getc(fin);
  if (EOF == car) {
    if (!quiet) {
      fprintf(stderr, "%s: \"%s\" is empty, skipping ...\n", me, name);
    }
    airMopError(mop); return;
  }
  do {
    ci = airArrayLenIncr(dataArr, 1);
    if (!dataArr->data) {
      if (!quiet) {
        fprintf(stderr, "%s: internal allocation error #2\n", me);
      }
      airMopError(mop); return;
    }
    data[ci] = car;
    numBad += !(isprint(data[ci]) || isspace(data[ci]));
    car = getc(fin);
  } while (EOF != car && BAD_PERC > 100.0*numBad/dataArr->len);
  if (EOF != car) {
    if (!quiet) {
      fprintf(stderr, "%s: more than %g%% of \"%s\" is non-printing, "
              "skipping ...\n", me, BAD_PERC, name);
    }
    airMopError(mop); return;    
  }
  fin = airFclose(fin);

  /* -------------------------------------------------------- */
  /* see if we really need to do anything */
  willConvert = AIR_FALSE;
  if (!strcmp("-", name)) {
    willConvert = AIR_TRUE;
  } else if (reverse) {
    for (ci=0; ci<dataArr->len; ci++) {
      if (mac) {
        if (CR == data[ci]) {
          willConvert = AIR_TRUE;
          break;
        }
      } else {
        if (CR == data[ci] && (ci && LF != data[ci-1])) {
          willConvert = AIR_TRUE;
          break;
        }
      }
    }
  } else {
    for (ci=0; ci<dataArr->len; ci++) {
      if (mac) {
        if (LF == data[ci]) {
          willConvert = AIR_TRUE;
          break;
        }
      } else {
        if (LF == data[ci] && (ci+1<dataArr->len && CR == data[ci+1])) {
          willConvert = AIR_TRUE;
          break;
        }
      }
    }
  }
  if (!willConvert) {
    /* no, we don't need to do anything; quietly quit */
    airMopOkay(mop);
    return;
  } else {
    if (!quiet) {
      fprintf(stderr, "%s: %s \"%s\" %s %s ... \n", me, 
              noAction ? "would convert" : "converting",
              name,
              reverse ? "to" : "from",
              mac ? "MAC" : "DOS");
    }
  }
  if (noAction) {
    /* just joking, we won't actually write anything.
       (yes, even if input was stdin) */
    airMopOkay(mop);
    return;
  }

  /* -------------------------------------------------------- */
  /* open output file */
  fout = airFopen(name, stdout, "wb");
  if (!fout) {
    if (!quiet) {
      fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", 
              me, name, strerror(errno));
    }
    airMopError(mop); return;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* write output file */
  car = 'a';
  if (reverse) {
    for (ci=0; ci<dataArr->len; ci++) {
      if ((mac && CR == data[ci])
          || (CR == data[ci] && (ci && LF != data[ci-1]))) {
        car = putc(LF, fout);
        if (!mac && EOF != car) {
          car = putc(CR, fout);
        }
      } else {
        car = putc(data[ci], fout);
      }
    }
  } else {
    for (ci=0; EOF != car && ci<dataArr->len; ci++) {
      if ((mac && LF == data[ci])
          || (LF == data[ci] && (ci+1<dataArr->len && CR == data[ci+1]))) {
        car = putc(CR, fout);
        ci += !mac;
      } else {
        car = putc(data[ci], fout);
      }
    }
  }
  if (EOF == car) {
    if (!quiet) {
      fprintf(stderr, "%s: ERROR writing \"%s\" possible data loss !!! "
              "(sorry)\n", me, name);
    }
  }
  fout = airFclose(fout);

  airMopOkay(mop);
  return;
}

int
main(int argc, char *argv[]) {
  char *me, **name;
  int lenName, ni, reverse, quiet, noAction, mac;
  hestOpt *hopt = NULL;
  airArray *mop;

  me = argv[0];
  hestOptAdd(&hopt, "r", NULL, airTypeInt, 0, 0, &reverse, NULL,
             "convert back to DOS, instead of converting from DOS to normal");
  hestOptAdd(&hopt, "q", NULL, airTypeInt, 0, 0, &quiet, NULL,
             "never print anything to stderr, even for errors.");
  hestOptAdd(&hopt, "m", NULL, airTypeInt, 0, 0, &mac, NULL,
             "deal with legacy MAC text files.");
  hestOptAdd(&hopt, "n", NULL, airTypeInt, 0, 0, &noAction, NULL,
             "don't actually write converted files, just pretend to. "
             "This is useful to see which files WOULD be converted. ");
  hestOptAdd(&hopt, NULL, "file", airTypeString, 1, -1, &name, NULL,
             "all the files to convert.  Each file will be over-written "
             "with its converted contents.  Use \"-\" to read from stdin "
             "and write to stdout", &lenName);
  hestParseOrDie(hopt, argc-1, argv+1, NULL, me, info,
                 AIR_TRUE, AIR_TRUE, AIR_TRUE);
  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  for (ni=0; ni<lenName; ni++) {
    undosConvert(me, name[ni], reverse, mac, quiet, noAction);
  }
  
  airMopOkay(mop);
  return 0;
}
