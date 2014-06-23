#ifndef CLEAVERWIDGET_H
#define CLEAVERWIDGET_H

#include <QDockWidget>
#include <QMdiSubWindow>
#include <Cleaver/CleaverMesher.h>

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

    void topologyActionChanged();

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


    
private:
    Ui::CleaverWidget *ui;
    cleaver::CleaverMesher *mesher;
};

#endif // CLEAVERWIDGET_H
