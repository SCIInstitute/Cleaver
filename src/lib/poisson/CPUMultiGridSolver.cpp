//=========================================================================
//  CPUMultiGridSolver.cpp
//
//  This file is the definition for the CPU implementation of the MultiGrid
//  version of the Conjugate Gradient solver. It uses the Base Class
//  Solver as foundation, and is meant to be called, externally, from a
//  GUI Window.
//
//  Author: Jonathan Bronson
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#include "CPUMultiGridSolver.h"
#include "cblas.h"
#include "math.h"
#include "string.h"
#include "time.h"
#include <iostream>

using namespace std;

CPUMultiGridSolver::CPUMultiGridSolver(int n, int levels, float *x, float *b, float tolerance, int maxIterations)
    : eps(tolerance), maxIterations(maxIterations), g(NULL),r(NULL),p(NULL)
{
    if(levels < 1)
        levels = 1;



    N = new int[levels];
    this->rows = new int[levels];
    this->cols = new int[levels];
    this->x = new float*[levels];
    this->b = new float*[levels];

    for(int i=0; i < levels; i++)
    {
        N[i] = n*n;
        this->rows[i] = n;
        this->cols[i] = n;
        n /= 2;
    }

    // use pointers to base level directly
    this->x[0] = x;
    this->b[0] = b;

    // allocate memory for reduced regions (for now)
    for(int i=1; i < levels; i++)
    {
        this->x[i] = new float[N[i]];
        this->b[i] = new float[N[i]];
    }

    this->numLevels = levels;
    this->level = 0;
    down_phase = true;
    up_phase = false;
    this->initialize();
}

CPUMultiGridSolver::~CPUMultiGridSolver()
{
    this->cleanup();
}

void CPUMultiGridSolver::cleanup()
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
    if(N != NULL){
        delete [] N;
        N = NULL;
    }
    if(rows != NULL){
        delete [] rows;
        rows = NULL;
    }
    if(cols != NULL){
        delete [] cols;
        cols = NULL;
    }

    for(int i=1; i < this->numLevels; i++){
        delete this->x[i];
        delete this->b[i];
    }

    delete []this->x;
    delete []this->b;
}

void CPUMultiGridSolver::initialize()
{
    if(g != NULL)
        delete []g;
     g = new float[N[0]];

    if(r != NULL)
        delete []r;
    r = new float[N[0]];

    if(p != NULL)
        delete []p;
    p = new float[N[0]];

    its = 0;
    err = eps*eps*sdot(N[level],b[level],1,b[level],1);

    mult(x[level],g);
    saxpy(N[level],-1.,b[level],1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N[level],-1.,g,1);             // g = -1*g;            (scale g by -1)
    scopy(N[level],g,1,r,1);             // r = g                (with strides 1)
}

void CPUMultiGridSolver::solve()
{
    // batch solve without animation, not implemented
}


bool CPUMultiGridSolver::runIteration()
{
    bool workLeft = true;
    this->begin = clock();

    this->mult(r,p);
    rho=sdot(N[level],p,1,p,1);
    sig=sdot(N[level],r,1,p,1);
    tau=sdot(N[level],g,1,r,1);
    t=tau/sig;
    saxpy(N[level],t,r,1,x[level],1);     // x =  t*r + x
    saxpy(N[level],-t,p,1,g,1);           // g = -t*p + g;
    gam=(t*t*rho-tau)/tau;
    sscal(N[level],gam,r,1);              // scale vector by constant
    saxpy(N[level],1.,g,1,r,1);           // r = 1.0*g + r   (with strides 1)
    ++its;

    workLeft = workLeft && sdot(N[level],g,1,g,1)>err && its < maxIterations;
    workLeft = workLeft && (!down_phase || (down_phase && its < 3));
    this->end = clock();

    if(workLeft)
        return false;  // not done
    else
        return true;   // done
}

void CPUMultiGridSolver::reduce()
{
    level++;
    this->begin = clock();

    for(int j=0; j < rows[level]; j++){
        for(int i=0; i < cols[level]; i++){

            int ii = i*2;
            int jj = j*2;
            float x_value = 0, b_value = 0;

            x_value += x[level-1][(jj+0)*rows[level-1] + ii + 0];
            x_value += x[level-1][(jj+0)*rows[level-1] + ii + 1];
            x_value += x[level-1][(jj+1)*rows[level-1] + ii + 0];
            x_value += x[level-1][(jj+1)*rows[level-1] + ii + 1];
            x_value *= 0.25f;


            b_value += b[level-1][(jj+0)*rows[level-1] + ii + 0];
            b_value += b[level-1][(jj+0)*rows[level-1] + ii + 1];
            b_value += b[level-1][(jj+1)*rows[level-1] + ii + 0];
            b_value += b[level-1][(jj+1)*rows[level-1] + ii + 1];
            b_value *= 0.5;

            x[level][j*rows[level] + i] = x_value;
            b[level][j*rows[level] + i] = b_value;
        }
    }

    this->end = clock();
    cout << "Reduction from Level " << level-1 << " to " << level << " took " << this->iterationDuration() << "ms" << endl;

    if(level == numLevels-1){
        down_phase = false;
        up_phase = true;
    }

    // prepare for next solver
    its = 0;

    err = eps*eps*sdot(N[level],b[level],1,b[level],1);
    mult(x[level],g);
    saxpy(N[level],-1.,b[level],1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N[level],-1.,g,1);             // g = -1*g;            (scale g by -1)
    scopy(N[level],g,1,r,1);             // r = g                (with strides 1)
}

