#include "CleaverWidget.h"
#include "ui_CleaverWidget.h"
#include <Cleaver/TetMesh.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/Timer.h>
#include <Cleaver/SizingFieldCreator.h>
#include <iostream>
#include <QApplication>
#include <stdexcept>

CleaverWidget::CleaverWidget(cleaver::CleaverMesher& mesher,
  QWidget *parent) :
  QDockWidget(parent),
  mesher_(mesher),
  ui(new Ui::CleaverWidget) {
  this->ui->setupUi(this);
  qRegisterMetaType<std::string>();
}

CleaverWidget::~CleaverWidget() {
  delete this->ui;
}

void CleaverWidget::setMeshButtonEnabled(bool b) {
  this->ui->createMeshButton->setEnabled(b);
}
void CleaverWidget::clear() {
  this->resetCheckboxes();
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
  this->ui->fixJacobianCheckBox->setChecked(false);
}
//=========================================
// - createMesh()
//=========================================
void CleaverWidget::createMesh() {
  this->clear();
  CleaverThread *workerThread = new CleaverThread(this->mesher_, this);
  connect(workerThread, SIGNAL(doneMeshing()), this, SLOT(handleDoneMeshing()));
  connect(workerThread, SIGNAL(repaintGL()), this, SLOT(handleRepaintGL()));
  connect(workerThread, SIGNAL(newMesh()), this, SLOT(handleNewMesh()));
  connect(workerThread, SIGNAL(message(std::string)), this, SLOT(handleMessage(std::string)));
  connect(workerThread, SIGNAL(errorMessage(std::string)), this, SLOT(handleErrorMessage(std::string)));
  connect(workerThread, SIGNAL(progress(int)), this, SLOT(handleProgress(int)));
  connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
  workerThread->start();
}

//=========================================
// - Handlers
//=========================================

void CleaverWidget::handleDoneMeshing() { emit doneMeshing(); }

void CleaverWidget::handleRepaintGL() { emit repaintGL(); }

void CleaverWidget::handleNewMesh() { emit newMesh(); }

void CleaverWidget::handleMessage(std::string str) { emit message(str); }

void CleaverWidget::handleErrorMessage(std::string str) { emit errorMessage(str); }

void CleaverWidget::handleProgress(int v) {
  switch (v) {
  case 15:
    this->ui->backgroundCheck->setChecked(true);
    break;
  case 25:
    this->ui->adjacencyCheck->setChecked(true);
    break;
  case 35:
    this->ui->sampleCheck->setChecked(true);
    break;
  case 45:
    this->ui->alphaCheck->setChecked(true);
    break;
  case 55:
    this->ui->interfaceCheck->setChecked(true);
    break;
  case 65:
    this->ui->generalizeCheck->setChecked(true);
    break;
  case 75:
    this->ui->snapCheck->setChecked(true);
    break;
  case 85:
    this->ui->stencilCheck->setChecked(true);
    break;
  case 100:
    this->ui->fixJacobianCheckBox->setChecked(true);
    break;
  default:
    break;
  }
  emit progress(v);
}

//=========================================
// - Threaded functions
//=========================================
CleaverThread::CleaverThread(cleaver::CleaverMesher& mesher,
  QObject * parent) : QThread(parent), mesher_(mesher) {}

CleaverThread::~CleaverThread() {}

void CleaverThread::run() {
  //progress
  emit message("Creating Background mesh...");
  emit progress(5);
  try {
    //create background mesh
    this->mesher_.setConstant(false);
    this->mesher_.createBackgroundMesh(true);
    this->mesher_.getBackgroundMesh()->name = "Adaptive-BCC-Mesh";
    emit newMesh();
    emit message("Calculating Adjacencies...");
    emit progress(15);
    //adjacencies
    this->mesher_.buildAdjacency(true);
    emit message("Sampling Volume...");
    emit progress(25);
    //sample vol
    this->mesher_.sampleVolume(true);
    emit repaintGL();
    emit message("Computing Alphas...");
    emit progress(35);
    //alphas
    this->mesher_.computeAlphas();
    emit message("Computing Interfaces...");
    emit progress(45);
    //interfaces
    this->mesher_.computeInterfaces(true);
    emit repaintGL();
    emit message("Generalizing Tets...");
    emit progress(55);
    //generalize
    this->mesher_.generalizeTets();
    emit message("Snapping and Warping Mesh...");
    emit progress(65);
    //snap and warp
    this->mesher_.snapsAndWarp();
    emit repaintGL();
    emit message("Stecilling Tets...");
    emit progress(75);
    //stencil
    this->mesher_.stencilTets();
    emit repaintGL();
    emit message("Fixing vertex wind-ups...");
    emit progress(85);
    this->mesher_.fixVertexWindup(true);
  } catch (std::exception e) {
    emit errorMessage(e.what());
  } catch (...) {
    emit errorMessage("There was a problem creating the sizing field!");
  }
  emit message("Successfully completed meshing!");
  emit progress(100);
  emit doneMeshing();
}
