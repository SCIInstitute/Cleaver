//=====================================================
//  Copyright (c) 2014,  Jonathan Bronson
//
//  This source code is not to be used, copied,
//  or  shared,  without explicit permission from
//  the author.
//=====================================================


#ifndef BLOBBIESWIDGET_H
#define BLOBBIESWIDGET_H

#include <QDockWidget>
#include <Cleaver/Volume.h>

namespace Ui {
class BlobbiesWidget;
}

class BlobbiesWidget : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit BlobbiesWidget(QWidget *parent = 0);
    ~BlobbiesWidget();


public slots:

    void createBlobbies();
    
private:
    Ui::BlobbiesWidget *ui;
};

#endif // BLOBBIESWIDGET_H
