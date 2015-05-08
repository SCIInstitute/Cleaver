#include "CleaverWidget.h"
#include "ui_CleaverWidget.h"
#include "MainWindow.h"
#include "ViewWidgets/MeshWindow.h"
#include <Cleaver/TetMesh.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/Timer.h>
#include <Cleaver/SizingFieldCreator.h>
#include <iostream>
#ifdef USING_QT5
#include <QApplication>
#include <QProgressDialog>
#else
#include <QtGui/QApplication>
#endif


CleaverWidget::CleaverWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CleaverWidget)
{
    ui->setupUi(this);

    QObject::connect(MainWindow::dataManager(), SIGNAL(volumeListChanged()), this, SLOT(updateVolumeList()));
}

CleaverWidget::~CleaverWidget()
{
    delete ui;
}


//========================
//     Public Slots
//========================


void CleaverWidget::focus(QMdiSubWindow* subwindow)
{
    if (subwindow != NULL){

        MeshWindow *window = qobject_cast<MeshWindow *>(subwindow->widget());

        if(window != NULL)
        {
            //std::cout << "Cleaver Widget has Volume from Window" << std::endl;
            mesher = window->mesher();
        }
        else
        {
            mesher = NULL;
        }

        update();
    }
}


void CleaverWidget::clear()
{
    //
}


void CleaverWidget::resetCheckboxes() {
    
    this->ui->backgroundCheck->setChecked(false);
    this->ui->adjacencyCheck->setChecked(false);
    this->ui->sampleCheck->setChecked(false);
    this->ui->alphaCheck->setChecked(false);
    this->ui->interfaceCheck->setChecked(false);
    this->ui->generalizeCheck->setChecked(false);
    this->ui->snapCheck->setChecked(false);
    this->ui->stencilCheck->setChecked(false);
}

//=========================================
// - update()       Updates the Widget
//
//=========================================
void CleaverWidget::update()
{
    //-----------------------------
    // Set Main Mesh Button
    //-----------------------------
    if(mesher && !mesher->backgroundMeshCreated()) {
      ui->createMeshButton->setEnabled(true);
      ui->createRegularMeshButton->setEnabled(true);
    } else {
        ui->createMeshButton->setEnabled(false);
        ui->createRegularMeshButton->setEnabled(false);
    }

    /*
    //-----------------------
    //  Set Stuffing Button
    //-----------------------
    if(mesher->backgroundMesh() && !mesher->completed())
        ui->stuffButton->setEnabled(true);
    else
        ui->stuffButton->setEnabled(false);

    // allow early stenciling if generalization done
    if(mesher->generalized())
        ui->stencilButton->setEnabled(true);

    QDockWidget::update();
    */

    QDockWidget::update();
}


//=========================================
// - createMesh()
//=========================================
void CleaverWidget::createMesh()
{
    MeshWindow *window = MainWindow::instance()->activeWindow();
    if(window != NULL){
		QProgressDialog status(QString("Meshing Indicator Functions..."),QString(),0,100, this);
		status.show();
		status.setWindowModality(Qt::WindowModal);
        mesher->setRegular(false);
        mesher->createBackgroundMesh(true);
		status.setValue(10);
        this->ui->backgroundCheck->setChecked(true);
        qApp->processEvents();
        window->setMesh(mesher->getBackgroundMesh());

        mesher->buildAdjacency(true);
		status.setValue(20);
        this->ui->adjacencyCheck->setChecked(true);
        qApp->processEvents();
        this->ui->adjacencyCheck->repaint();
        mesher->sampleVolume(true);
		status.setValue(30);
        this->ui->sampleCheck->setChecked(true);
        qApp->processEvents();
        window->updateMesh();
        window->updateGL();

        mesher->computeAlphas(true);
		status.setValue(40);
        this->ui->alphaCheck->setChecked(true);
        qApp->processEvents();
        mesher->computeInterfaces(true);
		status.setValue(50);
        this->ui->interfaceCheck->setChecked(true);
        qApp->processEvents();
        mesher->generalizeTets(true);
		status.setValue(60);
        this->ui->generalizeCheck->setChecked(true);
        qApp->processEvents();
        mesher->snapsAndWarp(true);
		status.setValue(70);
        this->ui->snapCheck->setChecked(true);
        qApp->processEvents();

        window->updateMesh();
        window->updateGL();

        mesher->stencilTets(true);
		status.setValue(80);
		if (this->ui->fixJacobianCheckBox->isChecked())
			mesher->fixVertexWindup(true);
		status.setValue(90);
        this->ui->stencilCheck->setChecked(true);
        qApp->processEvents();
        window->updateMesh();
        window->updateGL();

        update();
		MainWindow::instance()->enableMeshedVolumeOptions();
		status.setValue(100);
    }
}

