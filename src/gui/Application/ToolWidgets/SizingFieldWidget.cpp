#include "SizingFieldWidget.h"
#include "ui_SizingFieldWidget.h"
#include "MainWindow.h"
#include <QFileDialog>
#include <iostream>
#include <string>
#include <vector>
#include <Cleaver/Cleaver.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/InverseField.h>
#include <Cleaver/ScaledField.h>
#include <Cleaver/SizingFieldCreator.h>
#include <nrrd2cleaver/nrrd2cleaver.h>
#include <Cleaver/Timer.h>

SizingFieldWidget::SizingFieldWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SizingFieldWidget), volume(NULL)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    //ui->lipschitzWidget.layout()->setMargin(0);
    //ui->lipschitzWidget.layout()->setSpacing(4);
    ui->lipschitzWidget->layout()->setMargin(0);
    ui->lipschitzWidget->layout()->setSpacing(4);
    ui->dockWidgetContents->layout()->setMargin(4);
    ui->dockWidgetContents->layout()->setSpacing(1);
    ui->computeWidget->layout()->setMargin(0);
    ui->computeWidget->layout()->setSpacing(0);
    //ui->dockWidgetContents->setStyleSheet(QString("font-size: 8;"));
    //ui->lipschitzLabel->
    //ui->dockWidgetContents->layout()->setAlignment(ui->dockWidgetContents->layout(), Qt::AlignHCenter);

    QObject::connect(MainWindow::dataManager(), SIGNAL(volumeListChanged()), this, SLOT(updateVolumeList()));
    QObject::connect(ui->volumeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(volumeSelected(int)));
}

SizingFieldWidget::~SizingFieldWidget()
{
    delete ui;
}



//========================
//     Public Slots
//========================


void SizingFieldWidget::focus(QMdiSubWindow* subwindow)
{
    // TODO: This method shouldn't exist, but it exists
    // as a hack to get the current mesher.

    if (subwindow != NULL){

        MeshWindow *window = qobject_cast<MeshWindow *>(subwindow->widget());

        if(window != NULL)
        {
            //std::cout << "DataLoaderWidget has Volume from Window" << std::endl;
            //ui->loadSizingFieldButton->setEnabled(true);
            //ui->computeSizingFieldButton->setEnabled(true);
            this->volume = window->volume();
            this->mesher = window->mesher();
        }
        //else
        //{
        //    //ui->loadSizingFieldButton->setEnabled(false);
        //    ui->computeSizingFieldButton->setEnabled(false);
        //}
    }

}

void SizingFieldWidget::loadIndicatorFunctions()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Indicator Functions"), QDir::currentPath(), tr("NRRD (*.nrrd)"));

    if(!fileNames.isEmpty())
    {
        std::vector<std::string> inputs;

        for(int i=0; i < fileNames.size(); i++){
            inputs.push_back(fileNames[i].toStdString());
        }

        std::vector<cleaver::AbstractScalarField*> fields = loadNRRDFiles(inputs, true);
        if(fields.empty()){
            std::cerr << "Failed to load image data." << std::endl;
            return;
        }
        else if(fields.size() == 1){
            fields.push_back(new cleaver::InverseScalarField(fields[0]));
        }

        this->volume = new cleaver::Volume(fields);
        static int v = 0;
        std::string volumeName = std::string("Volume");
        if(v > 0){
            volumeName += std::string(" ") + QString::number(v).toStdString();
        }
        volume->setName(volumeName);


        //Cleaver::ConstantField *sizingField = new Cleaver::ConstantField(4.0, volume->width(), volume->height(), volume->depth());
        //volume->setSizingField(sizingField);
        //volume->setSize(128,128,128);

        MainWindow::instance()->createWindow(volume, QString(volumeName.c_str()));

        //ui->loadSizingFieldButton->setEnabled(true);
    }
}

void SizingFieldWidget::loadSizingField()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Sizing Field"), QDir::currentPath(), tr("NRRD (*.nrrd)"));

    if(!fileName.isEmpty())
    {
        cleaver::AbstractScalarField* sizingField = loadNRRDFile(fileName.toStdString(), true);
        this->volume->setSizingField(sizingField);
        std::cout << "Sizing Field Set" << std::endl;
    }
}

void SizingFieldWidget::computeSizingField()
{
    std::cout << "Computing Sizing Field..." << std::flush;

    float factor = ui->factorSpinBox->value();
    float speed = 1.0 / ui->lipschitzSpinBox->value();
    float scale = ui->scaleSpinBox->value();
    int padding = ui->paddingSpinBox->value();
    bool adaptiveSurface = QString::compare(ui->surfaceComboBox->currentText(), QString("constant"), Qt::CaseInsensitive) == 0 ? false : true;

    cleaver::Timer timer;
    timer.start();
    cleaver::AbstractScalarField *sizingField = cleaver::SizingFieldCreator::createSizingFieldFromVolume(this->volume, speed, scale, factor, padding, adaptiveSurface, true);
    timer.stop();

    std::string sizingFieldName = volume->name() + "-computed-sizing-field";
    sizingField->setName(sizingFieldName);
    this->volume->setSizingField(sizingField);
    this->mesher->setSizingFieldTime(timer.time());

    // Add new sizing field to data manager
    MainWindow::dataManager()->addField(sizingField);

    std::cout << "done!" << std::endl;
    std::cout << "Computed in " << timer.time() << " seconds." << std::endl;
}

void SizingFieldWidget::updateVolumeList()
{
    ui->volumeBox->clear();

    std::vector<cleaver::Volume*> volumes = MainWindow::dataManager()->volumes();

    for(size_t i=0; i < volumes.size(); i++)
    {
        ui->volumeBox->addItem(volumes[i]->name().c_str());
    }
}

void SizingFieldWidget::volumeSelected(int index)
{
    if(index < 0){
        this->volume = NULL;
        ui->computeSizingFieldButton->setEnabled(false);
    }
    else{
        std::cout << "volume index = " << index << std::endl;
        this->volume = MainWindow::dataManager()->volumes()[index];
        ui->computeSizingFieldButton->setEnabled(true);
    }
}

void SizingFieldWidget::dragEnterEvent(QDragEnterEvent *event)
{
  //  if (event->mimeData()->hasFormat("text/plain"))
        event->acceptProposedAction();
}

void SizingFieldWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
//    if (event->mimeData()->hasFormat("text/plain"))
    //event->
}

void SizingFieldWidget::dropEvent(QDropEvent *event)
{
//    textBrowser->setPlainText(event->mimeData()->text());
//         mimeTypeCombo->clear();
//         mimeTypeCombo->addItems(event->mimeData()->formats());

    event->acceptProposedAction();
}
