#include "CPUDirectSolve3D.h"
#include "cblas.h"

CPUDirectSolve3D::CPUDirectSolve3D(int w, int h, int d, double *x, double *b, double tolerance, int maxIterations) :
    width(w), height(h), depth(d), x(x), b(b), eps(tolerance), maxIterations(maxIterations),
    g(NULL), r(NULL), p(NULL)
{
    wh = width_height = width*height;
    N = width*height*depth;
    this->initialize();
}


CPUDirectSolve3D::~CPUDirectSolve3D()
{
    this->cleanup();
}

void CPUDirectSolve3D::cleanup()
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

void CPUDirectSolve3D::initialize()
{
    #ifdef BLAS_SUPPORT

    if(g != NULL)
        delete []g;
     g = new double[N];

    if(r != NULL)
        delete []r;
    r = new double[N];

    if(p != NULL)
        delete []p;
    p = new double[N];

    its = 0;
    err = eps*eps*ddot(N,b,1,b,1);

    mult(x,g);
    daxpy(N,-1.,b,1,g,1);  // g = -1*b + g         //   change to SAXPY
    dscal(N,-1.,g,1);      // g = -1*g;    (scale g by -1)
    dcopy(N,g,1,r,1);      // r = g        (with strides 1)

    #endif // BLAS_SUPPORT
}


bool CPUDirectSolve3D::runIteration()
{
    #ifdef BLAS_SUPPORT
    this->begin = clock();

    this->mult(r,p);
    rho = ddot(N,p,1,p,1);
    sig = ddot(N,r,1,p,1);
    tau = ddot(N,g,1,r,1);
    t   = tau/sig;
    daxpy(N,t,r,1,x,1);     // x =  t*r + x
    daxpy(N,-t,p,1,g,1);    // g = -t*p + g;
    gam = (t*t*rho-tau)/tau;
    dscal(N,gam,r,1);       // scale vector by constant
    daxpy(N,1.,g,1,r,1);    // r = 1.0*g + r   (with strides 1)
    ++its;

    bool terminationCondition = ddot(N,g,1,g,1)>err && its < maxIterations;
    this->end = clock();

    if(terminationCondition)
        return false;  // not done
    else
        return true;   // done
    #endif //BLAS_SUPPORT

    return true;
}

double CPUDirectSolve3D::residual()
{
   #ifdef BLAS_SUPPORT
   return ddot(N,g,1,g,1);
   #endif //BLAS_SUPPORT

   return 0;
}

void CPUDirectSolve3D::solve()
{
    #ifdef BLAS_SUPPORT
    float t, tau, sig, rho, gam;

    while ( ddot(N,g,1,g,1)>err && its < maxIterations) {
      mult(r,p);
      rho = ddot(N,p,1,p,1);
      sig = ddot(N,r,1,p,1);
      tau = ddot(N,g,1,r,1);
      t   = tau/sig;
      daxpy(N,t,r,1,x,1);
      daxpy(N,-t,p,1,g,1);
      gam = (t*t*rho-tau)/tau;
      dscal(N,gam,r,1);
      daxpy(N,1.,g,1,r,1);
      ++its;
    }
    #endif //BLAS_SUPPORT

}



#define LFT(x) (x - 1)
#define RGT(x) (x + 1)
#define UP(x)  (x + width)
#define DWN(x) (x - width)
#define FWD(x) (x + width_height)
#define BCK(x) (x - width_height)

