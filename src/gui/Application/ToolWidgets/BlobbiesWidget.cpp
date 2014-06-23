//=====================================================
//  Copyright (c) 2012,  Jonathan Bronson
//
//  This source code is not to be used, copied,
//  or  shared,  without explicit permission from
//  the author.
//=====================================================

#ifdef WIN32
  #define NOMINMAX  // so windows doesn't mangle std::min
#endif

#include "BlobbiesWidget.h"
#include "ui_BlobbiesWidget.h"

#include "MainWindow.h"
#include <Cleaver/Volume.h>
#include <Cleaver/Cleaver.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/ConstantField.h>

#include <Synthetic/BlobbyField.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>

//-------------------
// Helper Function
//-------------------

namespace {

float urandf()
{
    #ifdef RAND_MAX
    return ((float)rand() / RAND_MAX);
    #else
    return ((float)rand() / std::numeric_limits<int>::max());
    #endif
}

}

BlobbiesWidget::BlobbiesWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::BlobbiesWidget)
{
    ui->setupUi(this);

    //ui->dockWidgetContents->layout()->setMargin(4);
    //ui->dockWidgetContents->layout()->setSpacing(1);
    //ui->dockWidgetContents->layout()->setAlignment(ui->loadTestDataButton, Qt::AlignHCenter);
    //ui->dockWidgetContents->layout()->setAlignment(ui->saveTestDataButton, Qt::AlignHCenter);
}

BlobbiesWidget::~BlobbiesWidget()
{
    delete ui;
}

void BlobbiesWidget::createBlobbies()
{
    // get user options
    int width  = ui->widthSpinBox->value();
    int height = ui->heightSpinBox->value();
    int depth  = ui->depthSpinBox->value();
    int material_count = ui->materialsSpinBox->value();

    // create bounds
    cleaver::BoundingBox bounds(cleaver::vec3::zero,
                                cleaver::vec3(width, height, depth));

    // create list to store all of the materials
    std::vector<cleaver::AbstractScalarField*> fields;

    // create background material
    cleaver::ConstantField<float> *backField = new cleaver::ConstantField<float>(0, bounds);
    fields.push_back(backField);

    // create material indicator functions
    int min_dim = std::min(width, height);
    min_dim = std::min(min_dim, depth);

    for(int i=0; i < material_count; i++)
    {
        // center blobby randomly between 5% and 95% from boundaries
        double rcx = (0.9*urandf() + 0.05) * 0.5*bounds.size.x + 0.25*bounds.size.x;
        double rcy = (0.9*urandf() + 0.05) * 0.5*bounds.size.y + 0.25*bounds.size.y;
        double rcz = (0.9*urandf() + 0.05) * 0.5*bounds.size.z + 0.25*bounds.size.z;

        float rr = 0.05*min_dim + urandf()*(0.25*min_dim);

        BlobbyField *field = new BlobbyField(cleaver::vec3(rcx,rcy,rcz), rr, 8, bounds);
        fields.push_back(field);
    }

    // construct the volume from fields list
    cleaver::Volume *basevol = new cleaver::Volume(fields);
    cleaver::Volume *volume = cleaver::createFloatFieldVolumeFromVolume(basevol);

    MainWindow::dataManager()->addVolume(volume);
    MainWindow::instance()->createWindow(volume, volume->name().c_str());
}


