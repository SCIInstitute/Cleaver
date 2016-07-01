#include "DataManagerWidget.h"
#include "ui_DataManagerWidget.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include "MeshDataWidget.h"
#include "FieldDataWidget.h"
#include "VolumeDataWidget.h"

DataManagerWidget::DataManagerWidget(QWidget *parent) :
  QDockWidget(parent),
  vbox_(nullptr),
  addCount_(0),
  ui(new Ui::DataManagerWidget) {
  ui->setupUi(this);
  this->updateLayout();
}

void DataManagerWidget::updateLayout() {
  this->ui->scrollAreaWidgetContents =
    new QWidget(this);
  this->ui->scrollAreaWidgetContents->show();
  this->ui->scrollArea->setWidget(this->ui->scrollAreaWidgetContents);
  // Put the spacer in the new layout
  this->vbox_ = new QVBoxLayout;
  this->vbox_->setContentsMargins(0, 0, 0, 0);
  this->vbox_->setMargin(0);
  this->vbox_->setSpacing(0);
  this->vbox_->setAlignment(Qt::AlignTop);
  this->ui->scrollAreaWidgetContents->setLayout(this->vbox_);
  this->addCount_ = 0;
}

DataManagerWidget::~DataManagerWidget() {
    delete ui;
}

void DataManagerWidget::setMesh(cleaver::TetMesh *mesh) {
  this->manager_.setMesh(mesh);
  this->updateList();
}

void DataManagerWidget::setSizingField(cleaver::AbstractScalarField *field) {
  this->manager_.setSizingField(field);
  this->updateList();
}

void DataManagerWidget::setIndicators(std::vector<cleaver::AbstractScalarField *> indicators) {
  this->manager_.setIndicators(indicators);
  this->updateList();
}

void DataManagerWidget::setVolume(cleaver::Volume *volume) {
  this->manager_.setVolume(volume);
  this->updateList();
}

void DataManagerWidget::updateList() {
  this->updateLayout();
  //  Update volume
  auto v = this->manager_.volume();
  if (v != nullptr) {
    auto volWidget = new VolumeDataWidget(v, this);
    this->vbox_->insertWidget(static_cast<int>(this->addCount_++), volWidget);
    connect(volWidget, SIGNAL(updateDataWidget()),
      this, SLOT(clearRemoved()));
  }
  //  Update indicators
  auto inds = this->manager_.indicators();
  if (inds.size() < 2) {
    emit disableSizingField();
  }
  for (auto a : inds) {
    auto ind = new FieldDataWidget(a, this);
    this->vbox_->insertWidget(static_cast<int>(this->addCount_++), ind);
    connect(ind, SIGNAL(exportField(void*)),
      this, SLOT(handleExportField(void*)));
  }
  //  Update sizing Field
  auto sizingField = this->manager_.sizingField();
  if (sizingField != nullptr) {
    auto fieldWidget = new FieldDataWidget(sizingField, this);
    this->vbox_->insertWidget(static_cast<int>(this->addCount_++), fieldWidget);
    connect(fieldWidget, SIGNAL(exportField(void*)),
      this, SLOT(handleExportField(void*)));
  } else {
    emit disableMeshing();
  }
  //  Update mesh
  auto m = this->manager_.mesh();
  if (m != nullptr) {
    auto meshWidget = new MeshDataWidget(m, this);
    this->vbox_->insertWidget(static_cast<int>(this->addCount_++), meshWidget);
    connect(meshWidget, SIGNAL(exportMesh(void*)),
      this, SLOT(handleExportMesh(void*)));
  }
  // add spacer
  this->vbox_->addSpacerItem(new QSpacerItem(0, 0,
    QSizePolicy::Expanding, QSizePolicy::Expanding));
  this->repaint();
  this->update();
}

void DataManagerWidget::handleExportField(void* p) {
  emit exportField(p);
}

void DataManagerWidget::handleExportMesh(void* p) {
  emit exportMesh(p);
}

void DataManagerWidget::clearRemoved() {
  std::vector<cleaver::AbstractScalarField*> fields;
  for (size_t i = 0; i < this->manager_.volume()->numberOfMaterials(); i++) {
    fields.push_back(this->manager_.volume()->getMaterial(
      static_cast<int>(i)));
  }
  this->manager_.setIndicators(fields);
  this->manager_.setSizingField(this->manager_.volume()->getSizingField());
  this->updateList();
}