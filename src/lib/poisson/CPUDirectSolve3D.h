#ifndef CPUDIRECTSOLVE3D_H
#define CPUDIRECTSOLVE3D_H

#include "Solver.h"
#include <time.h>

class CPUDirectSolve3D : public Solver
{
public:
    CPUDirectSolve3D(int width, int height, int depth, double *x, double *b, double tolerance, int maxIterations);
    ~CPUDirectSolve3D();

    void initialize();
    void cleanup();
    bool runIteration();
    void solve();

    void setTolerance(float tolerance){ eps = tolerance; }
    void setMaxIterations(int iterations){ maxIterations = iterations; }

    int iterationCount(){ return its; }
    double iterationDuration();

    double residual();

private:
    void mult(const double *v, double *w );

    int N;
    int wh;
    int width;
    int height;
    int depth;
    int width_height;
    double *x;
    double *b;

    double alpha;
    double beta;

    int *debug;
    double *g;
    double *r;
    double *p;

    double err;
    double eps;
    int its;
    int maxIterations;

    double t, tau, sig, rho, gam;

    clock_t begin;
    clock_t end;
};

#endif // CPUDIRECTSOLVE3D_H
