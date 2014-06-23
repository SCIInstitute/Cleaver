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

    void addMesh(cleaver::TetMesh *mesh);
    void removeMesh(cleaver::TetMesh *mesh);

    void addField(cleaver::AbstractScalarField *field);
    void removeField(cleaver::AbstractScalarField *field);

    void addVolume(cleaver::Volume *volume);
    void removeVolume(cleaver::Volume *volume);

    std::vector<ulong> getSelection();
    void setSelection(ulong);
    void addSelection(ulong);
    void toggleSetSelection(ulong);
    void toggleAddSelection(ulong);
    void clearSelection();

    void update(){ emit dataChanged(); }

    std::vector<cleaver::AbstractScalarField*>  fields() const { return m_fields; }
    std::vector<cleaver::Volume*>       volumes() const { return m_volumes; }
    std::vector<cleaver::TetMesh*>      meshes() const { return m_meshes; }

signals:

    void dataAdded();
    void dataRemoved();
    void dataChanged();

    void meshAdded();
    void meshRemoved();

    void fieldAdded();
    void fieldRemoved();

    void volumeAdded();
    void volumeRemoved();

    void meshListChanged();
    void fieldListChanged();
    void volumeListChanged();

    void selectionChanged();

private:

    std::vector<ulong>                          m_selection;
    std::vector<cleaver::TetMesh*>              m_meshes;
    std::vector<cleaver::Volume*>               m_volumes;
    std::vector<cleaver::AbstractScalarField*>  m_fields;
};

#endif // DATAMANAGER_H
