//=====================================================
//  Copyright (c) 2014,  Jonathan Bronson
//
//  This source code is not to be used, copied,
//  or  shared,  without explicit permission from
//  the author.
//=====================================================

#ifndef TESTDATAWIDGET_H
#define TESTDATAWIDGET_H

#include <QDockWidget>
#include <Cleaver/Volume.h>

namespace Ui {
class TestDataWidget;
}

class TestDataWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit TestDataWidget(QWidget *parent = 0);
    ~TestDataWidget();

public slots:

    cleaver::Volume* createOldData();
    cleaver::Volume* createSphereData();
    cleaver::Volume* createTorusData();
    cleaver::Volume* createConstantData();
    cleaver::Volume* createHighDensityPatches();
    cleaver::Volume* create2DSliceData();
    cleaver::Volume* create2DVariableSliceData();
    cleaver::Volume* createSmoothDensities();

    void loadTestData();
    void saveTestData();
    
private:
    Ui::TestDataWidget *ui;
};

#endif // TESTDATAWIDGET_H
