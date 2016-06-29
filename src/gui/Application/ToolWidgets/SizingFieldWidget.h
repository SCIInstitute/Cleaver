#ifndef DATALOADERWIDGET_H
#define DATALOADERWIDGET_H

#include <QDockWidget>
#include <QMdiSubWindow>
#include <Cleaver/Volume.h>
#include <Cleaver/CleaverMesher.h> 
#include <DataWidgets/DataManagerWidget.h>

namespace Ui {
class SizingFieldWidget;
}

class SizingFieldWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit SizingFieldWidget(QWidget *parent = NULL, 
      DataManagerWidget * data = NULL);
    ~SizingFieldWidget();
    void setCreateButtonEnabled(bool b);

public slots:

    void loadSizingField();
    void computeSizingField();

protected:

    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    
private:
    Ui::SizingFieldWidget *ui;
    DataManagerWidget * data_;
};

#endif // DATALOADERWIDGET_H
