#include "SizingFieldWidget.h"
#include "ui_SizingFieldWidget.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <iostream>
#include <string>
#include <vector>
#include <Cleaver/Cleaver.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/ScaledField.h>
#include <Cleaver/SizingFieldCreator.h>
#include <NRRDTools.h>
#include <stdexcept>

SizingFieldWidget::SizingFieldWidget(cleaver::CleaverMesher& mesher,
  QWidget *parent) :
  QDockWidget(parent),
  mesher_(mesher),
  ui(new Ui::SizingFieldWidget) {
  ui->setupUi(this);
  setAcceptDrops(true);
  ui->lipschitzWidget->layout()->setMargin(0);
  ui->lipschitzWidget->layout()->setSpacing(4);
  ui->dockWidgetContents->layout()->setMargin(4);
  ui->dockWidgetContents->layout()->setSpacing(1);
  ui->computeWidget->layout()->setMargin(0);
  ui->computeWidget->layout()->setSpacing(0);
}

SizingFieldWidget::~SizingFieldWidget(){
  delete ui;
}

void SizingFieldWidget::setCreateButtonEnabled(bool b) {
  this->ui->computeSizingFieldButton->setEnabled(b);
}

void SizingFieldWidget::loadSizingField() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select Sizing Field"),
    QDir::currentPath(), tr("NRRD (*.nrrd)"));

  if (!fileName.isEmpty()) {
    std::vector<cleaver::AbstractScalarField*> sizingField =
      NRRDTools::loadNRRDFiles({ {fileName.toStdString()} });
    this->mesher_.getVolume()->setSizingField(sizingField[0]);
    emit sizingFieldDone();
  }
}

void SizingFieldWidget::computeSizingField() {
  float refinementFactor = ui->refinementFactor->value();
  float sizeMultiplier = ui->factorSpinBox->value();
  float lipschitz = 1.0 / ui->lipschitzSpinBox->value();
  int padding = ui->paddingSpinBox->value();
  bool adaptiveSurface = QString::compare(
    ui->surfaceComboBox->currentText(),
    QString("constant"), Qt::CaseInsensitive) == 0 ? false : true;
  SizingFieldThread *workerThread = new SizingFieldThread(this->mesher_, this,
    refinementFactor, sizeMultiplier, lipschitz, padding, adaptiveSurface);
  connect(workerThread, SIGNAL(sizingFieldDone()), this, SLOT(handleSizingFieldDone()));
  connect(workerThread, SIGNAL(message(std::string)), this, SLOT(handleMessage(std::string)));
  connect(workerThread, SIGNAL(errorMessage(std::string)), this, SLOT(handleErrorMessage(std::string)));
  connect(workerThread, SIGNAL(progress(int)), this, SLOT(handleProgress(int)));
  connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
  workerThread->start();
}


//=========================================
// - Handlers
//=========================================


void SizingFieldWidget::handleSizingFieldDone() { emit sizingFieldDone(); }

void SizingFieldWidget::handleMessage(std::string str) { emit message(str); }

void SizingFieldWidget::handleErrorMessage(std::string str) { emit errorMessage(str); }

void SizingFieldWidget::handleProgress(int v) { emit progress(v); }

SizingFieldThread::SizingFieldThread(
  cleaver::CleaverMesher& mesher, QObject * parent,
  float refinementFactor, float sizeMultiplier, float lipschitz,
  int padding, bool adapt) :
  QThread(parent), mesher_(mesher), refinementFactor_(refinementFactor),
  sizeMultiplier_(sizeMultiplier), lipschitz_(lipschitz), padding_(padding),
  adapt_(adapt) { }

SizingFieldThread::~SizingFieldThread() {}

void SizingFieldThread::run() {
  emit message("Computing Sizing Field...");
  emit progress(5);
  try {
    cleaver::AbstractScalarField *sizingField =
      cleaver::SizingFieldCreator::createSizingFieldFromVolume(
        this->mesher_.getVolume(), this->lipschitz_,
        this->refinementFactor_, this->sizeMultiplier_,
        this->padding_, this->adapt_, true);
    this->mesher_.getVolume()->setSizingField(sizingField);
    emit progress(50);
    std::string sizingFieldName =
      this->mesher_.getVolume()->name() + "-computed-sizing-field";
    sizingField->setName(sizingFieldName);
  } catch (std::exception& e) {
    emit errorMessage(e.what());
  } catch (...) {
    emit errorMessage("There was a problem creating the sizing field!");
  }
  emit message("Successfully computed Sizing Field.");
  emit progress(100);
  emit sizingFieldDone();
}