void CPUDirectSolve3D::mult(const double*v, double*w )
{
    //---------------------------------------//
    //   Multiply Interior Region            //
    //---------------------------------------//
    for(int k=1; k < depth-1; k++)
    {
        for(int j=1; j < height-1; j++)
        {
            for(int i=1; i < width-1; i++)
            {
                // TODO: Put Diffs in Order For Cache Coherency. One Directional (match major order)
                int cell = k*width_height + j*width + i;
                w[cell] = + 6*v[cell]
                          - v[LFT(cell)] - v[RGT(cell)]
                          - v[DWN(cell)] - v[ UP(cell)]
                          - v[BCK(cell)] - v[FWD(cell)];
            }
        }
    }

    //---------------------------------------//
    //   Multiply Face Boundaries            //
    //---------------------------------------//
    // left & right boundaries
    int i1 = 0, i2 = width-1;
    for(int k=1; k < depth-1; k++)
    {
        for(int j=1; j < height-1; j++)
        {
            int cell1 = k*width_height + j*width + i1;
            int cell2 = k*width_height + j*width + i2;
            w[cell1] = -    0         + 6*v[cell1] - v[RGT(cell1)] - v[FWD(cell1)] - v[BCK(cell1)] - v[UP(cell1)] - v[DWN(cell1)]; // left face
            w[cell2] = -v[LFT(cell2)] + 6*v[cell2] -      0        - v[FWD(cell2)] - v[BCK(cell2)] - v[UP(cell1)] - v[DWN(cell1)]; // right face
        }
    }

    // top & bottom boundaries
    int j1 = 0, j2 = height-1;
    for(int k=1; k < depth-1; k++)
    {
        for(int i=1; i < width-1; i++)
        {
            int cell1 = k*width_height + j1*width + i;
            int cell2 = k*width_height + j2*width + i;
            w[cell1] = -v[LFT(cell1)] + 6*v[cell1] - v[RGT(cell1)] - v[FWD(cell1)] - v[BCK(cell1)] -  v[UP(cell1)] -       0      ;  // bottom face
            w[cell2] = -v[LFT(cell2)] + 6*v[cell2] - v[RGT(cell2)] - v[FWD(cell2)] - v[BCK(cell2)] -      0        - v[DWN(cell2)];  // top face
        }
    }

    // front & back boundaries
    int k1 = 0, k2 = depth-1;
    for(int j=1; j < height-1; j++)
    {
        for(int i=1; i < width-1; i++)
        {
            int cell1 = k1*width_height + j*width + i;
            int cell2 = k2*width_height + j*width + i;
            w[cell1] = -v[LFT(cell1)] + 6*v[cell1] - v[RGT(cell1)] - v[FWD(cell1)] -      0        - v[UP(cell1)] - v[DWN(cell1)];  // front face
            w[cell2] = -v[LFT(cell2)] + 6*v[cell2] - v[RGT(cell2)] -     0         - v[BCK(cell2)] - v[UP(cell2)] - v[DWN(cell2)];  // back face
        }
    }

    //---------------------------------------//
    //      Multiply Edge Boundaries (12)    //
    //---------------------------------------//
    // x-axis aligned edges
    {
        int j1=0, j2 = height-1;
        int k1=0, k2 = depth-1;

        for(int i=1; i < width-1; i++)
        {
            int cell1 = k1*width_height + j1*width + i;
            int cell2 = k1*width_height + j2*width + i;
            int cell3 = k2*width_height + j1*width + i;
            int cell4 = k2*width_height + j2*width + i;

            w[cell1] = + 6*v[cell1]  - v[LFT(cell1)] - v[RGT(cell1)]  - v[DWN(cell1)] - v[UP(cell1)]  - v[BCK(cell1)] - v[FWD(cell1)];  // bottom front
            w[cell2] = + 6*v[cell2]  - v[LFT(cell2)] - v[RGT(cell2)]  - v[DWN(cell2)] - v[UP(cell2)]  - v[BCK(cell2)] - v[FWD(cell2)];  // top    front
            w[cell3] = + 6*v[cell3]  - v[LFT(cell3)] - v[RGT(cell3)]  - v[DWN(cell3)] - v[UP(cell3)]  - v[BCK(cell3)] - v[FWD(cell3)];  // bottom back
            w[cell4] = + 6*v[cell4]  - v[LFT(cell4)] - v[RGT(cell4)]  - v[DWN(cell4)] - v[UP(cell4)]  - v[BCK(cell4)] - v[FWD(cell4)];  // top    back
        }
    }
    // y-axis aligned edges
    {
        int i1=0, i2 = width-1;
        int k1=0, k2 = depth-1;

        for(int j=1; j < height-1; j++)
        {
            int cell1 = k1*width_height + j*width + i1;
            int cell2 = k2*width_height + j*width + i1;
            int cell3 = k1*width_height + j*width + i2;
            int cell4 = k2*width_height + j*width + i2;

            w[cell1] = + 6*v[cell1] -       0       - v[RGT(cell1)] - v[DWN(cell1)] - v[UP(cell1)] -      0        - v[FWD(cell1)];  // front left
            w[cell2] = + 6*v[cell2] - v[LFT(cell2)] -       0       - v[DWN(cell2)] - v[UP(cell2)] -      0        - v[FWD(cell2)];  // front right
            w[cell3] = + 6*v[cell3] -       0       - v[RGT(cell3)] - v[DWN(cell3)] - v[UP(cell3)] - v[BCK(cell3)] -       0      ;  // back left
            w[cell4] = + 6*v[cell4] - v[LFT(cell4)] -       0       - v[DWN(cell4)] - v[UP(cell4)] - v[BCK(cell4)] -       0      ;  // back right
        }
    }
    // z-axis aligned edges
    {
        int i1=0, i2=width-1;
        int j1=0, j2=height-1;

        for(int k=1; k < depth-1; k++)
        {
            int cell1 = k*width_height + j1*width + i1;
            int cell2 = k*width_height + j1*width + i2;
            int cell3 = k*width_height + j2*width + i1;
            int cell4 = k*width_height + j2*width + i2;

            w[cell1] = + 6*v[cell1] -       0       - v[RGT(cell1)]  -       0       - v[UP(cell1)] - v[BCK(cell1)] - v[FWD(cell1)];  // bottom left
            w[cell2] = + 6*v[cell2] - v[LFT(cell2)] -       0        -       0       - v[UP(cell2)] - v[BCK(cell2)] - v[FWD(cell2)];  // bottom right
            w[cell3] = + 6*v[cell3] -       0       - v[RGT(cell3)]  - v[DWN(cell3)] -      0       - v[BCK(cell3)] - v[FWD(cell3)];  // top left
            w[cell4] = + 6*v[cell4] - v[LFT(cell4)] -       0        - v[DWN(cell4)] -      0       - v[BCK(cell4)] - v[FWD(cell4)];  // top right
        }
    }


    //---------------------------------------//
    //       Multiply Corner Boundaries      //
    //---------------------------------------//
    int min_x = 0; int max_x = width-1;
    int min_y = 0; int max_y = height-1;
    int min_z = 0; int max_z = depth-1;

    int LLF = min_x*width*height + min_y*width + min_z;
    int RLF = max_x*width*height + min_y*width + min_z;
    int LUF = min_x*width*height + max_y*width + min_z;
    int RUF = max_x*width*height + max_y*width + min_z;
    int LLB = min_x*width*height + min_y*width + max_z;
    int RLB = max_x*width*height + min_y*width + max_z;
    int LUB = min_x*width*height + max_y*width + max_z;
    int RUB = max_x*width*height + max_y*width + max_z;

    w[LLF] = + 6*v[LLF] -      0       - v[RGT(LLF)] -       0      - v[UP(LLF)] -       0      - v[FWD(LLF)];   // left  lower front
    w[RLF] = + 6*v[RLF] - v[LFT(RLF)] -       0      -       0      - v[UP(RLF)] -       0      - v[FWD(RLF)];   // right lower front
    w[LUF] = + 6*v[LUF] -      0       - v[RGT(LUF)] - v[DWN(LUF)] -      0      -       0      - v[FWD(LUF)];   // left  upper front
    w[RUF] = + 6*v[RUF] - v[LFT(RUF)] -       0      - v[DWN(RUF)] -      0      -       0      - v[FWD(RUF)];   // right upper front
    w[LLB] = + 6*v[LLB] -      0       - v[RGT(LLB)] -       0      - v[UP(LLB)] - v[BCK(LLB)] -       0     ;   // left  lower back
    w[RLB] = + 6*v[RLB] - v[LFT(RLB)] -       0      -       0      - v[UP(RLB)] - v[BCK(RLB)] -       0     ;   // right lower back
    w[LUB] = + 6*v[LUB] -      0       - v[RGT(LUB)] - v[DWN(LUB)] -      0      - v[BCK(LUB)] -       0     ;   // left  upper back
    w[RUB] = + 6*v[RUB] - v[LFT(RUB)] -       0      - v[DWN(RUB)] -      0      - v[BCK(RUB)] -       0     ;   // right upper back
}

double CPUDirectSolve3D::iterationDuration()
{
    return (1000.0*(end - begin)) / CLOCKS_PER_SEC;
}
