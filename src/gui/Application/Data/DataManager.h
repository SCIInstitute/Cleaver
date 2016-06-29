#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <Cleaver/TetMesh.h>
#include <Cleaver/Volume.h>
#include <QObject>
#include <stdlib.h>

class DataManager : public QObject
{
    Q_OBJECT

public:
    DataManager();
    ~DataManager();

    void setMesh(cleaver::TetMesh *mesh);
    void setSizingField(cleaver::AbstractScalarField *field);
    void setIndicators(std::vector<cleaver::AbstractScalarField *> indicators);
    void setVolume(cleaver::Volume *volume);

    cleaver::AbstractScalarField* sizingField() const;
    std::vector<cleaver::AbstractScalarField*> indicators() const;
    cleaver::Volume*              volume() const;
    cleaver::TetMesh*             mesh() const;


private:
    cleaver::TetMesh*              mesh_;
    cleaver::Volume*               volume_;
    cleaver::AbstractScalarField*  sizingField_;
    std::vector<cleaver::AbstractScalarField*>  indicators_;
};

#endif // DATAMANAGER_H
