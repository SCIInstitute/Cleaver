#ifndef DATAMANAGERWIDGET_H
#define DATAMANAGERWIDGET_H

#include <QDockWidget>
#include <QMdiSubWindow>
#include <QVBoxLayout>
#include <QSpacerItem>
#include "FieldDataWidget.h"
#include "MeshDataWidget.h"
#include "VolumeDataWidget.h"
#include <map>
#include <string>
#include <Cleaver/ScalarField.h>
#include <Cleaver/Volume.h>
#include <Cleaver/TetMesh.h>
#include <Data/DataManager.h>

namespace Ui {
  class DataManagerWidget;
}

class DataManagerWidget : public QDockWidget
{
  Q_OBJECT

public:
  explicit DataManagerWidget(QWidget *parent = 0);
  ~DataManagerWidget();
  void setMesh(cleaver::TetMesh *mesh);
  void setSizingField(cleaver::AbstractScalarField *field);
  void setIndicators(std::vector<cleaver::AbstractScalarField *> indicators);
  void setVolume(cleaver::Volume *volume);
private:
  void updateLayout();
signals:
  void exportField(void*);
  void exportMesh(void*);
  void disableMeshing();
  void disableSizingField();
  public slots:
  void handleExportField(void*);
  void handleExportMesh(void*);
  void updateList();
  void clearRemoved();

private:
  Ui::DataManagerWidget *ui;
  DataManager manager_;
  QVBoxLayout * vbox_;
  size_t addCount_;
};

#endif // DATAMANAGERWIDGET_H
