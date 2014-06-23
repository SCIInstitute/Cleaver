//=========================================================================
//  CPUDirectSolver.h
//
//  This file is the header for the CPU implementation of the Conjugate
//  Gradient solver. It uses the Base Class Solver as foundation, and
//  is meant to be called, externally, from a GUI Window.
//
//  Author: Jonathan Bronson
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#ifndef __CPU_DIRECT_SOLVER__
#define __CPU_DIRECT_SOLVER__

#include "Solver.h"
#include <time.h>

#ifndef NULL
#define NULL 0
#endif

class CPUDirectSolver : public Solver
{
public:
    CPUDirectSolver(int n, float *x, float *b, float tolerance = 1E-4, int maxIterations = 9999);
    ~CPUDirectSolver();

    void initialize();
    void initializeOld();
    void cleanup();
    bool runIteration();
    bool runIterationOld();
    void solve();

    void setTolerance(float tolerance){ eps = tolerance; }
    void setMaxIterations(int iterations){ maxIterations = iterations; }

    int iterationCount(){ return its; }
    double iterationDuration();

private:
    void mult(const float *v, float *w );

    int N;
    int rows;
    int cols;
    float *x;
    float *b;

    float alpha;
    float beta;

    int *debug;
    float *g;
    float *r;
    float *p;

    float err;
    float eps;
    int its;
    int maxIterations;

    float t, tau, sig, rho, gam;

    clock_t begin;
    clock_t end;
};

#endif  // __CPU_DIRECT_SOLVER__
