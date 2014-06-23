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


typedef void DataGroup;

typedef std::map< std::string, DataGroupWidget* >  DataWidgetMap;
typedef std::map< ulong, FieldDataWidget* >  FieldMap;
typedef std::map< ulong, VolumeDataWidget* > VolumeMap;
typedef std::map< ulong, MeshDataWidget* >   MeshMap;


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

    DataWidgetMap groupMap;
    VolumeMap     volumeMap;
    MeshMap       meshMap;
    FieldMap      fieldMap;

    QSpacerItem *spacer;
    QVBoxLayout *vbox;
};

#endif // DATAMANAGERWIDGET_H