//=========================================
// - createRegularMesh()
//=========================================
void CleaverWidget::createRegularMesh()
{
      MeshWindow *window = MainWindow::instance()->activeWindow();

    if(window != NULL){
		QProgressDialog status(QString("Meshing Indicator Functions..."),QString(),0,100, this);
		status.show();
		status.setWindowModality(Qt::WindowModal);
        //create the default sizing field
        cleaver::Volume *volume = window->volume();

          float scale = ui->scalingSpinner->value();
          int padding = ui->paddingSpinner->value();

        cleaver::Timer timer;
        timer.start();
        cleaver::AbstractScalarField *sizingField =
            cleaver::SizingFieldCreator::createSizingFieldFromVolume(
                volume, 1.0, scale, 1.0, padding, false, true);
        timer.stop();

        std::string sizingFieldName = volume->name() + "-computed-sizing-field";
        sizingField->setName(sizingFieldName);
        volume->setSizingField(sizingField);
        this->mesher->setSizingFieldTime(timer.time());

        // Add new sizing field to data manager
        MainWindow::dataManager()->addField(sizingField);
        //update the mesher
        double al = ui->alphaLongSpinner->value();
        double as = ui->alphaShortSpinner->value();
        mesher->setAlphas(al,as);
		status.setValue(5);
        mesher->setRegular(true);
		status.setValue(10);
        mesher->createBackgroundMesh(true);
		status.setValue(15);
        this->ui->backgroundCheck->setChecked(true);
        qApp->processEvents();

        window->setMesh(mesher->getBackgroundMesh());
        
        mesher->buildAdjacency(true);
		status.setValue(20);
        this->ui->adjacencyCheck->setChecked(true);
        qApp->processEvents();
        mesher->sampleVolume(true);
		status.setValue(30);
        this->ui->sampleCheck->setChecked(true);
        qApp->processEvents();
        window->updateMesh();
        window->updateGL();
        
        mesher->computeAlphas(true);
		status.setValue(40);
        this->ui->alphaCheck->setChecked(true);
        qApp->processEvents();
        mesher->computeInterfaces(true);
		status.setValue(50);
        this->ui->interfaceCheck->setChecked(true);
        qApp->processEvents();
        mesher->generalizeTets(true);
		status.setValue(60);
        this->ui->generalizeCheck->setChecked(true);
        qApp->processEvents();
        mesher->snapsAndWarp(true);
		status.setValue(70);
        this->ui->snapCheck->setChecked(true);
        qApp->processEvents();

        window->updateMesh();
        window->updateGL();

        mesher->stencilTets(true);
		status.setValue(80);
		if (this->ui->fixJacobianCheckBox->isChecked())
			mesher->fixVertexWindup(true);
		status.setValue(90);
        this->ui->stencilCheck->setChecked(true);
        qApp->processEvents();
        window->updateMesh();
        window->updateGL();

        update();
		MainWindow::instance()->enableMeshedVolumeOptions();
		status.setValue(100);
    }
}
//=========================================
// - createBackgroundMesh()
//
//=========================================
void CleaverWidget::createBackgroundMesh()
{
    MeshWindow *window = MainWindow::instance()->activeWindow();
    if(window != NULL){
        if (!mesher->backgroundMeshCreated()){
            //Cleaver::TetMesh *mesh = Cleaver::createMeshFromVolume(window->volume());
            mesher->createBackgroundMesh();
            cleaver::TetMesh *mesh = mesher->getBackgroundMesh();

            double total_volume = 0;
            int positive = 0;
            int negative = 0;
            for(size_t t=0; t < mesh->tets.size(); t++){
                double volume = mesh->tets[t]->volume();
                if(volume < 0)
                    negative++;
                else if(volume > 0)
                    positive++;

                total_volume += volume;
            }
            mesher->buildAdjacency();            

            mesh->name = "Adaptive-BCC-Mesh";
            MainWindow::dataManager()->addMesh(mesh);

            window->setMesh(mesh);
        }

        //mesh->writeNodeEle("debug",true);
        update();
    }
}


//=========================================
// - buildMeshAdjacency()
//
//=========================================
void CleaverWidget::buildMeshAdjacency()
{
    mesher->buildAdjacency();
    update();
}

//=========================================
// - sampleData()
//
//=========================================
void CleaverWidget::sampleVolume()
{
    mesher->sampleVolume();
    MainWindow::instance()->activeWindow()->updateMesh();
    MainWindow::instance()->activeWindow()->updateGL();
    update();
}


//=========================================
// - computeAlphas();
//
//=========================================
void CleaverWidget::computeAlphas()
{
    mesher->computeAlphas();
    update();
}

//=========================================
// - computeCuts()
//
//=========================================
void CleaverWidget::computeInterfaces()
{
    mesher->computeInterfaces();
    MainWindow::instance()->activeWindow()->updateMesh();
    MainWindow::instance()->activeWindow()->updateGL();
    update();
}

//=========================================
// - generalizeTets()
//
//=========================================
void CleaverWidget::generalizeTets()
{
    mesher->generalizeTets();
    update();
}

//=========================================
// - snapAndWarp()
//
//=========================================
void CleaverWidget::snapAndWarp()
{
    mesher->snapsAndWarp();
    MainWindow::instance()->activeWindow()->updateMesh();
    MainWindow::instance()->activeWindow()->updateGL();
    update();
}

//=========================================
// - stencil()
//
//=========================================
void CleaverWidget::stencilTets()
{
    mesher->stencilTets();
    MainWindow::instance()->activeWindow()->updateMesh();
    MainWindow::instance()->activeWindow()->updateGL();
    update();
    mesher->getTetMesh()->writeNodeEle("output",true);
    mesher->getTetMesh()->writePly("output", true);
    //mesher->getTetMesh()->writeMatlab("mesh", true);
}

void CleaverWidget::updateVolumeList()
{
    ui->volumeBox->clear();
    ui->regularVolumeBox->clear();

    std::vector<cleaver::Volume*> volumes = MainWindow::dataManager()->volumes();

    for(size_t i=0; i < volumes.size(); i++)
    {
        ui->volumeBox->addItem(volumes[i]->name().c_str());
        ui->regularVolumeBox->addItem(volumes[i]->name().c_str());
    }
}

void CleaverWidget::updateMeshList()
{

}

void CleaverWidget::volumeSelected(int index)
{

}

void CleaverWidget::meshSelected(int index)
{

}

