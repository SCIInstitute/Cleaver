#include "MeshViewOptionsWidget.h"
#include "ui_MeshViewOptionsWidget.h"

MeshViewOptionsWidget::MeshViewOptionsWidget(QWidget *parent, MeshWindow * window) :
  QDockWidget(parent),
  window_(window),
  ui(new Ui::MeshViewOptionsWidget)
{
  ui->setupUi(this);

  signalMapper = new QSignalMapper(this);

  // set inputs to mapper
  connect(ui->showAxisCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showAxisCheckbox, 1);

  connect(ui->showBBoxCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showBBoxCheckbox, 2);

  connect(ui->showFacesCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showFacesCheckbox, 3);

  connect(ui->showEdgesCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showEdgesCheckbox, 4);

  connect(ui->showCutsCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showCutsCheckbox, 5);

  connect(ui->showSurfacesCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->showSurfacesCheckbox, 6);

  connect(ui->colorByQualityCheckbox, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(ui->colorByQualityCheckbox, 7);


  // set output of mapper
  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(scrollingCheckboxClicked(int)));


  m_materialViewModel = new CheckableItemModel(0, 3, this);
  m_materialViewModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Material"));
  m_materialViewModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Faces"));
  m_materialViewModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Cells"));
  ui->materialsView->setModel(m_materialViewModel);
  ui->materialsView->setColumnWidth(0, 80);
  ui->materialsView->setColumnWidth(1, 50);
  ui->materialsView->setColumnWidth(2, 50);
}

MeshViewOptionsWidget::~MeshViewOptionsWidget()
{
  delete ui;
}

void MeshViewOptionsWidget::addMaterialsItem(const QString &name, const QVariant &faceVisible, const QVariant &cellVisible)
{
  int row = m_materialViewModel->rowCount();
  m_materialViewModel->insertRow(row);
  m_materialViewModel->setData(m_materialViewModel->index(row, 0), name);
  m_materialViewModel->setData(m_materialViewModel->index(row, 1), faceVisible, Qt::CheckStateRole);
  m_materialViewModel->setData(m_materialViewModel->index(row, 2), cellVisible, Qt::CheckStateRole);

  connect(m_materialViewModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(materialItemChanged(QStandardItem*)));
}

void MeshViewOptionsWidget::materialItemChanged(QStandardItem* item)
{
  // get material changed
  int material = item->row();

  // if face visibily lock changed
  if (item->column() == 1) {
    if (item->checkState() == Qt::Checked)
      this->window_->setMaterialFaceLock(material, true);
    else
      this->window_->setMaterialFaceLock(material, false);
  }
  // if cell visibility lock changed
  else if (item->column() == 2) {
    if (item->checkState() == Qt::Checked)
      this->window_->setMaterialCellLock(material, true);
    else
      this->window_->setMaterialCellLock(material, false);
  }

  this->window_->updateMesh();
  this->window_->updateGL();
}


void MeshViewOptionsWidget::setShowCutsCheckboxEnabled(bool b)
{
  this->ui->showCutsCheckbox->setEnabled(b);
}

void MeshViewOptionsWidget::focus(QMdiSubWindow* subwindow)
{
  /*
  if (subwindow != NULL) {

    MeshWindow *window = this->window_;

      ui->showBBoxCheckbox->setChecked(window->bboxVisible());
      ui->showFacesCheckbox->setChecked(window->facesVisible());
      ui->showEdgesCheckbox->setChecked(window->edgesVisible());
      ui->showCutsCheckbox->setChecked(window->cutsVisible());

      // set material locks
      if (!window->mesher()->getVolume() && !window->mesher()->getTetMesh())
      {
        m_materialViewModel->removeRows(0, m_materialViewModel->rowCount());
      } else if ((window->mesher()->getVolume() && 
        (window->mesher()->getVolume()->numberOfMaterials() != m_materialViewModel->rowCount())) ||
        (window->mesher()->getTetMesh() && window->mesher()->getTetMesh()->material_count !=
          m_materialViewModel->rowCount()))
      {
        m_materialViewModel->removeRows(0, m_materialViewModel->rowCount());
        int material_count = 0;
        if (window->mesher()->getVolume())
          material_count = window->mesher()->getVolume()->numberOfMaterials();
        else if (window->mesher()->getTetMesh())
          material_count = window->mesher()->getTetMesh()->material_count;
        for (int m = 0; m < material_count; m++)
        {
          addMaterialsItem(QString::number(m),
            window->getMaterialFaceLock(m) ? Qt::Checked : Qt::Unchecked,
            window->getMaterialCellLock(m) ? Qt::Checked : Qt::Unchecked);
        }
      } else {
        int material_count = 0;
        if (window->mesher()->getVolume())
          material_count = window->mesher()->getVolume()->numberOfMaterials();
        else if (window->mesher()->getTetMesh())
          material_count = window->mesher()->getTetMesh()->material_count;

        for (int m = 0; m < material_count; m++)
        {
          m_materialViewModel->setData(m_materialViewModel->index(m, 1),
            window->getMaterialFaceLock(m) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
          m_materialViewModel->setData(m_materialViewModel->index(m, 2), 
            window->getMaterialCellLock(m) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        }
    }
  }*/
}

