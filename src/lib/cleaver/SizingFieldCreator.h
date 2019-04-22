//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
// -- SizingFieldCreator
//
//  Author: Shankar Sastry (sastry@sci.utah.edu)
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

#ifndef SIZINGFIELDCREATOR_H
#define SIZINGFIELDCREATOR_H


#include <cstdio>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <cmath>
#include <fstream>

#include "Volume.h"
#include "ScalarField.h"
#include "vec3.h"
#include "Octree.h"
#include "BoundingBox.h"


namespace cleaver
{

class Voxel
{
    public:
	int mat, x, y, z;
};

class VoxelMesh
{
public:
    VoxelMesh(bool verbose = false);
    ~VoxelMesh();

    VoxelMesh& operator=(const VoxelMesh &mesh);

    void init(int l, int m, int n);
    ScalarField<float>*  convertToFloatField(float factor,
      const cleaver::vec3 &padding, const cleaver::vec3 &offset);

    std::vector<std::vector<std::vector<bool> > > known;
    std::vector<std::vector<std::vector<double> > > dist;

    private:
    bool m_verbose;
};

class QueueIndex
{
    public:
	int index[3];
	double dist;
};

class Triple
{
    public:
	int index[3];
    bool operator<(const Triple &a)
	{
		if(index[0] < a.index[0] && index[1] < a.index[1] && index[2] < a.index[2])
			return true;
		else
			return false;
	}
    bool operator>(const Triple &a)
	{
		if(index[0] > a.index[0] && index[1] > a.index[1] && index[2] > a.index[2])
			return true;
		else
			return false;
	}
    bool operator<=(const Triple &a)
	{
		if(index[0] <= a.index[0] && index[1] <= a.index[1] && index[2] <= a.index[2])
			return true;
		else
			return false;
	}
    bool operator>=(const Triple &a)
	{
		if(index[0] >= a.index[0] && index[1] >= a.index[1] && index[2] >= a.index[2])
			return true;
		else
			return false;
	}
    bool operator==(const Triple &a)
	{
		if(index[0] == a.index[0] && index[1] == a.index[1] && index[2] == a.index[2])
			return true;
		else
			return false;
	}
    bool operator=(const Triple &a)
	{
		for(int i=0; i<3; i++)
			index[i]=a.index[i];
		return true;
	}
};

class FeatureOctant
{
    public:
	Triple a, b;
	FeatureOctant *child[8];
	double  min;
	void assign(int i, int j, int c, int d, int e, int f)
	{
		a.index[0]=i;
		a.index[1]=j;
		a.index[2]=c;
		b.index[0]=d;
		b.index[1]=e;
		b.index[2]=f;
	}
};


class SizingFieldCreator
{
    public:
    SizingFieldCreator(const Volume*, float speed = 1.0f,
      float sampleFactor = 2.0f, float sizingFactor = 1.0f,
      int padding = 0, bool adaptiveSurface=true, bool verbose=false);
    ~SizingFieldCreator();

    double valueAt(double x, double y, double z) const;
    double operator()(double x, double y, double z) const
    {
        return valueAt(x,y,z);
    }
    double valueAt(const vec3 &x) const
    {
        return valueAt(x.x,x.y,x.z);
    }
    ScalarField<float>* getField()
    {
        return mesh_padded_feature.convertToFloatField((float)m_sampleFactor, m_padding, m_offset);
    }

    static ScalarField<float>* createSizingFieldFromVolume(const Volume *volume,
      float speed = 1.0f, float sampleFactor = 2.0f, float sizingFactor = 1.0f,
      int m_padding = 0, bool featureSize=true, bool verbose=false);

    private:
    bool   m_verbose;
    double m_speed;
    double m_sampleFactor;
    double m_sizingFactor;

    double compute_size(VoxelMesh&, VoxelMesh&, FeatureOctant*, int);
    double search_size(VoxelMesh&, const Triple&, const Triple&, FeatureOctant*);
	bool exists(QueueIndex&, VoxelMesh&);
    void proceed(VoxelMesh&, std::vector<Triple>&, double, double);
    Triple make_triple(int, int, int);
    QueueIndex make_index(int i, int j, int k);
    double Fval(const Volume *volume, double x, double y, double z, int mat1, int mat2);
    double Gradval(const Volume *volume, double x, double y, double z, int mat1, int mat2, int n);
    double Newton(const Volume *volume, const Triple &vertex1, const Triple &vertex2, int mat1, int mat2, double &i_star, double &j_star, double &k_star);
    bool find_inv(vec3 hess[], vec3 *inv);
    double fnorm(vec3 matrix[]);
    void mult(vec3 *a, vec3 *b, vec3 *ret);
    double curvature(const Volume *volume, double x, double y, double z, int mat1, int mat2);
    double trace(vec3 matrix[]);
    void appendPadding(const vec3 &m_padding, const vec3 &m_offset, std::vector<Triple> &zeros);
    double Gradval(double x, double y, double z, ScalarField<float>* myField, int n);

    Octree* createOctree(const Volume *volume, ScalarField<float> *sizingField);
    double adaptCell(const Volume *volume, OTCell *cell, ScalarField<float> *sizingField);
    void printTree(OTCell *myCell, int n);

    VoxelMesh mesh_bdry, mesh_feature, mesh_padded_feature;
    vec3 m_padding, m_offset;
};



}

#endif // SIZINGFIELDCREATOR_H
