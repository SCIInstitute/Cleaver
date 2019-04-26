#include <NRRDTools.h>

#define TEEM_STATIC 1  // needed for nrrd to be linked properly
#include <teem/nrrd.h>
#include <iostream>
#include <fstream>
#include <cleaver/ScalarField.h>
#include <cleaver/Status.h>

using namespace cleaver;
using namespace std;

const float REALLY_SMALL = -10000;
const float REALLY_LARGE = +10000;

std::string condensed(const std::string &filename)
{
    return filename.substr(filename.find_last_of("/") + 1);
}

// Provide no-op implementation for unsupported method.
std::vector<cleaver::AbstractScalarField*>

NRRDTools::segmentationToIndicatorFunctions(std::string file, double sigma)
{
  cerr << "NRRD file read error: Cleaver2 does not currently support segmentation files with teem. Please use ITK." << endl;";
    return {};
}

AbstractScalarField* loadNRRDFile(const std::string &filename, bool verbose)
{
    //--------------------------------------------
    //  Read header
    //-------------------------------------------

    // create empty nrrd file container
    Nrrd* nin = nrrdNew();

    // load only the header, not the data
    NrrdIoState *nio = nrrdIoStateNew();
    nrrdIoStateSet(nio, nrrdIoStateSkipData, AIR_TRUE);

    // Read in the Header
    if(verbose)
        std::cout << "Reading File: " << filename << std::endl;
    if(nrrdLoad(nin, filename.c_str(), nio))
    {
        char *err = biffGetDone(NRRD);
        cerr << "Trouble Reading File: " << filename << " : " << err << endl;
        free(err);
        nio = nrrdIoStateNix(nio);
        return NULL;
    }

    if(verbose)
        cout << "Reading NRRD of type: " << airEnumStr(nrrdType, nin->type) << endl;

    // Done with nrrdIoState
    nio = nrrdIoStateNix(nio);

    if(nin->dim != 3)
    {
        std::cerr << "Fatal Error: volume dimension " << nin->dim << ", expected 3." << std::endl;
        return NULL;
    }

    //-----------------------------------
    //       Account For Padding
    //-----------------------------------
    int w = nin->axis[0].size;
    int h = nin->axis[1].size;
    int d = nin->axis[2].size;

    //---------------------------------------
    //     Allocate Sufficient Data
    //---------------------------------------
    float *data = new float[w * h * d];
    AbstractScalarField *field = new FloatField(data,w,h,d);

    //----------------------------------------
    //  Deferred  Data Load/Copy
    //----------------------------------------
    if(nrrdLoad(nin, filename.c_str(), NULL))
    {
        char *err = biffGetDone(NRRD);
        cerr << "trouble reading data in file: " << filename << " : " << err << endl;
        free(err);
        return NULL;
    }


    float (*lup)(const void *, size_t I);
    lup = nrrdFLookup[nin->type];

    // cast and copy into float array
    int s=0;
    for(int k=0; k < d; k++){
        for(int j=0; j < h; j++){
            for(int i=0; i < w; i++){
                ((FloatField*)field)->data()[i + j*w + k*w*h] = lup(nin->data, s++);
            }
        }
    }

    // set scale
    double xs = ((Nrrd*)nin)->axis[0].spacing;
    double ys = ((Nrrd*)nin)->axis[1].spacing;
    double zs = ((Nrrd*)nin)->axis[2].spacing;

    // handle NaN cases
    if(xs != xs) xs = 1;
    if(ys != ys) ys = 1;
    if(zs != zs) zs = 1;

    ((FloatField*)field)->setScale(vec3(xs,ys,zs));

    // free local copy
    nrrdNuke(nin);

    //----------------------------------------
    // Create and return ScalarField
    //----------------------------------------
    return field;
}

