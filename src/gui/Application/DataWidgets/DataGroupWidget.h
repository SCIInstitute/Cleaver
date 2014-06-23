#ifndef DATAGROUPWIDGET_H
#define DATAGROUPWIDGET_H

#include <QWidget>

namespace Ui {
class DataGroupWidget;
}

class DataGroupWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit DataGroupWidget(QWidget *parent = 0);
    ~DataGroupWidget();
    
private:
    Ui::DataGroupWidget *ui;
};

#endif // DATAGROUPWIDGET_H
