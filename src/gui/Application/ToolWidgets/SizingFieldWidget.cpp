#include "SizingFieldWidget.h"
#include "ui_SizingFieldWidget.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProgressDialog>
#include <iostream>
#include <string>
#include <vector>
#include <Cleaver/Cleaver.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/ScaledField.h>
#include <Cleaver/SizingFieldCreator.h>
#include <NRRDTools.h>
#include <Cleaver/Timer.h>

SizingFieldWidget::SizingFieldWidget(cleaver::CleaverMesher& mesher, 
  QWidget *parent) :
  QDockWidget(parent),
  mesher_(mesher),
  ui(new Ui::SizingFieldWidget)
{
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

void SizingFieldWidget::loadSizingField()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select Sizing Field"), 
    QDir::currentPath(), tr("NRRD (*.nrrd)"));

  if (!fileName.isEmpty())
  {
    std::vector<cleaver::AbstractScalarField*> sizingField =
      NRRDTools::loadNRRDFiles({ {fileName.toStdString()} });
    this->mesher_.getVolume()->setSizingField(sizingField[0]);
    emit sizingFieldDone();
  }
}

void SizingFieldWidget::computeSizingField()
{
  float factor = ui->factorSpinBox->value();
  float speed = 1.0 / ui->lipschitzSpinBox->value();
  int padding = ui->paddingSpinBox->value();
  bool adaptiveSurface = QString::compare(
    ui->surfaceComboBox->currentText(), 
    QString("constant"), Qt::CaseInsensitive) == 0 ? true : false;

  QProgressDialog status(QString("Computing Sizing Field..."), QString(), 0, 100, this);
  status.show();
  status.setWindowModality(Qt::WindowModal);
  cleaver::Timer timer;
  timer.start();
  status.setValue(10);
  QApplication::processEvents();
  cleaver::AbstractScalarField *sizingField = 
    cleaver::SizingFieldCreator::createSizingFieldFromVolume(
      this->mesher_.getVolume(), speed, 1., factor, padding, adaptiveSurface, true);
  timer.stop();
  this->mesher_.getVolume()->setSizingField(sizingField);
  status.setValue(50);
  QApplication::processEvents();
  std::string sizingFieldName = this->mesher_.getVolume()->name() + "-computed-sizing-field";
  sizingField->setName(sizingFieldName);
  status.setValue(100);
  emit sizingFieldDone();
} 

void SizingFieldWidget::dragEnterEvent(QDragEnterEvent *event)
{
  //  if (event->mimeData()->hasFormat("text/plain"))
  event->acceptProposedAction();
}

void SizingFieldWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
  //    if (event->mimeData()->hasFormat("text/plain"))
      //event->
}

void SizingFieldWidget::dropEvent(QDropEvent *event)
{
  //    textBrowser->setPlainText(event->mimeData()->text());
  //         mimeTypeCombo->clear();
  //         mimeTypeCombo->addItems(event->mimeData()->formats());

  event->acceptProposedAction();
}