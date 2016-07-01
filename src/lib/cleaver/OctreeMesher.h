#ifndef OCTREEMESHER_H
#define OCTREEMESHER_H

#include "AbstractScalarField.h"
#include "TetMesh.h"

namespace cleaver {

class OctreeMesherImp;

class OctreeMesher
{
public:
    OctreeMesher(const cleaver::AbstractScalarField *sizing_field = nullptr);
    ~OctreeMesher();

    void setSizingField(const cleaver::AbstractScalarField *sizing_field);

    void createMesh();

    cleaver::TetMesh* getMesh();

private:

    OctreeMesherImp *m_pimpl;
};

}

#endif // OCTREEMESHER_H