void CPUMultiGridSolver::interpolate()
{
    this->begin = clock();

    // expand rows[i] x cols[i] regions to be rows[i-1] x cols[i-1] regions
    // Nearest Neighbor Interpolation
    for(int j=0; j < rows[level]; j++){
        for(int i=0; i < cols[level]; i++){
            float value = x[level][j*rows[level] + i];
            int ii = i*2;
            int jj = j*2;
            x[level-1][(jj+0)*rows[level-1] + ii + 0] = value;
            x[level-1][(jj+0)*rows[level-1] + ii + 1] = value;
            x[level-1][(jj+1)*rows[level-1] + ii + 0] = value;
            x[level-1][(jj+1)*rows[level-1] + ii + 1] = value;
        }
    }

    this->end = clock();
    cout << "Interpolation from Level " << level << " to " << level-1 << " took " << this->iterationDuration() << "ms" << endl;



    // commmit interpolation
    level--;

    if(level == 0)
        up_phase = false;

    // prepare for next level solver
    its = 0;
    err = eps*eps*sdot(N[level],b[level],1,b[level],1);
    mult(x[level],g);
    saxpy(N[level],-1.,b[level],1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N[level],-1.,g,1);             // g = -1*g;            (scale g by -1)
    scopy(N[level],g,1,r,1);             // r = g                (with strides 1)
}

void CPUMultiGridSolver::bilinearInterpolate()
{
    // copy borders exactly
    // top row and bottom row
    int j = rows[level]-1;
    for(int i=0; i < cols[level]; i++){
        float topValue    = x[level][j*rows[level] + i];
        float bottomValue = x[level][j*rows[level] + i];
        int jj = j*2;
        int ii = i*2;
        x[level-1][ii + 0] = topValue;
        x[level-1][ii + 1] = topValue;

        x[level-1][jj*rows[level-1] + ii + 0] = bottomValue;
        x[level-1][jj*rows[level-1] + ii + 1] = bottomValue;
    }


    // interpolate interior
    for(int j=1; j < rows[level-1]-1; j++)
    {
        for(int i=1; i < cols[level-1]-1; i++)
        {
            float px = 0.5f*(i+0.5f);
            float py = 0.5f*(j+0.5f);

            float LL_x = int(px - 0.5f) + 0.5f;
            float LL_y = int(py - 0.5f) + 0.5f;

            float LR_x = LL_x + 1.0f;
            float LR_y = LL_y;

            float UL_x = LL_x;
            float UL_y = LL_y + 1.0f;

            float UR_x = LL_x + 1.0f;
            float UR_y = LL_y + 1.0f;

            float F00 = x[level][int(LL_y)*cols[level] + int(LL_x)];
            float F10 = x[level][int(LR_y)*cols[level] + int(LR_x)];
            float F01 = x[level][int(UL_y)*cols[level] + int(UL_x)];
            float F11 = x[level][int(UR_y)*cols[level] + int(UR_x)];
            float x = px - LL_x;
            float y = py - LL_y;
            float value = F00*(1-x)*(1-y) + F10*x*(1-y) + F01*(1-x)*y + F11*x*y;

            this->x[level-1][j*rows[level-1] + i] = value;
        }
    }

    level--;

    if(level == 0)
        up_phase = false;

    // prepare for next level solver
    its = 0;
    err = eps*eps*sdot(N[level],b[level],1,b[level],1);
    mult(x[level],g);
    saxpy(N[level],-1.,b[level],1,g,1);  // g = -1*b + g         //   change to SAXPY
    sscal(N[level],-1.,g,1);             // g = -1*g;            (scale g by -1)
    scopy(N[level],g,1,r,1);             // r = g                (with strides 1)
}

void CPUMultiGridSolver::mult(const float *v, float *w )
{
    int rows = this->rows[level];
    int cols = this->cols[level];

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

double CPUMultiGridSolver::iterationDuration()
{
    return (1000.0*(end - begin)) / CLOCKS_PER_SEC;
}


