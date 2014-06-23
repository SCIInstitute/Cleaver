//=========================================================================
//  CPUDirectSolver.cpp
//
//  This file is the definition for the CPU implementation of the Conjugate
//  Gradient solver. It uses the Base Class Solver as foundation, and
//  is meant to be called, externally, from a GUI Window.
//
//  Author: Jonathan Bronson
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#include "CPUDirectSolver.h"
#include "cblas.h"
#include "math.h"
#include "string.h"
#include "time.h"
#include <iostream>

using namespace std;

CPUDirectSolver::CPUDirectSolver(int n, float *x, float *b, float tolerance, int maxIterations) :
        rows(n), cols(n), x(x), b(b), eps(tolerance), maxIterations(maxIterations), g(NULL),r(NULL),p(NULL)
{
    N = n*n;
    this->initialize();
}

CPUDirectSolver::~CPUDirectSolver()
{
    this->cleanup();
}

void CPUDirectSolver::cleanup()
{
    if(g != NULL){
        delete [] g;
        g = NULL;
    }
    if(r != NULL){
        delete [] r;
        r = NULL;
    }
    if(p != NULL){
        delete [] p;
        p = NULL;
    }
}

void CPUDirectSolver::initialize()
{
    if(g != NULL)
        delete []g;
     g = new float[N];

    if(r != NULL)
        delete []r;
    r = new float[N];

    if(p != NULL)
        delete []p;
    p = new float[N];

    its = 0;
    err = eps*eps*sdot(N,b,1,b,1);

    mult(x,g);
    saxpy(N,-1.,b,1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N,-1.,g,1);      // g = -1*g;    (scale g by -1)
    scopy(N,g,1,r,1);      // r = g        (with strides 1)
}

bool CPUDirectSolver::runIteration()
{
    this->begin = clock();

    this->mult(r,p);
    rho=sdot(N,p,1,p,1);
    sig=sdot(N,r,1,p,1);
    tau=sdot(N,g,1,r,1);
    t=tau/sig;
    saxpy(N,t,r,1,x,1);     // x =  t*r + x
    saxpy(N,-t,p,1,g,1);    // g = -t*p + g;
    gam=(t*t*rho-tau)/tau;
    sscal(N,gam,r,1);       // scale vector by constant
    saxpy(N,1.,g,1,r,1);    // r = 1.0*g + r   (with strides 1)
    ++its;

    bool terminationCondition = sdot(N,g,1,g,1)>err && its < maxIterations;
    this->end = clock();

    if(terminationCondition)
        return false;  // not done
    else
        return true;   // done
}

void CPUDirectSolver::initializeOld()
{
    if(g != NULL)
        delete []g;
     g = new float[N];

    if(r != NULL)
        delete []r;
    r = new float[N];

    if(p != NULL)
        delete []p;
    p = new float[N];

    its = 0;
    err = eps*eps*sdot(N,b,1,b,1);


    //--------------------
    // p = r = b - Ax;
    //--------------------
    mult(x,g);        // g = Ax;
    saxpy(N,-1.,b,1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N,-1.,g,1);      // g = -1*g;            (scale g by -1)
    scopy(N,g,1,r,1);      // r = g                (with strides 1)
    scopy(N,r,1,p,1);      // p = r                (with strides 1)
}

bool CPUDirectSolver::runIterationOld()
{
    // rho = r dot r
    float alpha_top = sdot(N, r, 1, r, 1);

    mult(p,g);   // Ap
    float alpha_bottom = sdot(N, p, 1, g, 1);

    alpha = alpha_top / alpha_bottom;

    saxpy(N,alpha,p,1,x,1);     // x = x + alpha*p;
    saxpy(N,-alpha,g,1,r,1);    // r = r - alpha*Ap;

    its++;
    if(sdot(N,g,1,g,1)<err || its > maxIterations)
        return true;

    float beta_top = sdot(N, r, 1, r, 1);
    float beta_bottom = alpha_top;
    beta = beta_top / beta_bottom;

    sscal(N,beta, p, 1);  // p = Beta*p   \  p = r + beta*p;
    saxpy(N,1.,r,1,p,1);  // p = p + r;   /

    return false;
}

void CPUDirectSolver::solve()
{
    float t, tau, sig, rho, gam;

    while ( sdot(N,g,1,g,1)>err && its < maxIterations) {
      mult(r,p);
      rho=sdot(N,p,1,p,1);
      sig=sdot(N,r,1,p,1);
      tau=sdot(N,g,1,r,1);
      t=tau/sig;
      saxpy(N,t,r,1,x,1);
      saxpy(N,-t,p,1,g,1);
      gam=(t*t*rho-tau)/tau;
      sscal(N,gam,r,1);
      saxpy(N,1.,g,1,r,1);
      ++its;
    }
}

void CPUDirectSolver::mult(const float *v, float *w )
{
    //---------------------------------------//
    //   Multiply Interior Region            //
    //---------------------------------------//
    for(int i=1; i < rows-1; i++){
        for(int j=1; j < cols-1; j++){
            int cell = i*cols + j;
            w[cell] = -v[cell-1] + 4*v[cell] - v[cell+1] - v[cell-cols] - v[cell+cols];
        }
    }

    //---------------------------------------//
    //   Multiply Row/Column Boundaries      //
    //---------------------------------------//
    // top & bottom row boundaries
    int i1 = 0, i2 = rows-1;
    for(int j=1; j < rows-1; j++){
        int cell1 = i1*cols + j;
        int cell2 = i2*cols + j;
        w[cell1] = -v[cell1-1] + 4*v[cell1] - v[cell1+1] -     0.0       - v[cell1+cols];  // bottom row
        w[cell2] = -v[cell2-1] + 4*v[cell2] - v[cell2+1] - v[cell2-cols] -     0      ;  // top row


    }

    // left & right column boundaries
    int j1 = 0, j2 = cols-1;
    for(int i=1; i < cols-1; i++){
        int cell1 = i*cols + j1;
        int cell2 = i*cols + j2;
        w[cell1] = -    0      + 4*v[cell1] - v[cell1+1] - v[cell1+cols] - v[cell1-cols]; // left column
        w[cell2] = -v[cell2-1] + 4*v[cell2] -      0     - v[cell2+cols] - v[cell2-cols]; // right column
    }

    //---------------------------------------//
    //      Multiply Corner Boundaries       //
    //---------------------------------------//
    int LL = 0, LR = cols-1, UL = (rows-1)*cols, UR = (rows-1)*cols + cols-1;
    w[LL] = -   0    + 4*v[LL] - v[LL+1] - v[LL+cols];   // lower left
    w[LR] = -v[LR-1] + 4*v[LR] -    0    - v[LR+cols];   // lower right
    w[UL] = -   0    + 4*v[UL] - v[UL+1] - v[UL-cols];   // upper left
    w[UR] = -v[UR-1] + 4*v[UR] -    0    - v[UR-cols];   // upper right
}

double CPUDirectSolver::iterationDuration()
{
    return (1000.0*(end - begin)) / CLOCKS_PER_SEC;
}
