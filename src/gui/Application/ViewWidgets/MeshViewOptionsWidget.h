#ifndef MESHVIEWOPTIONSWIDGET_H
#define MESHVIEWOPTIONSWIDGET_H

#include <QDockWidget>
#include <QSignalMapper>
#include <QStandardItemModel>
#include "MeshWindow.h"
#include <QMdiSubWindow>

class CheckableItemModel : public QStandardItemModel
{
public:

  CheckableItemModel(int rows, int columns, QObject *parent) : QStandardItemModel(rows, columns, parent) { };

  Qt::ItemFlags flags(const QModelIndex& index) const {
    return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
  }
};


namespace Ui {
  class MeshViewOptionsWidget;
}

class MeshViewOptionsWidget : public QDockWidget
{
  Q_OBJECT

public:
  explicit MeshViewOptionsWidget(QWidget *parent = NULL, MeshWindow * window = NULL);
  ~MeshViewOptionsWidget();

  public slots:
  void focus(QMdiSubWindow *);

  void scrollingCheckboxClicked(int index);
  void clippingCheckboxClicked(bool value);
  void syncClippingCheckboxClicked(bool value);
  void xCheckboxClicked(bool value);
  void yCheckboxClicked(bool value);
  void zCheckboxClicked(bool value);

  void xSliderMoved(int value);
  void ySliderMoved(int value);
  void zSliderMoved(int value);

  void clippingSliderPressed();
  void clippingSliderReleased();

  void addMaterialsItem(const QString &name, const QVariant &faceVisible, const QVariant &cellVisible);
  void materialItemChanged(QStandardItem* item);
  void setShowCutsCheckboxEnabled(bool b);

private:
  Ui::MeshViewOptionsWidget *ui;
  QSignalMapper *signalMapper;
  CheckableItemModel *m_materialViewModel;
  MeshWindow * window_;
};

#endif // MESHVIEWOPTIONSWIDGET_H
