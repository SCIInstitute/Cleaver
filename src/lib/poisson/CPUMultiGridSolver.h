//=========================================================================
//  CPUMultiGridSolver.h
//
//  This file is the header for the CPU implementation of the MultiGrid
//  version of the Conjugate Gradient solver. It uses the Base Class
//  Solver as foundation, and is meant to be called, externally, from a
//  GUI Window.
//
//  Author: Jonathan Bronson
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#ifndef __CPU_MULTIGRID_SOLVER__
#define __CPU_MULTIGRID_SOLVER__

#include "Solver.h"
#include <time.h>

#ifndef NULL
#define NULL 0
#endif

class CPUMultiGridSolver : public Solver
{
public:
    CPUMultiGridSolver(int n, int levels, float *x, float *b, float tolerance = 1E-4, int maxIterations = 9999);
    ~CPUMultiGridSolver();

    void initialize();
    void cleanup();
    void solve();
    bool runIteration();

    bool upPhase(){ return up_phase; }
    bool downPhase(){ return down_phase; }

    void reduce();
    void interpolate();
    void bilinearInterpolate();

    void setTolerance(float tolerance){ eps = tolerance; }
    void setMaxIterations(int iterations){ maxIterations = iterations; }

    float* currentX(){ return x[level]; }
    int currentRows(){ return rows[level]; }
    int currentCols(){ return cols[level]; }
    int currentLevel(){ return level; }
    int totalLevels(){ return numLevels; }
    int iterationCount(){ return its; }
    double iterationDuration();

private:
    void mult(const float *v, float *w );

    int *N;
    int *rows;
    int *cols;
    int level;
    int numLevels;
    float **x;
    float **b;

    float alpha;
    float beta;

    float *g;  // reuse for all levels
    float *r;  // reuse for all levels
    float *p;  // reuse for all levels

    float err;
    float eps;
    int its;
    int maxIterations;

    float t, tau, sig, rho, gam;

    bool up_phase;
    bool down_phase;

    clock_t begin;
    clock_t end;
};

#endif // __CPU_MULTIGRID_SOLVER__
