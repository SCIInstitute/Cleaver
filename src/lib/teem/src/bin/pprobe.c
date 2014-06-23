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


#include <stdio.h>

#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>
#include <teem/limn.h>

#define SPACING(spc) (AIR_EXISTS(spc) ? spc: nrrdDefaultSpacing)

/* copied this from ten.h; I don't want gage to depend on ten */
#define PROBE_MAT2LIST(l, m) ( \
   (l)[1] = (m)[0],          \
   (l)[2] = (m)[3],          \
   (l)[3] = (m)[6],          \
   (l)[4] = (m)[4],          \
   (l)[5] = (m)[7],          \
   (l)[6] = (m)[8] )

int
probeParseKind(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "probeParseKind";
  gageKind **kindP;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  kindP = (gageKind **)ptr;
  airToLower(str);
  if (!strcmp(gageKindScl->name, str)) {
    *kindP = gageKindScl;
  } else if (!strcmp(gageKindVec->name, str)) {
    *kindP = gageKindVec;
  } else if (!strcmp(tenGageKind->name, str)) {
    *kindP = tenGageKind;
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, str)) {
    *kindP = tenDwiGageKindNew();
  } else {
    sprintf(err, "%s: not \"%s\", \"%s\", \"%s\", or \"%s\"", me,
            gageKindScl->name, gageKindVec->name,
            tenGageKind->name, TEN_DWI_GAGE_KIND_NAME);
    return 1;
  }

  return 0;
}

void *
probeParseKindDestroy(void *ptr) {
  gageKind *kind;
  
  if (ptr) {
    kind = AIR_CAST(gageKind *, ptr);
    if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
      tenDwiGageKindNix(kind);
    }
  }
  return NULL;
}

hestCB probeKindHestCB = {
  sizeof(gageKind *),
  "kind",
  probeParseKind,
  probeParseKindDestroy
}; 

void
printans(FILE *file, const double *ans, int len) {
  int a;

  AIR_UNUSED(file);
  for (a=0; a<=len-1; a++) {
    if (a) {
      printf(", ");
    }
    printf("%g", ans[a]);
  }
}

char *probeInfo = ("Uses gageProbe() to query scalar or vector volumes "
                   "at a single probe location.");

