
#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include <Cleaver/Cleaver.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/Volume.h>

class MockupField : public cleaver::AbstractScalarField
{
public:
	MockupField(const cleaver::vec3& center)
		: m_Center(center)
		, m_Radius(0.2f)
		, m_Box(cleaver::vec3::zero, cleaver::vec3(1,1,1))
	{}

	virtual double valueAt( double x, double y, double z ) const override
	{
		x -= m_Center[0];
		y -= m_Center[1];
		z -= m_Center[2];

		double d = std::sqrt(x*x + y*y + z*z);
		return (m_Radius-d);
	}

	virtual cleaver::BoundingBox bounds() const override
	{
		return m_Box;
	}

private:
	cleaver::vec3 m_Center;
	float m_Radius;
	cleaver::BoundingBox m_Box;
};

class ConstantField : public cleaver::AbstractScalarField
{
public:
	ConstantField(double constant)
		: m_Constant(constant)
		, m_Box(cleaver::vec3::zero, cleaver::vec3(1,1,1)) // don't like this ...
	{}

	virtual double valueAt( double x, double y, double z ) const override { return m_Constant; }

	virtual cleaver::BoundingBox bounds() const override { return m_Box; }

private:
	double m_Constant;
	cleaver::BoundingBox m_Box;
};


const std::string scirun = "scirun";
const std::string tetgen = "tetgen";
const std::string matlab = "matlab";
const std::string vtk = "vtk";

std::vector<std::string> inputs;
std::string output = "output";
std::string format = vtk;
bool verbose = true;
bool absolute_resolution = false;
bool scaled_resolution = false;
int rx,ry,rz;
float sx,sy,sz;


int main(int argc, char *argv[])
{
    //-------------------------------
    //  Define volume
    //-------------------------------
    std::vector<cleaver::AbstractScalarField*> fields;// = loadNRRDFiles(inputs, verbose);
	fields.push_back( new MockupField(cleaver::vec3(0.5f,0.5f,0.5f)) );
	fields.push_back( new MockupField(cleaver::vec3(0.4f,0.4f,0.4f)) );
	fields.push_back( new MockupField(cleaver::vec3(0.3f,0.3f,0.6f)) );
	fields.push_back( new ConstantField(0.f) );

	scaled_resolution = true;
	sx=sy=sz = 45;

    if(fields.empty())
	{
        std::cerr << "Failed to load image data. Terminating." << std::endl;
        return 0;
    }
    else if(fields.size() == 1)
	{
        fields.push_back(new cleaver::InverseScalarField(fields[0]));
	}

    cleaver::Volume *volume = new cleaver::Volume(fields);


    if(absolute_resolution)
        ((cleaver::Volume*)volume)->setSize(rx,ry,rz);
    if(scaled_resolution)
        ((cleaver::Volume*)volume)->setSize(sx*volume->size().x,
                                            sy*volume->size().y,
                                            sz*volume->size().z);

    std::cout << "Creating Mesh with Volume Size " << volume->size().toString() << std::endl;

    //--------------------------------
    //  Create Mesher & TetMesh
    //--------------------------------
    cleaver::TetMesh *mesh = cleaver::createMeshFromVolume(volume, verbose);
	if( !mesh ) 
	{
		for (size_t i=0; i<fields.size(); i++)
			delete fields[i];
		return 1;
	}

    //------------------
    //  Compute Angles
    //------------------
    mesh->computeAngles();
    if(verbose){
        std::cout.precision(12);
        std::cout << "Worst Angles:" << std::endl;
        std::cout << "min: " << mesh->min_angle << std::endl;
        std::cout << "max: " << mesh->max_angle << std::endl;
    }

    //----------------------
    //  Write Info File
    //----------------------
    mesh->writeInfo(output, verbose);

    //----------------------
    // Write Tet Mesh Files
    //----------------------
    if(format == tetgen)
        mesh->writeNodeEle(output, verbose);
    else if(format == scirun)
        mesh->writePtsEle(output, verbose);
    else if(format == matlab)
        mesh->writeMatlab(output, verbose);
    else if(format == vtk)
        mesh->writeVtkUnstructuredGrid(output, verbose);

    //----------------------
    // Write Surface Files
    //----------------------
    mesh->constructFaces();
    mesh->writePly(output, verbose);


    //-----------
    // Cleanup
    //-----------
    if(verbose)
        std::cout << "Cleaning up." << std::endl;
    delete mesh;
    for(unsigned int f=0; f < fields.size(); f++)
        delete fields[f];
    delete volume;

    //-----------
    //  Done
    //-----------
    if(verbose)
        std::cout << "Done." << std::endl;

    return 0;
}
