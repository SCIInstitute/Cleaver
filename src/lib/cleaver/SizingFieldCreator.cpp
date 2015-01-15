//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
// -- SizingFieldCreator Class
//
//  Primary Author: Shankar Sastry
//  Additional Authors:  Jonathan Bronson
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2011, 2012, Jonathan Bronson
//  Scientific Computing & Imaging Institute
//  University of Utah
//
//  Permission is  hereby  granted, free  of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files  ( the "Software" ),  to  deal in  the  Software without
//  restriction, including  without limitation the rights to  use,
//  copy, modify,  merge, publish, distribute, sublicense,  and/or
//  sell copies of the Software, and to permit persons to whom the
//  Software is  furnished  to do  so,  subject  to  the following
//  conditions:
//
//  The above  copyright notice  and  this permission notice shall
//  be included  in  all copies  or  substantial  portions  of the
//  Software.
//
//  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
//  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
//  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
//  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//  USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------

#include "SizingFieldCreator.h"
#include <vector>
#include "vec3.h"

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <limits>

#include <queue>
#include <cmath>
#include <set>
#include "vec3.h"
#include "Octree.h"
#include "BoundingBox.h"
#include "Status.h"

using namespace std;

namespace cleaver
{
 #ifndef isnan
 #define isnan(x) (x)!=(x)
 #endif

class CompareDist
{
    public:
    bool operator()(const QueueIndex& t1, const QueueIndex& t2) // Returns true if t1 is earlier than t2
    {
        return (t1.dist > t2.dist);
    }
};


VoxelMesh::VoxelMesh(bool verbose) : m_verbose(verbose)
{
}

VoxelMesh::~VoxelMesh()
{
    //std::cout << "Deallocating VoxelMesh!: " << this << std::endl;
}

void VoxelMesh::init(int l, int m, int n)
{
    bool temp1=false;
    int i;
    vector<bool> temp2;
    vector<vector<bool> > temp3;
    for(i=0; i<n; i++)
        temp2.push_back(temp1);
    for(i=0; i<m; i++)
        temp3.push_back(temp2);
    for(i=0; i<l; i++)
        known.push_back(temp3);

    double temp5=1e10;
    vector<double> temp6;
    vector<vector<double> >temp7;
    for(i=0; i<n; i++)
        temp6.push_back(temp5);
    for(i=0; i<m; i++)
        temp7.push_back(temp6);
    for(i=0; i<l; i++)
        dist.push_back(temp7);
}

ScalarField<float>* VoxelMesh::convertToFloatField(float factor, const cleaver::vec3 &padding, const cleaver::vec3 &offset)
{
    float *field;
    int w,h,d,i,j,k;
    ScalarField<float> *ret;
    double min=0;

    w = dist.size(); h=dist[0].size(); d=dist[0][0].size();

    //field = (float*)malloc(w*h*d*sizeof(float));
    field = new float[w*h*d];
    for(i=0; i<w; i++)
    {
        for(j=0; j<h; j++)
        {
            for(k=0;k<d; k++)
            {
                min=dist[i][j][k];
                field[k*(w*h)+j*w+i] = (float)(min/factor);
                //printf("%d %d %d %lf\n", i,j,k,dist[i][j][k]);
            }
        }
    }
    ret = new ScalarField<float>(field, w, h, d);
    double sx = 1.0/factor;
    double sy = 1.0/factor;
    double sz = 1.0/factor;
    ret->setScale(vec3(sx,sy,sz));
    vec3 origin(-offset[0], -offset[1], -offset[2]);
    vec3 mySize(w*sx, h*sy, d*sz);
    ret->setBounds(BoundingBox(origin, mySize));
    //std::cout << "w=" << w << " h=" << h << " d=" << d << std::endl;
    //std::cout << "sx=" << sx << " sy=" << sy << " sz=" << sz << std::endl;
    //std::cout << "padding = " << padding.toString() << std::endl;


    //printf("%lf %lf %lf\n", offset.x, offset.y, offset.z);
    //printf("%lf %lf %lf\n", padding[0],padding[1],padding[2]);
    //ret->setCenterType(Cleaver::FloatField::NodeCentered);
    //write the NRRD file

    // first write header file
        std::string filename = "sizing_field.nrrd";
        if (m_verbose)
            std::cout << "Writing file '" << filename << "'" << std::endl;
        ofstream nrrd_file(filename.c_str(), std::ofstream::binary);

        // JRB: added safety check
        if(nrrd_file.is_open())
        {
            nrrd_file << "NRRD0001" << std::endl;
            nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
            nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
            nrrd_file << "type: float" << std::endl;
            nrrd_file << "dimension: 3" << std::endl;
            nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
            nrrd_file << "axis mins: " << -offset.x << ", " << -offset.y << ", " << -offset.z << std::endl;
            nrrd_file << "spacings: " << sx << " " << sy << " " << sz << std::endl;
            nrrd_file << "centerings: cell cell cell" << std::endl;
            nrrd_file << "endian: little" << std::endl;
            nrrd_file << "encoding: raw" << std::endl;
            nrrd_file << std::endl;

            // write data portion
            for(int k=0; k < d; k++)
            {
                for(int j=0; j < h; j++)
                {
                    for(int i=0; i < w; i++)
                    {
                        float val = (float)dist[i][j][k]/factor;
                        nrrd_file.write((char*)&val, sizeof(float));
                        //if(dist[i][j][k]<0.5)
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist[i][j][k]<<endl;
                    }
                }
            }

            nrrd_file.close();

            //ret->setCenterType(FloatField::NodeCentered);
        }
        else{
            std::cerr << "Failed to open file '" << filename << "' for writing." << std::endl;
        }

    return ret;
}

//------------------------------------------------------------------
//------------------------------------------------------------------


SizingFieldCreator::SizingFieldCreator(const Volume *volume, float speed, float sampleFactor, float sizingFactor, int padding, bool adaptiveSurface, bool verbose) :
    m_speed(speed), m_sampleFactor(sampleFactor), m_sizingFactor(sizingFactor), m_verbose(verbose)
{
    m_padding[0]=m_padding[1]=m_padding[2]=2*padding;
    m_offset[0]=m_offset[1]=m_offset[2]=padding;


    //Variable Declaration
    int i,j,k,l;//,n;
    int w,h,d,m;
    double a,/*b,c,*/x,y,z,discont=275e-2,xdist=1,ydist=1,zdist=1;
    vector<vector<vector<double> > > mesh_discont;
    //double **image;
    //std::cout<<"Factor = "<<samplefactor<<std::endl;
    int neighbour[6][3] =
    {
        {-1,0,0},
        {1,0,0},
        {0,-1,0},
        {0,1,0},
        {0,0,-1},
        {0,0,1}
    };

    //Find Material
    //Volume volume;
    vector<vector<vector<Voxel> > > voxel;
    vector<Triple> zeros, medialaxis;
    vector<vector<vector<bool> > > myBdry;


        //w = lattice->width();
        //h = lattice->height();
        //d = lattice->depth();
        //m = lattice->materials();
    //if(volume->getSizingField() > 0)
    //{
        w = (int)(volume->bounds().size.x*m_sampleFactor);
        h = (int)(volume->bounds().size.y*m_sampleFactor);
        d = (int)(volume->bounds().size.z*m_sampleFactor);
        m = (int)(volume->numberOfMaterials());
    //}

    //w=15; h=15; d=15; m=2;
    double myTemp1 = 0;
    vector<double> myTemp2;
    vector<vector<double> >myTemp3;
    for(i=0; i<d; i++)
        myTemp2.push_back(myTemp1);
    for(i=0; i<h; i++)
        myTemp3.push_back(myTemp2);
    for(i=0; i<w; i++)
        mesh_discont.push_back(myTemp3);

    Voxel temp1;
    vector<Voxel> temp2;
    vector<vector<Voxel> >temp3;
    for(i=0; i<d; i++)
        temp2.push_back(temp1);
    for(i=0; i<h; i++)
        temp3.push_back(temp2);
    for(i=0; i<w; i++)
        voxel.push_back(temp3);

    myBdry.resize(w);
    for(i=0; i<w; i++)
        myBdry[i].resize(h);
    for(i=0; i<w; i++)
        for(j=0; j<h; j++)
            myBdry[i][j].resize(d, false);

    mesh_bdry.init(w,h,d);
    mesh_feature.init(w,h,d);

    Status status(w*d*h);
    for(k=0; k<d; k++)
    {
        for(j=0; j<h; j++)
        {
            for(i=0; i<w; i++)
            {
                double ii = (double)(i+0.5)/m_sampleFactor;
                double jj = (double)(j+0.5)/m_sampleFactor;
                double kk = (double)(k+0.5)/m_sampleFactor;
                int dom = 0;
                double max = volume->valueAt(ii,jj,kk,dom);
                for(int mat=1; mat < m; mat++)
                {
                    double val = volume->valueAt(ii,jj,kk,mat);
                    if(val > max)
                    {
                        max = val;
                        dom = mat;
                    }
                }
                voxel[i][j][k].mat=dom;
                if (verbose) status.printStatus();
            }
        }
    }
    if (verbose) status.done();
    if (verbose) std::cout << "Finding boundary vertices..." << std::endl;
    if (verbose) status = Status(w*h*d*6);

    //Find Boundary Vertices
    for(i=0; i<w; i++)
    {
        for(j=0; j<h; j++)
        {
            for(k=0; k<d; k++)
            {
                //Compare this voxel with its six neighbours
                for(l=0; l<6; l++)
                {
                    int i1,j1,k1;
                    i1=i+neighbour[l][0];
                    j1=j+neighbour[l][1];
                    k1=k+neighbour[l][2];

                    QueueIndex temp_q = make_index(i1,j1,k1);
                    if(exists(temp_q, mesh_bdry) && voxel[i][j][k].mat != voxel[i1][j1][k1].mat)
                    {
                        double i_star, j_star, k_star;
                        double dist=Newton(volume, make_triple(i,j,k), make_triple(i1,j1,k1), voxel[i][j][k].mat, voxel[i1][j1][k1].mat, i_star, j_star, k_star);

                        //if(dist>=sqrt(3))
                        //{//Sanity check
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist<<endl;
                        //    dist = 2;
                        //}

                        //if(i==23 && j==61 && k==84)
                            //printf("here %lf\n", dist);
                        zeros.push_back(make_triple(i,j,k));
                        myBdry[i][j][k]=true;
                        if(dist<mesh_bdry.dist[i][j][k])
                            mesh_bdry.dist[i][j][k]=dist;
                    }
                    if (verbose) status.printStatus();
                }
            }
        }
    }
    if (verbose) status.done();

    if(!adaptiveSurface)
    {
        vector<Triple>::iterator it;
        for(it = zeros.begin(); it!=zeros.end(); it++)
        {
            int i0 = (*it).index[0];
            int j0 = (*it).index[1];
            int k0 = (*it).index[2];
            mesh_feature.dist[i0][j0][k0]=1;
            mesh_feature.known[i0][j0][k0]=true;
        }
        vec3 mypadding = m_sampleFactor*m_padding;
        vec3 myoffset = m_sampleFactor*m_offset;
        appendPadding(mypadding, myoffset, zeros);
    }
    else
    {

        std::string filename = "boundary.nrrd";
        if (m_verbose)
            std::cout << "Writing file '" << filename << "'" << std::endl;
        ofstream nrrd_file(filename.c_str(), std::ofstream::binary);

        // JRB: added safety check
        if(nrrd_file.is_open())
        {
            nrrd_file << "NRRD0001" << std::endl;
            nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
            nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
            nrrd_file << "type: float" << std::endl;
            nrrd_file << "dimension: 3" << std::endl;
            nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
            nrrd_file << "axis mins: " << 0 << ", " << 0 << ", " << 0 << std::endl;
            nrrd_file << "spacings: " << 1 << " " << 1 << " " << 1 << std::endl;
            nrrd_file << "centerings: cell cell cell" << std::endl;
            nrrd_file << "endian: little" << std::endl;
            nrrd_file << "encoding: raw" << std::endl;
            nrrd_file << std::endl;

            if (verbose) status = Status(h*w*d);
            // write data portion
            for(int k=0; k < d; k++)
            {
                for(int j=0; j < h; j++)
                {
                    for(int i=0; i < w; i++)
                    {
                        float val = (float)mesh_bdry.dist[i][j][k];
                        nrrd_file.write((char*)&val, sizeof(float));
                        if (verbose) status.printStatus();
                        //if(val<1)
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist[i][j][k]<<endl;
                    }
                }
            }

            nrrd_file.close();
        }

        if (verbose) status.done();


        if (verbose) printf("\tComputing the distance transform\n"); 
        proceed(mesh_bdry, zeros, 1, 1e6);


        filename = "boundary_field.nrrd";
        if (m_verbose)
            std::cout << "Writing file '" << filename << "'" << std::endl;
        nrrd_file.open(filename.c_str(), std::ofstream::binary);

        // JRB: added safety check
        if(nrrd_file.is_open())
        {
            nrrd_file << "NRRD0001" << std::endl;
            nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
            nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
            nrrd_file << "type: float" << std::endl;
            nrrd_file << "dimension: 3" << std::endl;
            nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
            nrrd_file << "axis mins: " << 0 << ", " << 0 << ", " << 0 << std::endl;
            nrrd_file << "spacings: " << 1 << " " << 1 << " " << 1 << std::endl;
            nrrd_file << "centerings: cell cell cell" << std::endl;
            nrrd_file << "endian: little" << std::endl;
            nrrd_file << "encoding: raw" << std::endl;
            nrrd_file << std::endl;

            if (verbose) status = Status(h*w*d);
            // write data portion
            for(int k=0; k < d; k++)
            {
                for(int j=0; j < h; j++)
                {
                    for(int i=0; i < w; i++)
                    {
                        float val = (float)mesh_bdry.dist[i][j][k];
                        nrrd_file.write((char*)&val, sizeof(float));
                        if (verbose) status.printStatus();
                        //if(val<1)
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist[i][j][k]<<endl;
                    }
                }
            }

            nrrd_file.close();
        }
        if (verbose) status.done();

        //Search for discontinuity
        if (verbose)
          printf("\tSearching for discontinuity in the distance field\n"); 
        medialaxis.clear();
        if (verbose) status = Status((h-1)*(w-1)*(d-1) + (h-3)*(w-3)*(d-3));
        for(i=1; i<w; i++)
        {
            for(j=1; j<h; j++)
            {
                for(k=1; k<d; k++)
                {
                    if (verbose) status.printStatus();
                    a=mesh_bdry.dist[i][j][k];
                    //if(i==236 && j==179 && k==229)
                    //    cout<<i<<" "<<j<<" "<<k<<" "<<a<<endl;
                    if (a<xdist || a<ydist || a<zdist)
                        continue;
                    if(myBdry[i][j][k])
                        continue;

                    bool flag=false;

                    for(l=0; l<6; l++)
                    {
                        int i1,j1,k1;
                        i1=i+neighbour[l][0];
                        j1=j+neighbour[l][1];
                        k1=k+neighbour[l][2];

                        QueueIndex temp_q = make_index(i1,j1,k1);
                        if(!exists(temp_q, mesh_bdry))
                        {
                            flag=true;
                        }
                        else if(voxel[i][j][k].mat != voxel[i1][j1][k1].mat && mesh_bdry.dist[i1][j1][k1] != 0)
                        {
                            flag=true;
                        }
                    }

                    if(flag)
                        continue;

                    x = (mesh_bdry.dist[i-1][j][k]-2*a+mesh_bdry.dist[i+1][j][k])/(xdist*xdist);
                    y = (mesh_bdry.dist[i][j-1][k]-2*a+mesh_bdry.dist[i][j+1][k])/(ydist*ydist);
                    z = (mesh_bdry.dist[i][j][k-1]-2*a+mesh_bdry.dist[i][j][k+1])/(zdist*zdist);

                    //printf("%lf ", fabs(x*x+y*y+z*z));
                    mesh_discont[i][j][k] = fabs(x*x+y*y+z*z);

                    if (fabs(x*x+y*y+z*z) > discont)
                    {
                        medialaxis.push_back(make_triple(i,j,k));
                        mesh_feature.dist[i][j][k]=0.0;
                        mesh_feature.known[i][j][k]=true;
                    }
                }
                //printf("\n");
            }
            //printf("\n\n\n");
        }

        for(i=1; i<w-2; i++)
        {
            for(j=1; j<h-2; j++)
            {
                for(k=1; k<d-2; k++)
                {
                    if (verbose) status.printStatus();
                    if(myBdry[i][j][k])
                        continue;
                    a=mesh_bdry.dist[i][j][k];
                    if (a<xdist || a<ydist || a<zdist)
                        continue;
                    if (mesh_discont[i][j][k] < 0.8 || mesh_discont[i][j][k] > discont)
                        continue;
                    for(l=1; l<6; l+=2)
                    {
                        int i1,j1,k1;
                        i1=i+neighbour[l][0];
                        j1=j+neighbour[l][1];
                        k1=k+neighbour[l][2];

                        if (mesh_discont[i1][j1][k1] > 0.5)
                        {

                            int i2,j2,k2;
                            i2=i+2*neighbour[l][0];
                            j2=j+2*neighbour[l][1];
                            k2=k+2*neighbour[l][2];

                            int i0,j0,k0;
                            i0=i-neighbour[l][0];
                            j0=j-neighbour[l][1];
                            k0=k-neighbour[l][2];

                            switch(l)
                            {
                                case 1:
                                x = (mesh_bdry.dist[i0][j0][k0]-(mesh_bdry.dist[i][j][k] + mesh_bdry.dist[i1][j1][k1])+mesh_bdry.dist[i2][j2][k2])/(xdist*xdist);
                                y = (mesh_bdry.dist[i][j-1][k]-2*a+mesh_bdry.dist[i][j+1][k])/(ydist*ydist);
                                z = (mesh_bdry.dist[i][j][k-1]-2*a+mesh_bdry.dist[i][j][k+1])/(zdist*zdist);
                                break;

                                case 3:
                                x = (mesh_bdry.dist[i-1][j][k]-2*a+mesh_bdry.dist[i+1][j][k])/(xdist*xdist);
                                y = (mesh_bdry.dist[i0][j0][k0]-(mesh_bdry.dist[i][j][k] + mesh_bdry.dist[i1][j1][k1])+mesh_bdry.dist[i2][j2][k2])/(xdist*xdist);
                                z = (mesh_bdry.dist[i][j][k-1]-2*a+mesh_bdry.dist[i][j][k+1])/(zdist*zdist);
                                break;

                                case 5:
                                x = (mesh_bdry.dist[i-1][j][k]-2*a+mesh_bdry.dist[i+1][j][k])/(xdist*xdist);
                                y = (mesh_bdry.dist[i][j-1][k]-2*a+mesh_bdry.dist[i][j+1][k])/(ydist*ydist);
                                z = (mesh_bdry.dist[i0][j0][k0]-(mesh_bdry.dist[i][j][k] + mesh_bdry.dist[i1][j1][k1])+mesh_bdry.dist[i2][j2][k2])/(xdist*xdist);
                            }

                            //printf("%lf ", fabs(x*x+y*y+z*z));
                            mesh_discont[i][j][k] = fabs(x*x+y*y+z*z);
                            if (fabs(x*x+y*y+z*z) > discont)
                            {
                                medialaxis.push_back(make_triple(i,j,k));
                                medialaxis.push_back(make_triple(i1,j1,k1));


                                mesh_feature.dist[i][j][k]=0.5;
                                mesh_feature.dist[i1][j1][k1]=0.5;
                                mesh_feature.known[i][j][k]=true;
                                mesh_feature.known[i1][j1][k1]=true;
                            }
                        }
                    }
                }
            }
        }
        if (verbose) status.done();

        filename = "medial.nrrd";
        if (m_verbose)
            std::cout << "Writing file '" << filename << "'" << std::endl;
        nrrd_file.open(filename.c_str(), std::ofstream::binary);

        // JRB: added safety check
        if(nrrd_file.is_open())
        {
            nrrd_file << "NRRD0001" << std::endl;
            nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
            nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
            nrrd_file << "type: float" << std::endl;
            nrrd_file << "dimension: 3" << std::endl;
            nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
            nrrd_file << "axis mins: " << 0 << ", " << 0 << ", " << 0 << std::endl;
            nrrd_file << "spacings: " << 1 << " " << 1 << " " << 1 << std::endl;
            nrrd_file << "centerings: cell cell cell" << std::endl;
            nrrd_file << "endian: little" << std::endl;
            nrrd_file << "encoding: raw" << std::endl;
            nrrd_file << std::endl;

            if (verbose) status = Status(w*h*d);
            // write data portion
            for(int k=0; k < d; k++)
            {
                for(int j=0; j < h; j++)
                {
                    for(int i=0; i < w; i++)
                    {
                        float val = (float)mesh_feature.dist[i][j][k];
                        nrrd_file.write((char*)&val, sizeof(float));
                        if (verbose) status.printStatus();
                        //if(val<1)
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist[i][j][k]<<endl;
                    }
                }
            }

            nrrd_file.close();
        }
        if (verbose) status.done();

        //Thinning (if necessary)
        //Associate the feature size with the with the boundary voxels
        //BSF on the from media axis voxels to boundary voxels
        if (verbose) printf("\tComputing the feature size at the boundary vertices\n"); 
        proceed(mesh_feature, medialaxis, 1, 1e6);

        vec3 mypadding = m_sampleFactor*m_padding;
        vec3 myoffset = m_sampleFactor*m_offset;
        appendPadding(mypadding, myoffset, zeros);

        filename = "feature_field.nrrd";
        if (verbose)
            std::cout << "Writing file '" << filename << "'" << std::endl;
        nrrd_file.open(filename.c_str(), std::ofstream::binary);

        // JRB: added safety check
        if(nrrd_file.is_open())
        {
            nrrd_file << "NRRD0001" << std::endl;
            nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
            nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
            nrrd_file << "type: float" << std::endl;
            nrrd_file << "dimension: 3" << std::endl;
            nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
            nrrd_file << "axis mins: " << 0 << ", " << 0 << ", " << 0 << std::endl;
            nrrd_file << "spacings: " << 1 << " " << 1 << " " << 1 << std::endl;
            nrrd_file << "centerings: cell cell cell" << std::endl;
            nrrd_file << "endian: little" << std::endl;
            nrrd_file << "encoding: raw" << std::endl;
            nrrd_file << std::endl;

            if (verbose) status = Status(w*h*d);
            // write data portion
            for(int k=0; k < d; k++)
            {
                for(int j=0; j < h; j++)
                {
                    for(int i=0; i < w; i++)
                    {
                        float val = (float)mesh_feature.dist[i][j][k];
                        nrrd_file.write((char*)&val, sizeof(float));
                        if (verbose) status.printStatus();
                        //if(val<1)
                        //    cout<<i<<" "<<j<<" "<<k<<" "<<dist[i][j][k]<<endl;
                    }
                }
            }

            nrrd_file.close();
        }
    }

    if (verbose) status.done();
    if (verbose) 
      printf("\tComputing the sizing field in the interior vertices\n"); 
    //takeTheLog(mesh_padded_feature,zeros);
    proceed(mesh_padded_feature, zeros, speed, 1e6);
    //exponentiate(mesh_padded_feature);

    //FloatField *mySizingField = mesh_padded_feature.ConvertToFloatField(samplefactor, mypadding, myoffset);
    //createOctree(volume, mySizingField);

    //------------------------------------------
    //       Apply Scaling Multiplier
    //------------------------------------------
    if(m_sizingFactor != 1.0)
    {
        int w = mesh_padded_feature.dist.size();
        int h = mesh_padded_feature.dist[0].size();
        int d = mesh_padded_feature.dist[0][0].size();

        if (verbose) status = Status(w*h*d);
        for(int k=0; k < d; k++)
        {
            for(int j=0; j < h; j++)
            {
                for(int i=0; i < w; i++)
                {
                    mesh_padded_feature.dist[i][j][k] *= m_sizingFactor;
                    if (verbose) status.printStatus();
                }
            }
        }
    }
    if (verbose) status.done();
}

SizingFieldCreator::~SizingFieldCreator()
{
}

double SizingFieldCreator::compute_size(VoxelMesh &meshfeature, VoxelMesh &meshborder, FeatureOctant *oct, int stack)
{
    double min, temp;
    if(oct->a.index[0] > oct->b.index[0] || oct->a.index[1] > oct->b.index[1]|| oct->a.index[2] > oct->b.index[2])
    {
        //printf("entering %d %d %d %d %d %d %d\n", stack, oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->b.index[0], oct->b.index[1], oct->b.index[2]);
        //scanf("%lf", &min);
        return 1e10;
    }
    //printf("entering %d %d %d %d %d %d %d\n", stack, oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->b.index[0], oct->b.index[1], oct->b.index[2]);
    if(oct->a == oct->b)
    {
        //printf("equal\n");
        if(meshborder.dist[oct->a.index[0]][oct->a.index[1]][oct->a.index[2]]==0)
            oct->min = meshfeature.dist[oct->a.index[0]][oct->a.index[1]][oct->a.index[2]];
        else
            oct->min = 1e10;
        for(int i=0; i<8; i++)
            oct->child[i]=NULL;
        //if(oct->min!=1e10)
            //printf("%d %d %d %lf\n", oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->min);
        //printf("exiting %d %d %d %d %d %d\n", oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->b.index[0], oct->b.index[1], oct->b.index[2]);
        return oct->min;
    }

    Triple mid;
    for(int i=0; i<3; i++)
        mid.index[i]=(oct->a.index[i]+oct->b.index[i])/2;

    //Ennumerate the eight octants
    FeatureOctant suboct[8];

    suboct[0].assign(oct->a.index[0], oct->a.index[1], oct->a.index[2], mid.index[0], mid.index[1], mid.index[2]);
    suboct[1].assign(mid.index[0]+1, oct->a.index[1], oct->a.index[2], oct->b.index[0], mid.index[1], mid.index[2]);
    suboct[2].assign(mid.index[0]+1, mid.index[1]+1, oct->a.index[2], oct->b.index[0], oct->b.index[1], mid.index[2]);
    suboct[3].assign(oct->a.index[0], mid.index[1]+1, oct->a.index[2], mid.index[0], oct->b.index[1], mid.index[2]);
    suboct[4].assign(oct->a.index[0], oct->a.index[1], mid.index[2]+1, mid.index[0], mid.index[1], oct->b.index[2]);
    suboct[5].assign(mid.index[0]+1, oct->a.index[1], mid.index[2]+1, oct->b.index[0], mid.index[1], oct->b.index[2]);
    suboct[6].assign(mid.index[0]+1, mid.index[1]+1, mid.index[2]+1, oct->b.index[0], oct->b.index[1], oct->b.index[2]);
    suboct[7].assign(oct->a.index[0], mid.index[1]+1, mid.index[2]+1, mid.index[0], oct->b.index[1], oct->b.index[2]);

    min=1e10;
    for(int i=0; i<8; i++)
    {
        oct->child[i] = (FeatureOctant *)malloc(sizeof(FeatureOctant));
        oct->child[i]->a = suboct[i].a;
        oct->child[i]->b = suboct[i].b;
        temp = compute_size(meshfeature, meshborder, oct->child[i], stack+1);
        if(temp<min)
            min=temp;
    }
    oct->min = min;
    //if(min!=1e10)
        //printf("%d %d %d %d %d %d %lf\n", oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->b.index[0], oct->b.index[1], oct->b.index[2], min);
    //printf("exiting %d %d %d %d %d %d\n", oct->a.index[0], oct->a.index[1], oct->a.index[2], oct->b.index[0], oct->b.index[1], oct->b.index[2]);
    return min;
}

double SizingFieldCreator::search_size(VoxelMesh &mesh, const Triple &a, const Triple &b, FeatureOctant *c)
{
    double temp, min;
    //printf("%d %d %d %d %d %d\n", a.index[0],a.index[1],a.index[2],b.index[0],b.index[1],b.index[2]);
    //printf("%d %d %d %d %d %d\n\n", c->a.index[0],c->a.index[1],c->a.index[2],c->b.index[0],c->b.index[1],c->b.index[2]);

    if (c->a == a && c->b == b)
    {
        //if(!(a==b))
            //printf("Here\n");
        return c->min;
    }

    min=1e10;
    for(int i=0; i<8; i++)
    {
        if (!(b.index[0]>=c->child[i]->a.index[0] && a.index[0]<=c->child[i]->b.index[0]))
            continue;
        if (!(b.index[1]>=c->child[i]->a.index[1] && a.index[1]<=c->child[i]->b.index[1]))
            continue;
        if (!(b.index[2]>=c->child[i]->a.index[2] && a.index[2]<=c->child[i]->b.index[2]))
            continue;

        Triple a1, b1;
        for(int j=0; j<3; j++)
        {
            if(a.index[j]<=c->child[i]->a.index[j])
                a1.index[j]=c->child[i]->a.index[j];
            else
                a1.index[j]=a.index[j];
        }

        for(int j=0; j<3; j++)
        {
            if(b.index[j]>=c->b.index[j])
                b1.index[j]=c->child[i]->b.index[j];
            else
                b1.index[j]=b.index[j];
        }

        temp = search_size(mesh, a1, b1, c->child[i]);
        if (temp<min)
            min=temp;
    }
    return min;
}

bool SizingFieldCreator::exists(QueueIndex &newtemp, VoxelMesh &mesh)
{
    if((newtemp.index[0] < 0) || (newtemp.index[0] >= (int)mesh.known.size()))
        return false;
    if((newtemp.index[1] < 0) || (newtemp.index[1] >= (int)mesh.known[0].size()))
        return false;
    if((newtemp.index[2] < 0) || (newtemp.index[2] >= (int)mesh.known[0][0].size()))
        return false;
    return true;
}

void SizingFieldCreator::proceed(VoxelMesh &mesh, vector<Triple> &zeros, double F, double max)
{
    int neighbour[6][3] =
    {
        {-1,0,0},
        {1,0,0},
        {0,-1,0},
        {0,1,0},
        {0,0,-1},
        {0,0,1}
    };

    //Fast Marching Method

    priority_queue<QueueIndex, vector<QueueIndex>, CompareDist> myqueue;
    QueueIndex temp, newtemp, left, leftleft, right, rightright;
    set<int> mySet, doneSet;


    int w,h,d;
    w = mesh.dist.size();
    h = mesh.dist[0].size();
    d = mesh.dist[0][0].size();

    for(int i=0; i<w; i++)
    {
        for(int j=0; j<h; j++)
        {
            for(int k=0; k<d; k++)
            {
                mesh.known[i][j][k]=false;
            }
        }
    }


    for(size_t i=0; i < zeros.size(); i++)
    {
        mySet.insert(w*h*zeros[i].index[0]+w*zeros[i].index[1]+zeros[i].index[2]);
        for(int j=0; j<3; j++)
            temp.index[j] = zeros[i].index[j];
        temp.dist = mesh.dist[zeros[i].index[0]][zeros[i].index[1]][zeros[i].index[2]];
        //mesh.known[zeros[i].index[0]][zeros[i].index[1]][zeros[i].index[2]]=1;
        //printf("%lf\n", temp.dist);
        myqueue.push(temp);
    }

    //Dijkstra

    while(!myqueue.empty())
    {
        temp = myqueue.top();
        if (mesh.known[temp.index[0]][temp.index[1]][temp.index[2]]) //known?
        {
            myqueue.pop();
            continue;
        }
        //printf("%d %d %d %lf\n", temp.index[0],temp.index[1],temp.index[2], temp.dist);

        //update the vertex as known
        mesh.known[temp.index[0]][temp.index[1]][temp.index[2]] = true;
        mesh.dist[temp.index[0]][temp.index[1]][temp.index[2]] = temp.dist;
        //if(temp.index[0]==23 && temp.index[1]==61 && temp.index[2]==84)
            //printf("we assign: %lf\n", temp.dist);
        //printf("updated!\n");

        for(int i=0; i<6; i++)
        {
            bool flag0=0, flag1=0, flag2=0, flag3=0;
            //printf("%d\n", i);
            for(int j=0; j<3; j++)
                newtemp.index[j] = temp.index[j] + neighbour[i][j];

            if (!exists(newtemp, mesh))
                continue;
            //printf("%d %d %d\n", newtemp.index[0],newtemp.index[1],newtemp.index[2]);
            if(mesh.known[newtemp.index[0]][newtemp.index[1]][newtemp.index[2]])
                continue;

            //solve quaratic equation
            double x;

            //Get partial derivatives Dx Dy Dz and find coefficients of the quadratic equation
            double coeff[3];
            for(int j=0; j<2; j++)
                coeff[j]=0;
            coeff[2]=-1.0/(F*F);

            for(int j=0; j<6; j+=2)
            {
                flag0=0; flag1=0; flag2=0; flag3=0;
                QueueIndex *neigh=0, *neighnext=0;
                for(int k=0; k<3; k++)
                {
                    right.index[k] = newtemp.index[k] + neighbour[j][k];
                    rightright.index[k] = newtemp.index[k] + 2*neighbour[j][k];
                    left.index[k] = newtemp.index[k] - neighbour[j][k];
                    leftleft.index[k] = newtemp.index[k] - 2*neighbour[j][k];
                }
                if(exists(left, mesh) && mesh.known[left.index[0]][left.index[1]][left.index[2]])
                    flag0=1;
                int dummy = w*h*left.index[0]+w*left.index[1]+left.index[2];
                if(exists(leftleft, mesh) && mesh.known[leftleft.index[0]][leftleft.index[1]][leftleft.index[2]] && mySet.find(dummy)==mySet.end())
                    flag1=1;
                if(exists(right, mesh) && mesh.known[right.index[0]][right.index[1]][right.index[2]])
                    flag2=1;
                dummy = w*h*right.index[0]+w*right.index[1]+right.index[2];
                if(exists(rightright, mesh) && mesh.known[rightright.index[0]][rightright.index[1]][rightright.index[2]] && mySet.find(dummy)==mySet.end())
                    flag3=1;

                if(flag0 && flag2)
                {
                    if(mesh.dist[left.index[0]][left.index[1]][left.index[2]]<mesh.dist[right.index[0]][right.index[1]][right.index[2]])
                    {
                        neigh = &left;
                        if(flag1)
                            neighnext=&leftleft;
                    }
                    else
                    {
                        neigh = &right;
                        if(flag3)
                            neighnext=&rightright;
                    }
                }
                else if (flag0)
                {
                    neigh = &left;
                    if(flag1)
                        neighnext=&leftleft;
                }
                else if (flag2)
                {
                    neigh = &right;
                    if(flag3)
                        neighnext=&rightright;
                }
                else
                {
                    continue;
                }

                double a = 9.0/4, K;
                double val1, val2;
                if(neighnext!=NULL)
                {
                    val1 = mesh.dist[neigh->index[0]][neigh->index[1]][neigh->index[2]];
                    val2 = mesh.dist[neighnext->index[0]][neighnext->index[1]][neighnext->index[2]];
                    if(val2>=val1)
                    {
                        coeff[0]+=1;
                        coeff[1]-=2*val1;
                        coeff[2]+=val1*val1;
                    }
                    else
                    {
                        K = (1.0/3)*(4*val1 - val2);
                        coeff[0]+=a;
                        coeff[1]-=2*a*K;
                        coeff[2]+=a*K*K;

                        /*coeff[0]+=1;
                        coeff[1]-=2*val1;
                        coeff[2]+=val1*val1;*/
                    }
                }
                else
                {
                    val1 = mesh.dist[neigh->index[0]][neigh->index[1]][neigh->index[2]];
                    coeff[0]+=1;
                    coeff[1]-=2*val1;
                    coeff[2]+=val1*val1;
                }
            }

            x = (-coeff[1]+sqrt((coeff[1]*coeff[1])-4*coeff[0]*coeff[2]))/(2*coeff[0]);
            if(isnan(x))
            {
                coeff[0]=0;
                coeff[1]=0;
                coeff[2]=-1.0/(F*F);
                for(int j=0; j<6; j+=2)
                {
                    flag0=0; flag1=0; flag2=0; flag3=0;
                    QueueIndex *neigh=0;
                    for(int k=0; k<3; k++)
                    {
                        right.index[k] = newtemp.index[k] + neighbour[j][k];
                        left.index[k] = newtemp.index[k] - neighbour[j][k];
                    }
                    if(exists(left, mesh) && mesh.known[left.index[0]][left.index[1]][left.index[2]])
                        flag0=1;
                    if(exists(right, mesh) && mesh.known[right.index[0]][right.index[1]][right.index[2]])
                        flag2=1;

                    if(flag0 && flag2)
                    {
                        if(mesh.dist[left.index[0]][left.index[1]][left.index[2]]<mesh.dist[right.index[0]][right.index[1]][right.index[2]])
                        {
                            neigh = &left;
                        }
                        else
                        {
                            neigh = &right;
                        }
                    }
                    else if (flag0)
                    {
                        neigh = &left;
                    }
                    else if (flag2)
                    {
                        neigh = &right;
                    }
                    else
                    {
                        continue;
                    }

                    double val1;
                    val1 = mesh.dist[neigh->index[0]][neigh->index[1]][neigh->index[2]];
                    coeff[0]+=1;
                    coeff[1]-=2*val1;
                    coeff[2]+=val1*val1;
                }
                x = (-coeff[1]+sqrt((coeff[1]*coeff[1])-4*coeff[0]*coeff[2]))/(2*coeff[0]);
            }

            //push into queue
            newtemp.dist = x;
            if(isnan(x))
                printf("Problem with proceed\n");

            //if(newtemp.index[0]==23 && newtemp.index[1]==61 && newtemp.index[2]==84)
                //printf("inserting %lf\n", x);
            myqueue.push(newtemp);
            //printf("\t%d %d %d %lf\n", newtemp.index[0], newtemp.index[1], newtemp.index[2], x);
        }
        myqueue.pop();
    }
    //if (mySet.size()==doneSet.size())
    //    printf("boo\n");
    return;
}

void takeTheLog(VoxelMesh &mesh, vector<Triple> &zeros)
{
    for(size_t i=0; i<zeros.size(); i++)
    {
        double temp = mesh.dist[zeros[i].index[0]][zeros[i].index[1]][zeros[i].index[2]];
        //std::cout << "log10(" << temp << ") = " << log10(temp) << std::endl;
        if(temp != temp){
            std::cerr << "NAN in takeTheLog()" << std::endl;
            exit(-1);
        }
        mesh.dist[zeros[i].index[0]][zeros[i].index[1]][zeros[i].index[2]] = log10(temp)+1;
    }
}

void exponentiate(VoxelMesh &mesh)
{
    int a,b,c,i,j,k;
    a = mesh.dist.size();
    b = mesh.dist[0].size();
    c = mesh.dist[0][0].size();

    for (i=0; i<a; i++)
    {
        for (j=0; j<b; j++)
        {
            for (k=0; k<c; k++)
            {
                double temp = mesh.dist[i][j][k];
                if(temp != temp){
                    std::cerr << "NAN in exponentiate()" << std::endl;
                    std::cout << "pow(10," << temp << ") = " << pow(10,temp) << std::endl;
                    exit(-1);
                }
                //std::cout << "pow(10," << temp << ") = " << pow(10,temp) << std::endl;
                mesh.dist[i][j][k] = pow(10,temp-1);
            }
        }
    }

}

Triple SizingFieldCreator::make_triple(int i, int j, int k)
{
    Triple ret;
    ret.index[0]=i;
    ret.index[1]=j;
    ret.index[2]=k;
    return ret;
}

QueueIndex SizingFieldCreator::make_index(int i, int j, int k)
{
    QueueIndex ret;
    ret.index[0]=i;
    ret.index[1]=j;
    ret.index[2]=k;
    ret.dist = 0;
    return ret;
}

double SizingFieldCreator::valueAt(double x, double y, double z) const
{
    int x1,y1,z1,x2,y2,z2;
    double val_x[4], val_y[2], val_z;

    //trilinear interpolation
    //along x
    x1 = (int)floor(x); y1 = (int)floor(y); z1 = (int)floor(z);
    x2 = (int)ceil(x);  y2 = (int)floor(y); z2 = (int)floor(z);
    if(x2-x1!=0)
        val_x[0] = ((x2-x)/(x2-x1))*mesh_feature.dist[x1][y1][z1] + ((x-x1)/(x2-x1))*mesh_feature.dist[x2][y2][z2];
    else
        val_x[0] = mesh_feature.dist[x1][y1][z1];

    x1 = (int)floor(x); y1 = (int)ceil(y); z1 = (int)floor(z);
    x2 = (int)ceil(x);  y2 = (int)ceil(y); z2 = (int)floor(z);
    if(x2-x1!=0)
        val_x[1] = ((x2-x)/(x2-x1))*mesh_feature.dist[x1][y1][z1] + ((x-x1)/(x2-x1))*mesh_feature.dist[x2][y2][z2];
    else
        val_x[1] = mesh_feature.dist[x1][y1][z1];


    x1 = (int)floor(x); y1 = (int)floor(y); z1 = (int)ceil(z);
    x2 = (int)ceil(x);  y2 = (int)floor(y); z2 = (int)ceil(z);
    if(x2-x1!=0)
        val_x[2] = ((x2-x)/(x2-x1))*mesh_feature.dist[x1][y1][z1] + ((x-x1)/(x2-x1))*mesh_feature.dist[x2][y2][z2];
    else
        val_x[2] = mesh_feature.dist[x1][y1][z1];

    x1 = (int)floor(x); y1 = (int)ceil(y); z1 = (int)ceil(z);
    x2 = (int)ceil(x);  y2 = (int)ceil(y); z2 = (int)ceil(z);
    if(x2-x1!=0)
        val_x[3] = ((x2-x)/(x2-x1))*mesh_feature.dist[x1][y1][z1] + ((x-x1)/(x2-x1))*mesh_feature.dist[x2][y2][z2];
    else
        val_x[3] = mesh_feature.dist[x1][y1][z1];

    //along y
    y1=(y1);
    y2=(y1);
    if(y2-y1!=0)
        val_y[0]=((y2-y)/(y2-y1))*val_x[0] + ((y-y1)/(y2-y1))*val_x[1];
    else
        val_y[0]=val_x[0];

    if(y2-y1!=0)
        val_y[1]=((y2-y)/(y2-y1))*val_x[2] + ((y-y1)/(y2-y1))*val_x[3];
    else
        val_y[1]=val_x[2];

    //along z
    z1=(z1);
    z2=(z1);
    if(z2-z1!=0)
        val_z=((z2-z)/(z2-z1))*val_y[0] + ((z-z1)/(z2-z1))*val_y[1];
    else
        val_z=val_y[0];

    return val_z;

}

double SizingFieldCreator::Fval(const Volume *volume, double x, double y, double z, int mat1, int mat2)
{
    double ret;
    ret = (volume->valueAt((float)x/m_sampleFactor, (float)y/m_sampleFactor, (float)z/m_sampleFactor, mat1) -
           volume->valueAt((float)x/m_sampleFactor, (float)y/m_sampleFactor, (float)z/m_sampleFactor, mat2));
    //return (ret*ret);
    return (ret);
}

double SizingFieldCreator::Gradval(const Volume *volume, double x, double y, double z, int mat1, int mat2, int n)
{
    double h=1e-3;
    switch(n)
    {
        case 0:
            if (Fval(volume, x+h,y ,z , mat1, mat2) < Fval(volume, x-h, y, z, mat1, mat2))
                return (Fval(volume,x+h,y,z,mat1,mat2) - Fval(volume,x,y,z,mat1,mat2))/h;
            else
                return (Fval(volume,x,y,z,mat1,mat2) - Fval(volume,x-h,y,z,mat1,mat2))/h;
            break;
        case 1:
            if (Fval(volume, x, y+h, z, mat1, mat2) < Fval(volume, x, y-h, z, mat1, mat2))
                return (Fval(volume,x,y+h,z,mat1,mat2) - Fval(volume,x,y,z,mat1,mat2))/h;
            else
                return (Fval(volume,x,y,z,mat1,mat2) - Fval(volume,x,y-h,z,mat1,mat2))/h;
            break;
        case 2:
            if (Fval(volume, x, y, z+h, mat1, mat2) < Fval(volume, x, y, z-h, mat1, mat2))
                return (Fval(volume,x,y,z+h,mat1,mat2) - Fval(volume,x,y,z,mat1,mat2))/h;
            else
            {
                double one = Fval(volume,x,y,z,mat1,mat2);
                double newz = z-h;
                double two = Fval(volume,x,y,newz,mat1,mat2);
                double ret = (one-two)/h;
                return ret;
            }
            break;
       default:
            return std::numeric_limits<double>::quiet_NaN();   // TODO(JRB) : Replace function with non-branching form.
    }
}

bool SizingFieldCreator::find_inv(vec3 hess[], vec3 *inv)
{
    //int i,j;
    //compute det
    double det;
    det = hess[0][0]*(hess[1][1]*hess[2][2] - hess[1][2]*hess[2][1]);
    det -= hess[0][1]*(hess[1][0]*hess[2][2] - hess[1][2]*hess[2][0]);
    det += hess[0][2]*(hess[1][0]*hess[2][1] - hess[2][0]*hess[1][1]);
    if(fabs(det)<1e-10)
            return false;

    //compute cofactor matrix
    inv[0][0] =  (hess[1][1]*hess[2][2] - hess[1][2]*hess[2][1])/det;
    inv[1][0] = -(hess[1][0]*hess[2][2] - hess[1][2]*hess[2][0])/det;
    inv[2][0] =  (hess[1][0]*hess[2][1] - hess[1][1]*hess[2][0])/det;

    inv[0][1] = -(hess[0][1]*hess[2][2] - hess[2][1]*hess[0][2])/det;
    inv[1][1] =  (hess[0][0]*hess[2][2] - hess[2][0]*hess[0][2])/det;
    inv[2][1] = -(hess[0][0]*hess[2][1] - hess[2][0]*hess[0][1])/det;

    inv[0][2] =  (hess[0][1]*hess[1][2] - hess[0][2]*hess[1][1])/det;
    inv[1][2] = -(hess[0][0]*hess[1][2] - hess[0][2]*hess[1][0])/det;
    inv[2][2] =  (hess[0][0]*hess[1][1] - hess[0][1]*hess[1][0])/det;

    return true;
}


double SizingFieldCreator::Gradval(double x, double y, double z, ScalarField<float> *myField, int n)
{
    double initVal, nextVal, myVal, h=0.25;
    int sign; //i,j,k,

    //for(i=0; i<3; i++)
    //{
        switch(n)
        {
            case 0:
                initVal = myField->valueAt(x+h,y,z);
                nextVal = myField->valueAt(x-h,y,z);
                if(initVal<nextVal)
                {
                    myVal = initVal;
                    sign = 1;
                }
                else
                {
                    myVal = nextVal;
                    sign = -1;
                }
                return (myVal - myField->valueAt(x,y,z))/(sign*h);
                break;
            case 1:
                initVal = myField->valueAt(x,y+h,z);
                nextVal = myField->valueAt(x,y-h,z);
                if(initVal<nextVal)
                {
                    myVal = initVal;
                    sign = 1;
                }
                else
                {
                    myVal = nextVal;
                    sign = -1;
                }
                return (myVal - myField->valueAt(x,y,z))/(sign*h);
                break;
            case 2:
                initVal = myField->valueAt(x,y,z+h);
                nextVal = myField->valueAt(x,y,z-h);
                if(initVal<nextVal)
                {
                    myVal = initVal;
                    sign = 1;
                }
                else
                {
                    myVal = nextVal;
                    sign = -1;
                }
                return (myVal - myField->valueAt(x,y,z))/(sign*h);
                break;
            default:
                return 0;
        }
    //}
}

double SizingFieldCreator::Newton(const Volume *volume, const Triple &vertex1, const Triple &vertex2, int mat1, int mat2, double &i_star, double &j_star, double &k_star)
{
    int i, /*j,*/ i1,j1,k1;
    double grad[3], /*temp[3],*/ h, ret;

    i1 = vertex1.index[0];
    j1 = vertex1.index[1];
    k1 = vertex1.index[2];
    h=0.5; //1e-3;

    vec3 gradient, hess[3], inv[3];

    i_star=i1; j_star=j1; k_star=k1;
    double norm = 1, step=1;

    double current_value = Fval(volume, i_star, j_star, k_star, mat1, mat2);
    //while(norm>1e-6 && step>1e-6)
    int no_iter=0;
    while(fabs(current_value)>1e-3 && no_iter<20)
    {
        no_iter++;
        for(i=0; i<3; i++)
            grad[i]= Gradval(volume, i_star, j_star, k_star, mat1, mat2, i);


        for(i=0; i<3; i++)
            gradient[i] = grad[i];

        norm = length(gradient);
        if(norm<=1e-10)
            break;

        i_star -= current_value*gradient[0]/(norm*norm);
        j_star -= current_value*gradient[1]/(norm*norm);
        k_star -= current_value*gradient[2]/(norm*norm);

        current_value = Fval(volume, i_star, j_star, k_star, mat1, mat2);

    }
    ret = (i_star-i1)*(i_star-i1) + (j_star-j1)*(j_star-j1) + (k_star-k1)*(k_star-k1);
    //if(ret>=3)
    //    cout<<i1<<" "<<j1<<" "<<k1<<" "<<sqrt(ret)<<endl;
    return sqrt(ret);
}

double SizingFieldCreator::trace(vec3 matrix[])
{
    double ret;
    ret = matrix[0][0]+matrix[1][1]+matrix[2][2];
    return (ret);
}

double SizingFieldCreator::fnorm(vec3 matrix[])
{
    int i,j;
    double ret =0;
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            ret += matrix[i][j]*matrix[i][j];
    ret = sqrt(ret);
    return ret;
}

void SizingFieldCreator::mult(vec3 *a, vec3 *b, vec3 *ret)
{
    int i,j,k;
    for(i=0; i<3; i++)
    {
        for(j=0; j<3; j++)
        {
            ret[i][j] = 0;
            for(k=0; k<3; k++)
            {
                ret[i][j]+=a[i][k]*b[k][j];
            }
        }
    }
}

double SizingFieldCreator::curvature(const Volume *volume, double x, double y, double z, int mat1, int mat2)
{
    vec3 gradient, hess[3], P[3], G[3], temp;
    double norm, T, F, ret, h=1;
    int i,j;//,k;

    for(i=0; i<3; i++)
        gradient[i]= Gradval(volume, x, y, z, mat1, mat2, i);
    norm = length(gradient);
    for(i=0; i<3; i++)
        gradient[i]/=norm;



    for(i=0; i<3; i++)
    {
        if (Fval(volume, x+h, y, z, mat1, mat2) < Fval(volume, x-h, y, z, mat1, mat2))
            temp[0] = (Gradval (volume, x+h, y, z, mat1, mat2, i) - Gradval (volume, x, y, z, mat1, mat2, i))/h;
        else
            temp[0] = (Gradval (volume, x, y, z, mat1, mat2, i) - Gradval (volume, x-h, y, z, mat1, mat2, i))/h;

        if (Fval(volume, x, y+h, z, mat1, mat2) < Fval(volume, x, y-h, z, mat1, mat2))
            temp[1] = (Gradval (volume, x, y+h, z, mat1, mat2, i) - Gradval (volume, x, y, z, mat1, mat2, i))/h;
        else
            temp[1] = (Gradval (volume, x, y, z, mat1, mat2, i) - Gradval (volume, x, y-h, z, mat1, mat2, i))/h;

        if (Fval(volume, x, y, z+h, mat1, mat2) < Fval(volume, x, y, z-h, mat1, mat2))
            temp[2] = (Gradval (volume, x, y, z+h, mat1, mat2, i) - Gradval (volume, x, y, z, mat1, mat2, i))/h;
        else
            temp[2] = (Gradval (volume, x, y, z, mat1, mat2, i) - Gradval (volume, x, y, z-h, mat1, mat2, i))/h;
        for(j=0; j<3; j++)
            hess[i][j] = temp[j];
    }

    //P = I - nn'
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            P[i][j] = -gradient[i]*gradient[j];
    for(i=0; i<3; i++)
        P[i][i] += 1.0;

    //G = -PHP/|g|
    vec3 dummy[3];
    mult(hess, P, dummy);
    mult(P, dummy, G);
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            G[i][j]*=(-1.0/norm);

    //computing curvatrue
    T = trace(G); F = fnorm(G);
    ret = 2/(T + sqrt(2*F*F - T*T));
    return fabs(ret);
}


void SizingFieldCreator::appendPadding(const vec3 &mypadding, const vec3 &myoffset, vector<Triple> &zeros)
{
    int full_w, full_h, full_d;

    int w = mesh_feature.dist.size();
    int h = mesh_feature.dist[0].size();
    int d = mesh_feature.dist[0][0].size();

    full_w = w + (int)mypadding[0];
    full_h = h + (int)mypadding[1];
    full_d = d + (int)mypadding[2];
    mesh_padded_feature.init(full_w, full_h, full_d);

    for(int i=0; i<full_w; i++)
        for(int j=0; j<full_h; j++)
            for(int k=0; k<full_d; k++)
                mesh_padded_feature.known[i][j][k] = false;

    int x_offset = (int)myoffset[0];
    int y_offset = (int)myoffset[1];
    int z_offset = (int)myoffset[2];

    for(int i=0; i<w; i++)
        for(int j=0; j<h; j++)
            for(int k=0; k<d; k++)
            {
                mesh_padded_feature.dist[i + x_offset][j + y_offset][k + z_offset]
                        = mesh_feature.dist[i][j][k];
                mesh_padded_feature.known[i + x_offset][j + y_offset][k + z_offset]
                        = mesh_feature.known[i][j][k];
            }

    for(size_t i=0; i<zeros.size(); i++)
    {
        for(int j=0; j<3; j++)
        {
            zeros[i].index[j] += (int)myoffset[j];
        }
    }
    return;

}

ScalarField<float>* SizingFieldCreator::createSizingFieldFromVolume(const Volume *volume, float speed, float sampleFactor, float sizingFactor, int padding, bool adaptiveSurface, bool verbose)
{
    if(verbose)
        std::cout << "Creating sizing field at " << sampleFactor << "x resolution, with "
                  << "Lipschitz=" << speed
                  << ", sizingFactor=" << sizingFactor
                  << ", padding=" << padding
                  << ", adaptive=" << adaptiveSurface
                  << std::endl;

    SizingFieldCreator fieldCreator(volume, speed, sampleFactor, sizingFactor, padding, adaptiveSurface, verbose);

    if (verbose)
        std::cout << "Sizing Field Creating! Returning it.." << std::endl;

    return fieldCreator.getField();
}



}
