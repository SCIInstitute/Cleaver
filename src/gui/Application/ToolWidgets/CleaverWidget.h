#ifndef CLEAVERWIDGET_H
#define CLEAVERWIDGET_H

#include <QDockWidget>
#include <Cleaver/CleaverMesher.h>

namespace Ui {
class CleaverWidget;
}

class CleaverWidget : public QDockWidget
{
    Q_OBJECT
    
public:
  explicit CleaverWidget(cleaver::CleaverMesher& mesher,
    QWidget *parent = NULL);
    ~CleaverWidget();
    void setMeshButtonEnabled(bool b);
  signals:
    void doneMeshing();
    void repaintGL();
    void newMesh();
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
    void resetCheckboxes();
    
private:
    Ui::CleaverWidget *ui;
    cleaver::CleaverMesher& mesher_;
};

#endif // CLEAVERWIDGET_H
