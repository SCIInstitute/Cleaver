#ifndef DATAMANAGERWIDGET_H
#define DATAMANAGERWIDGET_H

#include <QDockWidget>
#include <QMdiSubWindow>
#include <QVBoxLayout>
#include <QSpacerItem>
#include "DataGroupWidget.h"
#include "FieldDataWidget.h"
#include "MeshDataWidget.h"
#include "VolumeDataWidget.h"
#include <map>
#include <string>
#include <Cleaver/ScalarField.h>
#include <Cleaver/Volume.h>
#include <Cleaver/TetMesh.h>
#include <DataManager.h>

typedef void DataGroup;

namespace Ui {
class DataManagerWidget;
}

class DataManagerWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit DataManagerWidget(QWidget *parent = 0);
    ~DataManagerWidget();

    void updateGroupWidgets();
    DataGroupWidget* makeNewGroup(DataGroup *);

public slots:
    void focus(QMdiSubWindow*){}
    void updateList();
    void selectionUpdate();

private:
    Ui::DataManagerWidget *ui;
    DataManager manager_;
    QSpacerItem *spacer_;
    QVBoxLayout *vbox_;
    std::vector<QWidget*> widgets_;
};

#endif // DATAMANAGERWIDGET_H