void MeshViewOptionsWidget::scrollingCheckboxClicked(int index)
{
  MeshWindow *window = this->window_;
  if (window)
  {
    switch (index) {
    case 1:
      window->setAxisVisible(ui->showAxisCheckbox->isChecked());
      break;
    case 2:
      window->setBBoxVisible(ui->showBBoxCheckbox->isChecked());
      break;
    case 3:
      window->setFacesVisible(ui->showFacesCheckbox->isChecked());
      break;
    case 4:
      window->setEdgesVisible(ui->showEdgesCheckbox->isChecked());
      break;
    case 5:
      window->setCutsVisible(ui->showCutsCheckbox->isChecked());
      break;
    case 6:
      window->setSurfacesOnly(ui->showSurfacesCheckbox->isChecked());
      window->updateMesh();
      break;
    case 7:
      window->setColorByQuality(ui->colorByQualityCheckbox->isChecked());
      window->updateMesh();
      break;
    }

    window->updateGL();
  }
}

void MeshViewOptionsWidget::clippingCheckboxClicked(bool value)
{
  ui->xCheckbox->setEnabled(value);
  ui->yCheckbox->setEnabled(value);
  ui->zCheckbox->setEnabled(value);

  ui->xSlider->setEnabled(ui->xCheckbox->isChecked());
  ui->ySlider->setEnabled(ui->yCheckbox->isChecked());
  ui->zSlider->setEnabled(ui->zCheckbox->isChecked());

  MeshWindow *window = this->window_;

  if (window != NULL) {
    window->setClipping(value);
    if (ui->xCheckbox->isChecked())
      this->xSliderMoved(ui->xSlider->value());
    else if (ui->yCheckbox->isChecked())
      this->ySliderMoved(ui->zSlider->value());
    else if (ui->zCheckbox->isChecked())
      this->zSliderMoved(ui->ySlider->value());

    window->updateGL();
  }
}

void MeshViewOptionsWidget::syncClippingCheckboxClicked(bool value)
{
  MeshWindow *window = this->window_;

  if (window != NULL) {
    window->setSyncedClipping(value);
  }
}

void MeshViewOptionsWidget::xCheckboxClicked(bool value)
{
  ui->xSlider->setEnabled(ui->xCheckbox->isChecked());
  ui->ySlider->setEnabled(ui->yCheckbox->isChecked());
  ui->zSlider->setEnabled(ui->zCheckbox->isChecked());

  this->xSliderMoved(ui->xSlider->value());
}

void MeshViewOptionsWidget::yCheckboxClicked(bool value)
{
  ui->xSlider->setEnabled(ui->xCheckbox->isChecked());
  ui->ySlider->setEnabled(ui->yCheckbox->isChecked());
  ui->zSlider->setEnabled(ui->zCheckbox->isChecked());

  this->ySliderMoved(ui->zSlider->value());
}

void MeshViewOptionsWidget::zCheckboxClicked(bool value)
{
  ui->xSlider->setEnabled(ui->xCheckbox->isChecked());
  ui->ySlider->setEnabled(ui->yCheckbox->isChecked());
  ui->zSlider->setEnabled(ui->zCheckbox->isChecked());

  this->zSliderMoved(ui->ySlider->value());
}

void MeshViewOptionsWidget::xSliderMoved(int value)
{
  MeshWindow *window = this->window_;
  if (window != NULL) {

    float margin = 0.01f;
    float t = (1 + margin)*(value / (float)ui->xSlider->maximum()) - margin / 2;

    float plane[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    plane[3] = t;
    plane[3] *= window->dataBounds().size.x;

    window->setClippingPlane(plane);
    window->updateGL();
  }
}

void MeshViewOptionsWidget::ySliderMoved(int value)
{
  MeshWindow *window = this->window_;
  if (window != NULL)
  {
    float margin = 0.01f;
    float t = (1 + margin)*(value / (float)ui->ySlider->maximum()) - margin / 2;

    float plane[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    plane[3] = t;
    plane[3] *= window->dataBounds().size.y;

    window->setClippingPlane(plane);
    window->updateGL();
  }
}

void MeshViewOptionsWidget::zSliderMoved(int value)
{
  MeshWindow *window = this->window_;
  if (window != NULL)
  {
    float margin = 0.01f;
    float t = (1 + margin)*(value / (float)ui->zSlider->maximum()) - margin / 2;

    float plane[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    plane[3] = t;
    plane[3] *= window->dataBounds().size.z;

    window->setClippingPlane(plane);
    window->updateGL();
  }
}

void MeshViewOptionsWidget::clippingSliderPressed()
{
  // TODO: Save Handle to window so this check doesn't
  //       have to be here. Will be safe since focus change
  //       will allow us chance to invalidate this button
  //       so this function can't be called if window=NULL
  MeshWindow *window = this->window_;
  if (window != NULL) {
    window->setClippingPlaneVisible(true);
    window->updateMesh();
    window->updateGL();
  }
}

void MeshViewOptionsWidget::clippingSliderReleased()
{
  MeshWindow *window = this->window_;
  if (window != NULL) {
    window->setClippingPlaneVisible(false);
    window->updateMesh();
    window->updateGL();
  }
}
