#ifndef CLEAVERWIDGET_H
#define CLEAVERWIDGET_H

#include <QDockWidget>
#include <QMdiSubWindow>
#include <Cleaver/CleaverMesher.h>
#include "ViewWidgets/MeshWindow.h"

namespace Ui {
class CleaverWidget;
}

class CleaverWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit CleaverWidget(QWidget *parent = 0);
    ~CleaverWidget();

public slots:
    void clear();
    void update();
    void focus(QMdiSubWindow *);

    //------ run entire cleaving algorithm
    void createMesh();
    //------ advanced indididual calls
    void createBackgroundMesh(MeshWindow * window);
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
  void updateOpenGL(MeshWindow * window);
    
private:
    Ui::CleaverWidget *ui;
    cleaver::CleaverMesher *mesher;
};

#endif // CLEAVERWIDGET_H
