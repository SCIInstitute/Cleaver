#include "CleaverWidget.h"
#include "ui_CleaverWidget.h"
#include "MainWindow.h"
#include <Cleaver/TetMesh.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/Timer.h>
#include <Cleaver/SizingFieldCreator.h>
#include <iostream>
#include <QApplication>
#include <QProgressDialog>

CleaverWidget::CleaverWidget(QWidget *parent) :
  QDockWidget(parent),
  ui(new Ui::CleaverWidget) {
  this->ui->setupUi(this);

  QObject::connect(MainWindow::dataManager(), SIGNAL(volumeListChanged()), this, SLOT(updateVolumeList()));
}

CleaverWidget::~CleaverWidget() {
  delete this->ui;
}

//========================
//     Public Slots
//========================

void CleaverWidget::focus(QMdiSubWindow* subwindow) {
  if (subwindow != NULL) {
    MeshWindow *window = qobject_cast<MeshWindow *>(subwindow->widget());
    if (window != NULL) {
      //std::cout << "Cleaver Widget has Volume from Window" << std::endl;
      this->mesher = window->mesher();
    } else {
      this->mesher = NULL;
    }
    this->update();
  }
}

void CleaverWidget::clear() {
  resetCheckboxes();
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
void CleaverWidget::update() {
  //-----------------------------
  // Set Main Mesh Button
  //-----------------------------
  if (this->mesher) {
    this->ui->createMeshButton->setEnabled(true);
  } else {
    this->ui->createMeshButton->setEnabled(false);
  }
  QDockWidget::update();
}

void CleaverWidget::updateOpenGL(MeshWindow * window) {
  window->updateMesh();
  window->updateGL();
}

//=========================================
// - createMesh()
//=========================================
void CleaverWidget::createMesh() {
  this->clear();
  MeshWindow *window = MainWindow::instance()->activeWindow();
  if (window != NULL) {
    //progress
    QProgressDialog status(QString("Meshing Indicator Functions..."),
      QString(), 0, 100, this);
    status.show();
    status.setWindowModality(Qt::WindowModal);
    status.setValue(5);
    qApp->processEvents();
    //create background mesh
    mesher->setRegular(false);
    this->createBackgroundMesh(window);
    status.setValue(10);
    qApp->processEvents();
    //adjacencies
    this->buildMeshAdjacency();
    status.setValue(20);
    qApp->processEvents();
    //sample vol
    this->sampleVolume();
    this->updateOpenGL(window);
    status.setValue(30);
    qApp->processEvents();
    //alphas
    this->computeAlphas();
    status.setValue(40);
    qApp->processEvents();
    //interfaces
    this->computeInterfaces();
    this->updateOpenGL(window);
    status.setValue(50);
    qApp->processEvents();
    //generalize
    this->generalizeTets();
    status.setValue(60);
    qApp->processEvents();
    //snap and warp
    this->snapAndWarp();
    this->updateOpenGL(window);
    status.setValue(70);
    qApp->processEvents();
    //stencil
    this->stencilTets();
    this->updateOpenGL(window);
    status.setValue(80);
    if (this->ui->fixJacobianCheckBox->isChecked()) {
      this->mesher->fixVertexWindup(true);
    }
    status.setValue(90);
    qApp->processEvents();
    MainWindow::instance()->enableMeshedVolumeOptions();
    status.setValue(100);
  }
}
//=========================================
// - createBackgroundMesh()
//
//=========================================
void CleaverWidget::createBackgroundMesh(MeshWindow *window) {
  this->mesher->createBackgroundMesh(true);
  cleaver::TetMesh *mesh = this->mesher->getBackgroundMesh();
  mesh->name = "Adaptive-BCC-Mesh";
  MainWindow::dataManager()->addMesh(mesh);
  window->setMesh(mesh);
  this->ui->backgroundCheck->setChecked(true);
  this->update();
}
//=========================================
// - buildMeshAdjacency()
//
//=========================================
void CleaverWidget::buildMeshAdjacency() {
  this->mesher->buildAdjacency(true);
  this->ui->adjacencyCheck->setChecked(true);
  this->update();
}
//=========================================
// - sampleData()
//
//=========================================
void CleaverWidget::sampleVolume() {
  this->mesher->sampleVolume(true);
  this->ui->sampleCheck->setChecked(true);
  this->update();
}
//=========================================
// - computeAlphas();
//
//=========================================
void CleaverWidget::computeAlphas() {
  this->mesher->computeAlphas();
  this->ui->alphaCheck->setChecked(true);
  this->update();
}
//=========================================
// - computeCuts()
//
//=========================================
void CleaverWidget::computeInterfaces() {
  this->mesher->computeInterfaces(true);
  this->ui->interfaceCheck->setChecked(true);
  this->update();
}
//=========================================
// - generalizeTets()
//
//=========================================
void CleaverWidget::generalizeTets() {
  this->mesher->generalizeTets();
  this->ui->generalizeCheck->setChecked(true);
  this->update();
}
//=========================================
// - snapAndWarp()
//
//=========================================
void CleaverWidget::snapAndWarp() {
  this->mesher->snapsAndWarp();
  this->ui->snapCheck->setChecked(true);
  this->update();
}
//=========================================
// - stencil()
//
//=========================================
void CleaverWidget::stencilTets() {
  this->mesher->stencilTets();
  this->ui->stencilCheck->setChecked(true);
  this->update();
}

void CleaverWidget::updateMeshList() {

}

void CleaverWidget::volumeSelected(int index) {

}

void CleaverWidget::meshSelected(int index) {

}

