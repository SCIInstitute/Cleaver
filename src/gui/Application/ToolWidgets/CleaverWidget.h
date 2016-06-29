#ifndef CLEAVERWIDGET_H
#define CLEAVERWIDGET_H

#include <QDockWidget>
#include <Cleaver/CleaverMesher.h>
#include "ViewWidgets/MeshWindow.h"
#include "DataWidgets/DataManagerWidget.h"

namespace Ui {
class CleaverWidget;
}

class CleaverWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit CleaverWidget(QWidget *parent = NULL, 
      DataManagerWidget * data = NULL,
      MeshWindow * window = NULL);
    ~CleaverWidget();
    void setMeshButtonEnabled(bool b);
    cleaver::CleaverMesher * getMesher();

public slots:
    void clear();

    //------ run entire cleaving algorithm
    void createMesh();
    //------ advanced indididual calls
    void createBackgroundMesh();
    void buildMeshAdjacency();
    void sampleVolume();
    void computeAlphas();
    void computeInterfaces();
    void generalizeTets();
    void snapAndWarp();
    void stencilTets();

    void updateMeshList();
    void volumeSelected(int index);
    void meshSelected(int index);
    void resetCheckboxes();
private:
  void updateOpenGL();
    
private:
    Ui::CleaverWidget *ui;
    cleaver::CleaverMesher *mesher;
    DataManagerWidget * data_;
    MeshWindow * window_;
};

#endif // CLEAVERWIDGET_H