int
main(int argc, char *argv[]) {
  gageKind *kind;
  char *me, *whatS, *err, *outS, *stackSavePath;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  NrrdKernelSpec *k00, *k11, *k22, *kSS, *kSSblur;
  float pos[3], lineInfo[4];
  double gmc, rangeSS[2], posSS, *scalePos;
  unsigned int ansLen, numSS, ninSSIdx, lineStepNum;
  int what, E=0, renorm, SSrenorm, SSuniform, verbose;
  const double *answer;
  Nrrd *nin, **ninSS=NULL, *nout=NULL;
  gageContext *ctx;
  gagePerVolume *pvl;
  limnPolyData *lpld=NULL;
  airArray *mop;
  int worldSpace;
  Nrrd *ngrad=NULL, *nbmat=NULL;
  double bval, eps;
  unsigned int *skip, skipNum;

  mop = airMopNew();
  me = argv[0];
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hparm->elideSingleOtherType = AIR_TRUE;
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "k", "kind", airTypeOther, 1, 1, &kind, NULL,
             "\"kind\" of volume (\"scalar\", \"vector\", or \"tensor\")",
             NULL, NULL, &probeKindHestCB);
  hestOptAdd(&hopt, "p", "x y z", airTypeFloat, 3, 3, pos, NULL,
             "the position in space at which to probe");
  hestOptAdd(&hopt, "wsp", NULL, airTypeInt, 0, 0, &worldSpace, NULL,
             "if using this option, position (\"-p\") will be in world "
             "space, instead of index space (the default)");
  hestOptAdd(&hopt, "pi", "lpld in", airTypeOther, 1, 1, &lpld, "",
             "input polydata (overrides \"-p\")",
             NULL, NULL, limnHestPolyDataLMPD);
  hestOptAdd(&hopt, "pl", "x y z s", airTypeFloat, 4, 4, lineInfo, "0 0 0 0",
             "probe along line, instead of at point.  "
             "The \"-p\" three coords are the line start point. "
             "If \"s\" is zero, (x,y,z) is the line end point. "
             "If \"s\" is non-zero, (x,y,z) is the line direction, "
             "which is scaled to have length \"s\", "
             "and then used as the step between line samples. ");
  hestOptAdd(&hopt, "pln", "num", airTypeUInt, 1, 1, &lineStepNum, "0",
             "if non-zero, number of steps of probing to do along line, "
             "which overrides \"-p\" and \"-pi\"");
  hestOptAdd(&hopt, "v", "verbosity", airTypeInt, 1, 1, &verbose, "1", 
             "verbosity level");
  hestOptAdd(&hopt, "q", "query", airTypeString, 1, 1, &whatS, NULL,
             "the quantity (scalar, vector, or matrix) to learn by probing");
  hestOptAdd(&hopt, "eps", "epsilon", airTypeDouble, 1, 1, &eps, "0",
             "if non-zero, and if query is a scalar, epsilon around probe "
             "location where we will do discrete differences to find the "
             "gradient and hessian (for debugging)");
  hestOptAdd(&hopt, "k00", "kern00", airTypeOther, 1, 1, &k00,
             "tent", "kernel for gageKernel00",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kern11", airTypeOther, 1, 1, &k11,
             "cubicd:1,0", "kernel for gageKernel11",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kern22", airTypeOther, 1, 1, &k22,
             "cubicdd:1,0", "kernel for gageKernel22",
             NULL, NULL, nrrdHestKernelSpec);

  hestOptAdd(&hopt, "ssn", "SS #", airTypeUInt, 1, 1, &numSS,
             "0", "how many scale-space samples to evaluate, or, "
             "0 to turn-off all scale-space behavior");
  hestOptAdd(&hopt, "ssr", "scale range", airTypeDouble, 2, 2, rangeSS,
             "nan nan", "range of scales in scale-space");
  hestOptAdd(&hopt, "sss", "scale save path", airTypeString, 1, 1,
             &stackSavePath, "",
             "give a non-empty path string (like \"./\") to save out "
             "the pre-blurred volumes computed for the stack");
  hestOptAdd(&hopt, "ssp", "SS pos", airTypeDouble, 1, 1, &posSS, "0",
             "position at which to sample in scale-space");
  hestOptAdd(&hopt, "kssblur", "kernel", airTypeOther, 1, 1, &kSSblur,
             "dgauss:1,5", "blurring kernel, to sample scale space",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "kss", "kernel", airTypeOther, 1, 1, &kSS,
             "tent", "kernel for reconstructing from scale space samples",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "ssrn", "ssrn", airTypeInt, 1, 1, &SSrenorm, "0",
             "enable derivative normalization based on scale space");
  hestOptAdd(&hopt, "ssu", NULL, airTypeInt, 0, 0, &SSuniform, NULL,
             "do uniform samples along sigma, and not (by default) "
             "samples according to the logarithm of diffusion time");

  hestOptAdd(&hopt, "rn", NULL, airTypeInt, 0, 0, &renorm, NULL,
             "renormalize kernel weights at each new sample location. "
             "\"Accurate\" kernels don't need this; doing it always "
             "makes things go slower");
  hestOptAdd(&hopt, "gmc", "min gradmag", airTypeDouble, 1, 1, &gmc,
             "0.0", "For curvature-based queries, use zero when gradient "
             "magnitude is below this");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output array, when probing on polydata vertices");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, probeInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  what = airEnumVal(kind->enm, whatS);
  if (!what) {
    /* 0 indeed always means "unknown" for any gageKind */
    fprintf(stderr, "%s: couldn't parse \"%s\" as measure of \"%s\" volume\n",
            me, whatS, kind->name);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    airMopError(mop);
    return 1;
  }
  
  if (ELL_4V_LEN(lineInfo) && !lineStepNum) {
    fprintf(stderr, "%s: gave line info (\"-pl\") but not "
            "# samples (\"-pln\")", me);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    airMopError(mop);
    return 1;
  }

  /* special set-up required for DWI kind */
  if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
    if (tenDWMRIKeyValueParse(&ngrad, &nbmat, &bval, &skip, &skipNum, nin)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble parsing DWI info:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
    if (skipNum) {
      fprintf(stderr, "%s: sorry, can't do DWI skipping in tenDwiGage", me);
      airMopError(mop); return 1;
    }
    /* this could stand to use some more command-line arguments */
    if (tenDwiGageKindSet(kind, 50, 1, bval, 0.001, ngrad, nbmat,
                          tenEstimate1MethodLLS,
                          tenEstimate2MethodQSegLLS,
                          /* randSeed */ 7919)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble parsing DWI info:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
  }

  ansLen = kind->table[what].answerLength;

  /* for setting up pre-blurred scale-space samples */
  if (numSS) {
    unsigned int vi;
    ninSS = AIR_CAST(Nrrd **, calloc(numSS, sizeof(Nrrd *)));
    scalePos = AIR_CAST(double *, calloc(numSS, sizeof(double)));
    if (!(ninSS && scalePos)) {
      fprintf(stderr, "%s: couldn't allocate ninSS", me);
      airMopError(mop); return 1;
    }
    for (ninSSIdx=0; ninSSIdx<numSS; ninSSIdx++) {
      ninSS[ninSSIdx] = nrrdNew();
      airMopAdd(mop, ninSS[ninSSIdx], (airMopper)nrrdNuke, airMopAlways);
    }
    if (SSuniform) {
      for (vi=0; vi<numSS; vi++) {
        scalePos[vi] = AIR_AFFINE(0, vi, numSS-1, rangeSS[0], rangeSS[1]);
      }
    } else {
      double rangeTau[2], tau;
      rangeTau[0] = gageTauOfSig(rangeSS[0]);
      rangeTau[1] = gageTauOfSig(rangeSS[1]);
      for (vi=0; vi<numSS; vi++) {
        tau = AIR_AFFINE(0, vi, numSS-1, rangeTau[0], rangeTau[1]);
        scalePos[vi] = gageSigOfTau(tau);
      }
    }
    if (verbose > 2) {
      fprintf(stderr, "%s: sampling scale range %g--%g %suniformly:\n", me,
              rangeSS[0], rangeSS[1], SSuniform ? "" : "non-");
      for (vi=0; vi<numSS; vi++) {
        fprintf(stderr, "    scalePos[%u] = %g\n", vi, scalePos[vi]);
      }
    }
    if (gageStackBlur(ninSS, scalePos, numSS,
                      nin, kind->baseDim, kSSblur, 
                      nrrdBoundaryBleed, AIR_TRUE,
                      verbose)) {
      airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble pre-computing blurrings:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
    if (airStrlen(stackSavePath)) {
      char fnform[AIR_STRLEN_LARGE];
      sprintf(fnform, "%s/blur-%%02u.nrrd", stackSavePath);
      fprintf(stderr, "%s: |%s|\n", me, fnform);
      if (nrrdSaveMulti(fnform, AIR_CAST(const Nrrd *const *, ninSS),
                        numSS, 0, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble saving blurrings:\n%s\n", me, err);
        airMopError(mop); return 1;
      }
    }
  } else {
    ninSS = NULL;
    scalePos = NULL;
  }
  
  ctx = gageContextNew();
  airMopAdd(mop, ctx, AIR_CAST(airMopper, gageContextNix), airMopAlways);
  gageParmSet(ctx, gageParmGradMagCurvMin, gmc);
  gageParmSet(ctx, gageParmVerbose, verbose);
  gageParmSet(ctx, gageParmRenormalize, renorm ? AIR_TRUE : AIR_FALSE);
  gageParmSet(ctx, gageParmCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(ctx, nin, kind));
  if (!E) E |= gageKernelSet(ctx, gageKernel00, k00->kernel, k00->parm);
  if (!E) E |= gageKernelSet(ctx, gageKernel11, k11->kernel, k11->parm); 
  if (!E) E |= gageKernelSet(ctx, gageKernel22, k22->kernel, k22->parm);
  if (numSS) {
    gagePerVolume **pvlSS;
    gageParmSet(ctx, gageParmStackUse, AIR_TRUE);
    gageParmSet(ctx, gageParmStackRenormalize,
                SSrenorm ? AIR_TRUE : AIR_FALSE);
    if (!E) E |= gageStackPerVolumeNew(ctx, &pvlSS,
                                       AIR_CAST(const Nrrd**, ninSS),
                                       numSS, kind);
    if (!E) airMopAdd(mop, pvlSS, (airMopper)airFree, airMopAlways);
    if (!E) E |= gageStackPerVolumeAttach(ctx, pvl, pvlSS, scalePos, numSS);
    if (!E) E |= gageKernelSet(ctx, gageKernelStack, kSS->kernel, kSS->parm);
  } else {
    if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  }
  if (!E) E |= gageQueryItemOn(ctx, pvl, what);
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  /* test with original context */
  answer = gageAnswerPointer(ctx, pvl, what);
  if (lpld) {
    /* probing on locations of polydata */
    double *dout, xyzw[4];
    unsigned int vidx, ai;
    nout = nrrdNew();
    airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
    if (1 == ansLen) {
      E = nrrdAlloc_va(nout, nrrdTypeDouble, 1, 
                       AIR_CAST(size_t, lpld->xyzwNum));
    } else {
      E = nrrdAlloc_va(nout, nrrdTypeDouble, 2, 
                       AIR_CAST(size_t, ansLen),
                       AIR_CAST(size_t, lpld->xyzwNum));
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    dout = AIR_CAST(double *, nout->data);
    for (vidx=0; vidx<lpld->xyzwNum; vidx++) {
      ELL_4V_COPY(xyzw, lpld->xyzw + 4*vidx);
      ELL_4V_HOMOG(xyzw, xyzw);
      if (gageProbeSpace(ctx, xyzw[0], xyzw[1], xyzw[2],
                         !worldSpace, AIR_TRUE)) {
        fprintf(stderr, "%s: trouble:\n%s\n(%d)\n",
                me, ctx->errStr, ctx->errNum);
        airMopError(mop);
        return 1;
      }
      for (ai=0; ai<ansLen; ai++) {
        dout[ai + ansLen*vidx] = answer[ai];
      }
    }
    if (nrrdSave(outS, nout, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  } else if (lineStepNum) {
    /* we're probing along a line */
    double *dout;
    double start[3], dir[3], end[3], lpos[3];
    unsigned int vidx, ai;
    nout = nrrdNew();
    airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
    if (1 == ansLen) {
      E = nrrdAlloc_va(nout, nrrdTypeDouble, 1, 
                       AIR_CAST(size_t, lineStepNum));
    } else {
      E = nrrdAlloc_va(nout, nrrdTypeDouble, 2, 
                       AIR_CAST(size_t, ansLen),
                       AIR_CAST(size_t, lineStepNum));
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    dout = AIR_CAST(double *, nout->data);
    ELL_3V_COPY(start, pos);
    if (lineInfo[3]) {
      double tmp;
      /* stepping along vector */
      ELL_3V_COPY(dir, 0 + lineInfo);
      ELL_3V_SET(end, AIR_NAN, AIR_NAN, AIR_NAN);
      ELL_3V_NORM(dir, dir, tmp);
      if (!tmp) {
        fprintf(stderr, "%s: requested vector stepping, but vlen = 0", me);
        airMopError(mop);
        return 1;
      }
      ELL_3V_SCALE(dir, lineInfo[3], dir);
    } else {
      /* stepping between points */
      ELL_3V_SET(dir, AIR_NAN, AIR_NAN, AIR_NAN);
      ELL_3V_COPY(end, 0 + lineInfo);
    }
    for (vidx=0; vidx<lineStepNum; vidx++) {
      if (lineInfo[3]) {
        ELL_3V_SCALE_ADD2(lpos, 1, start, vidx, dir);
      } else {
        ELL_3V_AFFINE(lpos, 0, vidx, lineStepNum-1, start, end);
      }
      if (gageProbeSpace(ctx, lpos[0], lpos[1], lpos[2],
                         !worldSpace, AIR_FALSE)) {
        fprintf(stderr, "%s: trouble:\n%s\n(%d)\n",
                me, ctx->errStr, ctx->errNum);
        airMopError(mop);
        return 1;
      }
      for (ai=0; ai<ansLen; ai++) {
        dout[ai + ansLen*vidx] = answer[ai];
      }
    }
    if (nrrdSave(outS, nout, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    /* simple probing at a point */
    E = (numSS
         ? gageStackProbeSpace(ctx, pos[0], pos[1], pos[2], posSS,
                               !worldSpace, AIR_FALSE)
         : gageProbeSpace(ctx, pos[0], pos[1], pos[2],
                          !worldSpace, AIR_FALSE));
    if (E) {
      fprintf(stderr, "%s: trouble:\n%s\n(%d)\n",
              me, ctx->errStr, ctx->errNum);
      airMopError(mop);
      return 1;
    }
    printf("%s: %s(%g,%g,%g) = ", me,
           airEnumStr(kind->enm, what), pos[0], pos[1], pos[2]);
    printans(stdout, answer, ansLen);
    printf("\n");
    if (eps && 1 == ansLen) {
      double v[3][3][3], fes, ee;
      int xo, yo, zo;
      if (!worldSpace) {
        fprintf(stderr, "\n%s: WARNING!!: not probing in world-space (via "
                "\"-wsp\") likely leads to errors in estimated "
                "derivatives\n\n", me);
      }
      gageParmSet(ctx, gageParmVerbose, 0);
#define PROBE(x, y, z)                                                     \
      ((numSS                                                              \
        ? gageStackProbeSpace(ctx, x, y, z, posSS, !worldSpace, AIR_FALSE) \
        : gageProbeSpace(ctx, x, y, z, !worldSpace, AIR_FALSE)), answer[0])
      for (xo=0; xo<=2; xo++) {
        for (yo=0; yo<=2; yo++) {
          for (zo=0; zo<=2; zo++) {
            v[xo][yo][zo] = PROBE(pos[0] + (xo-1)*eps,
                                  pos[1] + (yo-1)*eps,
                                  pos[2] + (zo-1)*eps);
          }
        }
      }
      printf("%s: approx gradient(%s) at (%g,%g,%g) = %f %f %f\n", me,
             airEnumStr(kind->enm, what), pos[0], pos[1], pos[2],
             (v[2][1][1] - v[0][1][1])/(2*eps),
             (v[1][2][1] - v[1][0][1])/(2*eps),
             (v[1][1][2] - v[1][1][0])/(2*eps));
      fes = 4*eps*eps;
      ee = eps*eps;
      printf("%s: approx hessian(%s) at (%g,%g,%g) = \n"
             "%f %f %f\n"
             "   %f %f\n"
             "      %f\n", me,
             airEnumStr(kind->enm, what), pos[0], pos[1], pos[2],
             (v[0][1][1] - 2*v[1][1][1] + v[2][1][1])/ee,
             (v[2][2][1] - v[0][2][1] - v[2][0][1] + v[0][0][1])/fes,
             (v[2][1][2] - v[0][1][2] - v[2][1][0] + v[0][1][0])/fes,
             (v[1][2][1] - 2*v[1][1][1] + v[1][0][1])/ee,
             (v[1][2][2] - v[1][0][2] - v[1][2][0] + v[1][0][0])/fes,
             (v[1][1][2] - 2*v[1][1][1] + v[1][1][0])/ee);
    }
  }

  airMopOkay(mop);
  return 0;
}