std::vector<AbstractScalarField*>
NRRDTools::loadNRRDFiles(std::vector<std::string> filenames, double sigma)
{
    bool verbose = false;
    bool pad = false;

    //--------------------------------------------
    //  Read only headers of each file
    //-------------------------------------------
    vector<Nrrd*> nins;
    for(size_t i=0; i < filenames.size(); i++)
    {
        // create empty nrrd file container
        nins.push_back(nrrdNew());

        // load only the header, not the data
        NrrdIoState *nio = nrrdIoStateNew();
        nrrdIoStateSet(nio, nrrdIoStateSkipData, AIR_TRUE);

        // Read in the Header
        if(verbose)
            std::cout << "Reading File: " << filenames[i] << std::endl;
        if(nrrdLoad(nins[i], filenames[i].c_str(), nio))
        {
            char *err = biffGetDone(NRRD);
            cerr << "Trouble Reading File: " << filenames[i] << " : " << err << endl;
            free(err);
            nio = nrrdIoStateNix(nio);
            continue;
        }

        // Done with nrrdIoState
        nio = nrrdIoStateNix(nio);

        if(nins[i]->dim != 3)
        {
            cerr << "Fatal Error: volume dimension " << nins[i]->dim << ", expected 3." << endl;
            std::vector<AbstractScalarField*> empty_vector;
            return empty_vector;
        }
    }

    //-----------------------------------
    // Verify contents match
    //-----------------------------------
    bool match = true;

    for(unsigned i=1; i < filenames.size(); i++)
    {
        if(nins[i]->dim != nins[0]->dim){
            cerr << "Error, file " << i << " # dims don't match." << endl;
            match = false;
        }

        for(unsigned int j=0; j < nins[0]->dim-1; j++)
            if(nins[i]->axis[j].size != nins[0]->axis[j].size)
            {
                cerr << "Error, file " << j << " dimensions don't match." << endl;
                match = false;
                break;
            }

        if((int)nrrdElementNumber(nins[i]) != (int)nrrdElementNumber(nins[0]))
        {
            cerr << "Error, file " << i << " does not contain expected number of elements" << endl;
            cerr << "Expected: " << (int)nrrdElementNumber(nins[0]) << endl;
            cerr << "Found:    " << (int)nrrdElementNumber(nins[i]) << endl;

            match = false;
            break;
        }
    }

    if(!match)
    {
        std::vector<AbstractScalarField*> empty_vector;
        return empty_vector;
    }


    //-----------------------------------
    //       Account For Padding
    //-----------------------------------
    int w = nins[0]->axis[0].size;
    int h = nins[0]->axis[1].size;
    int d = nins[0]->axis[2].size;
    int m = filenames.size();
    int p = 0;

    if(verbose)
        std::cout << "Input Dimensions: " << w << " x " << h << " x " << d << std::endl;

    if(pad){
        p = 2;

        // correct for new grid size
        w += 2*p;
        h += 2*p;
        d += 2*p;

        // add material for 'outside'
        m++;

        if(verbose)
            std::cout << "Padded to: " << w << " x " << h << " x " << d << std::endl;
    }

    //---------------------------------------
    //     Allocate Sufficient Data
    //---------------------------------------
    std::vector<AbstractScalarField*> fields;

    for(unsigned int f=0; f < filenames.size(); f++)
    {
        float *data = new float[w * h * d];
        fields.push_back(new FloatField(data,w,h,d));
        fields[f]->setName(condensed(filenames[f]));
    }

    if(pad)
    {
        float *data = new float[w * h * d];
        fields.push_back(new FloatField(data,w,h,d));
        fields[fields.size()-1]->setName("Padding");
    }


    //----------------------------------------
    //  Deferred  Data Load/Copy
    //----------------------------------------
    if (verbose) std::cout << "Loading into memory..." << std::endl;
    Status status(filenames.size() * (d-2*p) * (w-2*p) * (h-2*p));
    for(unsigned int f=0; f < filenames.size(); f++)
    {
        // load nrrd data, reading memory into data array
        if(nrrdLoad(nins[f], filenames[f].c_str(), NULL))
        {
            char *err = biffGetDone(NRRD);
            cerr << "trouble reading data in file: " << filenames[f] << " : " << err << endl;
            free(err);
            std::vector<AbstractScalarField*> empty_vector;
            return empty_vector;
        }

        float (*lup)(const void *, size_t I);
        lup = nrrdFLookup[nins[f]->type];

        // cast and copy into large array
        int s=0;
        for(int k=p; k < d-p; k++){
            for(int j=p; j < h-p; j++){
                for(int i=p; i < w-p; i++){
                    ((FloatField*)fields[f])->data()[i + j*w + k*w*h] = lup(nins[f]->data, s++);
                    if (verbose)
                      status.printStatus();
                }
            }
        }

        // set scale
        double xs = ((Nrrd*)nins[f])->axis[0].spacing;
        double ys = ((Nrrd*)nins[f])->axis[1].spacing;
        double zs = ((Nrrd*)nins[f])->axis[2].spacing;

        // handle NaN cases
        if(xs != xs) xs = 1;
        if(ys != ys) ys = 1;
        if(zs != zs) zs = 1;

        ((FloatField*)fields[f])->setScale(vec3(xs,ys,zs));

        // free local copy
        nrrdNuke(nins[f]);
    }
    status.done();

    //----------------------------------------------
    //      Copy Boundary Values If Necessary
    //----------------------------------------------
    if(pad)
    {
      status = Status(d * w * h);
      if(verbose) std::cout << "Adjust for padding..."  << std::endl;
        // set 'outside' material small everywhere except exterior
        for(int k=0; k < d; k++){
            for(int j=0; j < h; j++){
                for(int i=0; i < w; i++){

                    // is distance to edge less than padding
                    if(k < p || k >= (d - p) ||
                       j < p || j >= (h - p) ||
                       i < p || i >= (w - p))
                    {
                        // set all other materials really small
                        for(unsigned int f=0; f < filenames.size(); f++)
                            ((FloatField*)fields[f])->data()[i + j*w + k*w*h] = REALLY_SMALL;

                        // set 'outside' material really large
                        ((FloatField*)fields[m-1])->data()[i + j*w + k*w*h] = REALLY_LARGE;
                    }
                    // otherwise
                    else{
                        // set 'outside' material very small
                        ((FloatField*)fields[m-1])->data()[i + j*w + k*w*h] = REALLY_SMALL;
                    }
                    if (verbose) status.printStatus();
                }
            }
        }
        status.done();
    }

    return fields;
}


