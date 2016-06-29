#include "DataManagerWidget.h"
#include "ui_DataManagerWidget.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include "MeshDataWidget.h"
#include "FieldDataWidget.h"
#include "VolumeDataWidget.h"
#include "MainWindow.h"

DataManagerWidget::DataManagerWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DataManagerWidget)
{
    ui->setupUi(this);

    updateGroupWidgets();
    QObject::connect(MainWindow::dataManager(), SIGNAL(dataChanged()), this, SLOT(updateList()));
}

DataManagerWidget::~DataManagerWidget()
{
    delete ui;
}

DataGroupWidget* DataManagerWidget::makeNewGroup(DataGroup *group)
{
    DataGroupWidget *new_group = new DataGroupWidget();  // (parent, group)

    return new_group;
}

void DataManagerWidget::updateGroupWidgets()
{
    // Get a list of all the groups
    std::vector< DataGroup* > groups;

    this->spacer_ = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Put the widget in the layout
    this->vbox_ = new QVBoxLayout;
    this->vbox_->addSpacerItem(this->spacer_);
    this->vbox_->setContentsMargins(0,0,0,0);
    this->vbox_->setMargin(0);
    this->vbox_->setSpacing(0);
    this->vbox_->setAlignment(Qt::AlignTop);

    ui->scrollAreaWidgetContents->setLayout(this->vbox_);
}

void DataManagerWidget::updateList() {
  // clear vbox
  this->vbox_->removeItem(this->spacer_);
  for (auto a : this->widgets_) {
    this->vbox_->removeWidget(a);
  }
  this->widgets_.clear();
  //  Update sizing Field
  auto sizingField = this->manager_.sizingField();
  auto fieldWidget = new FieldDataWidget(sizingField, this);
  this->vbox_->insertWidget(this->widgets_.size(), fieldWidget);
  this->widgets_.push_back(fieldWidget);
  //  Update volume
  auto v = this->manager_.volume();
  auto volWidget = new VolumeDataWidget(v, this);
  this->vbox_->insertWidget(this->widgets_.size(), volWidget);
  this->widgets_.push_back(volWidget);
  //  Update indicators
  auto inds = this->manager_.indicators();
  for (auto a : inds) {
    auto ind = new FieldDataWidget(a, this);
    this->vbox_->insertWidget(this->widgets_.size(), ind);
    this->widgets_.push_back(ind);
  }
  //  Update volume
  auto v = this->manager_.volume();
  auto volWidget = new VolumeDataWidget(v, this);
  this->vbox_->insertWidget(this->widgets_.size(), volWidget);
  this->widgets_.push_back(volWidget);
  //  Update mesh
  auto m = this->manager_.mesh();
  auto meshWidget = new MeshDataWidget(m, this);
  this->vbox_->insertWidget(this->widgets_.size(), meshWidget);
  this->widgets_.push_back(meshWidget);
  // add spacer back
  this->vbox_->addSpacerItem(this->spacer_);
}

void DataManagerWidget::selectionUpdate() {
  //todo
  //meshWidget->setSelected(false);
}



