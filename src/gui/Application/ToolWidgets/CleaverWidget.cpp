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

CleaverWidget::CleaverWidget(QWidget *parent, 
  DataManagerWidget* data, MeshWindow* window) :
  QDockWidget(parent),
  data_(data),
  window_(window),
  mesher(NULL),
  ui(new Ui::CleaverWidget) {
  this->ui->setupUi(this);
}

CleaverWidget::~CleaverWidget() {
  delete this->ui;
}

void CleaverWidget::setMeshButtonEnabled(bool b) {
  this->ui->createMeshButton->setEnabled(b);
}

cleaver::CleaverMesher * CleaverWidget::getMesher() {
  return this->mesher;
}

//========================
//     Public Slots
//========================

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

void CleaverWidget::updateOpenGL() {
  this->window_->updateMesh();
  this->window_->updateGL();
  this->window_->repaint();
}

//=========================================
// - createMesh()
//=========================================
void CleaverWidget::createMesh() {
  this->clear();
  //progress
  QProgressDialog status(QString("Meshing Indicator Functions..."),
    QString(), 0, 100, this);
  if (this->mesher != NULL) {
    delete this->mesher;
  }
  this->mesher = new cleaver::CleaverMesher(this->data_->getVolume());
  this->window_->setMesher(this->mesher);
  status.show();
  status.setWindowModality(Qt::WindowModal);
  status.setValue(5);
  qApp->processEvents();
  //create background mesh
  mesher->setRegular(false);
  this->createBackgroundMesh();
  status.setValue(10);
  qApp->processEvents();
  //adjacencies
  this->buildMeshAdjacency();
  status.setValue(20);
  qApp->processEvents();
  //sample vol
  this->sampleVolume();
  this->updateOpenGL();
  status.setValue(30);
  qApp->processEvents();
  //alphas
  this->computeAlphas();
  status.setValue(40);
  qApp->processEvents();
  //interfaces
  this->computeInterfaces();
  this->updateOpenGL();
  status.setValue(50);
  qApp->processEvents();
  //generalize
  this->generalizeTets();
  status.setValue(60);
  qApp->processEvents();
  //snap and warp
  this->snapAndWarp();
  this->updateOpenGL();
  status.setValue(70);
  qApp->processEvents();
  //stencil
  this->stencilTets();
  this->updateOpenGL();
  status.setValue(80);
  if (this->ui->fixJacobianCheckBox->isChecked()) {
    this->mesher->fixVertexWindup(true);
  }
  status.setValue(90);
  qApp->processEvents();
  //MainWindow::instance()->enableMeshedVolumeOptions();
  status.setValue(100);
}
//=========================================
// - createBackgroundMesh()
//
//=========================================
void CleaverWidget::createBackgroundMesh() {
  auto sf = this->data_->hasSizingField();
  this->mesher->createBackgroundMesh(true);
  cleaver::TetMesh *mesh = this->mesher->getBackgroundMesh();
  mesh->name = "Adaptive-BCC-Mesh";
  this->data_->setMesh(mesh);
  this->window_->setMesh(mesh);
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