std::vector<cleaver::AbstractScalarField*> loadNRRDLabelMap(const std::string &filename, bool verbose = false)
{
    // load the data
    // scan it and count the number of labels.

    return std::vector<cleaver::AbstractScalarField*>();
}


void NRRDTools::saveNRRDFile(const cleaver::FloatField *field, const std::string &filename)
{
    std::string nrrdfilename = filename + std::string(".nrrd");
    std::ofstream nrrd_file(nrrdfilename.c_str(), ios::out | ios::binary);
    if(!nrrd_file.is_open())
    {
        std::cerr << "Problem opening " << filename << " to write." << std::endl;
        exit(0);
    }

    int w = (int) field->dataBounds().size.x;
    int h = (int) field->dataBounds().size.y;
    int d = (int) field->dataBounds().size.z;
    double origin_x = field->bounds().minCorner().x;
    double origin_y = field->bounds().minCorner().y;
    double origin_z = field->bounds().minCorner().z;

    nrrd_file << "NRRD0001" << std::endl;
    nrrd_file << "# Complete NRRD file format specification at:" << std::endl;
    nrrd_file << "# http://teem.sourceforge.net/nrrd/format.html" << std::endl;
    nrrd_file << "type: float" << std::endl;
    nrrd_file << "dimension: 3" << std::endl;
    nrrd_file << "sizes: " << w << " " << h << " " << d << std::endl;
    nrrd_file << "axis mins: " <<  origin_x << " " << origin_y << " " << origin_z << std::endl;
    nrrd_file << "spacings: " << field->scale().x << " " << field->scale().y << " " << field->scale().z << std::endl;
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
                float val = field->data()[k*(w*h) + j*w + i];
                nrrd_file.write((char*)&val, sizeof(float));
            }
        }
    }

    nrrd_file.close();
}
