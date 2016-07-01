#include "DataManagerWidget.h"
#include "ui_DataManagerWidget.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include "MeshDataWidget.h"
#include "FieldDataWidget.h"
#include "VolumeDataWidget.h"

DataManagerWidget::DataManagerWidget(QWidget *parent) :
  QDockWidget(parent),
  spacer_(nullptr),
  vbox_(nullptr),
  ui(new Ui::DataManagerWidget) {
  ui->setupUi(this);
  this->spacer_ = new QSpacerItem(0, 0,
    QSizePolicy::Expanding, QSizePolicy::Expanding);
  // Put the widget in the layout
  this->vbox_ = new QVBoxLayout;
  this->vbox_->addSpacerItem(this->spacer_);
  this->vbox_->setContentsMargins(0, 0, 0, 0);
  this->vbox_->setMargin(0);
  this->vbox_->setSpacing(0);
  this->vbox_->setAlignment(Qt::AlignTop);
  ui->scrollAreaWidgetContents->setLayout(this->vbox_);
  this->vbox_->addSpacerItem(this->spacer_);
}

void DataManagerWidget::updateLayout() {
  this->vbox_->removeItem(this->spacer_);
  QLayoutItem *child;
  while ((child = this->vbox_->takeAt(0)) != 0) {
    delete child->widget();
    delete child;
  }
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
  this->widgets_.clear();
  //  Update volume
  auto v = this->manager_.volume();
  if (v != nullptr) {
    auto volWidget = new VolumeDataWidget(v, this);
    this->vbox_->insertWidget(static_cast<int>(this->widgets_.size()), volWidget);
    this->widgets_.push_back(volWidget);
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
    this->vbox_->insertWidget(static_cast<int>(this->widgets_.size()), ind);
    this->widgets_.push_back(ind);
    connect(ind, SIGNAL(exportField(void*)),
      this, SLOT(handleExportField(void*)));
  }
  //  Update sizing Field
  auto sizingField = this->manager_.sizingField();
  if (sizingField != nullptr) {
    auto fieldWidget = new FieldDataWidget(sizingField, this);
    this->vbox_->insertWidget(static_cast<int>(this->widgets_.size()), fieldWidget);
    this->widgets_.push_back(fieldWidget);
    connect(fieldWidget, SIGNAL(exportField(void*)),
      this, SLOT(handleExportField(void*)));
  } else {
    emit disableMeshing();
  }
  //  Update mesh
  auto m = this->manager_.mesh();
  if (m != nullptr) {
    auto meshWidget = new MeshDataWidget(m, this);
    this->vbox_->insertWidget(static_cast<int>(this->widgets_.size()), meshWidget);
    this->widgets_.push_back(meshWidget);
    connect(meshWidget, SIGNAL(exportMesh(void*)),
      this, SLOT(handleExportMesh(void*)));
  }
  // add spacer back
  this->vbox_->addSpacerItem(this->spacer_);
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