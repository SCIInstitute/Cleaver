#include "DataGroupWidget.h"
#include "ui_DataGroupWidget.h"

DataGroupWidget::DataGroupWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataGroupWidget)
{
    ui->setupUi(this);
}

DataGroupWidget::~DataGroupWidget()
{
    delete ui;
}
