#include "CleaverWidget.h"
#include "ui_CleaverWidget.h"
#include <Cleaver/TetMesh.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/Timer.h>
#include <Cleaver/SizingFieldCreator.h>
#include <iostream>
#include <QApplication>
#include <QProgressDialog>

CleaverWidget::CleaverWidget(cleaver::CleaverMesher& mesher,
  QWidget *parent) :
  QDockWidget(parent),
  mesher_(mesher),
  ui(new Ui::CleaverWidget) {
  this->ui->setupUi(this);
}

CleaverWidget::~CleaverWidget() {
  delete this->ui;
}

void CleaverWidget::setMeshButtonEnabled(bool b) {
  this->ui->createMeshButton->setEnabled(b);
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
//=========================================
// - createMesh()
//=========================================
void CleaverWidget::createMesh() {
  this->clear();
  //progress
  QProgressDialog status(QString("Meshing Indicator Functions..."),
    QString(), 0, 100, this);
  status.setWindowModality(Qt::WindowModal);
  status.show();
  status.setValue(5);
  qApp->processEvents();
  //create background mesh
  this->mesher_.setRegular(false);
  this->createBackgroundMesh();
  status.setValue(10);
  qApp->processEvents();
  //adjacencies
  this->buildMeshAdjacency();
  status.setValue(20);
  qApp->processEvents();
  //sample vol
  this->sampleVolume();
  emit repaintGL();
  status.setValue(30);
  qApp->processEvents();
  //alphas
  this->computeAlphas();
  status.setValue(40);
  qApp->processEvents();
  //interfaces
  this->computeInterfaces();
  emit repaintGL();
  status.setValue(50);
  qApp->processEvents();
  //generalize
  this->generalizeTets();
  status.setValue(60);
  qApp->processEvents();
  //snap and warp
  this->snapAndWarp();
  emit repaintGL();
  status.setValue(70);
  qApp->processEvents();
  //stencil
  this->stencilTets();
  emit repaintGL();
  status.setValue(80);
  if (this->ui->fixJacobianCheckBox->isChecked()) {
    this->mesher_.fixVertexWindup(true);
  }
  status.setValue(90);
  qApp->processEvents();
  status.setValue(100);
  emit doneMeshing();
}
//=========================================
// - createBackgroundMesh()
//
//=========================================
void CleaverWidget::createBackgroundMesh() {
  this->mesher_.createBackgroundMesh(true);
  this->mesher_.getBackgroundMesh()->name = "Adaptive-BCC-Mesh";
  this->ui->backgroundCheck->setChecked(true);
  emit newMesh();
  this->update();
}
//=========================================
// - buildMeshAdjacency()
//
//=========================================
void CleaverWidget::buildMeshAdjacency() {
  this->mesher_.buildAdjacency(true);
  this->ui->adjacencyCheck->setChecked(true);
  this->update();
}
//=========================================
// - sampleData()
//
//=========================================
void CleaverWidget::sampleVolume() {
  this->mesher_.sampleVolume(true);
  this->ui->sampleCheck->setChecked(true);
  this->update();
}
//=========================================
// - computeAlphas();
//
//=========================================
void CleaverWidget::computeAlphas() {
  this->mesher_.computeAlphas();
  this->ui->alphaCheck->setChecked(true);
  this->update();
}
//=========================================
// - computeCuts()
//
//=========================================
void CleaverWidget::computeInterfaces() {
  this->mesher_.computeInterfaces(true);
  this->ui->interfaceCheck->setChecked(true);
  this->update();
}
//=========================================
// - generalizeTets()
//
//=========================================
void CleaverWidget::generalizeTets() {
  this->mesher_.generalizeTets();
  this->ui->generalizeCheck->setChecked(true);
  this->update();
}
//=========================================
// - snapAndWarp()
//
//=========================================
void CleaverWidget::snapAndWarp() {
  this->mesher_.snapsAndWarp();
  this->ui->snapCheck->setChecked(true);
  this->update();
}
//=========================================
// - stencil()
//
//=========================================
void CleaverWidget::stencilTets() {
  this->mesher_.stencilTets();
  this->ui->stencilCheck->setChecked(true);
  this->update();
}