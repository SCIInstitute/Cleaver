#include "VolumeDataWidget.h"
#include "ui_VolumeDataWidget.h"
#include <sstream>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenu>
#include <MainWindow.h>

VolumeDataWidget::VolumeDataWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumeDataWidget),
    volume(NULL)
{
    ui->setupUi(this);

    this->ui->infoWidget->hide();

    this->ui->sizingFieldWidget->setAcceptDrops(true);

    QObject::connect(this->ui->detailViewButton, SIGNAL(clicked(bool)),
            this, SLOT(showInfoClicked(bool)));

    //QObject::connect(ui->dataLabel, SIGNAL(lo)


    openStyle =
    "QWidget { background-color:rgb(177, 177, 177); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n       "
    "        }                                        ";

    closedStyle =
    "QWidget { background-color:rgb(177, 177, 177); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n\n\n   "
    "          border-bottom-left-radius: 10px;\n     "
    "          border-bottom-right-radius: 10px;\n    "
    "         }                                       ";

    ui->headerWidget->setStyleSheet(closedStyle.c_str());

    selected = false;
    open = false;
}

VolumeDataWidget::VolumeDataWidget(cleaver::Volume *volume, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VolumeDataWidget),
    volume(volume)
{
    ui->setupUi(this);
    ui->dimensionsWidget->layout()->setMargin(0);
    ui->dimensionsWidget->layout()->setSpacing(4);
    ui->originWidget->layout()->setMargin(0);
    ui->originWidget->layout()->setSpacing(4);
    ui->materialsWidget->layout()->setMargin(0);
    ui->materialsWidget->layout()->setSpacing(4);
    ui->infoWidget->layout()->setMargin(4);
    ui->infoWidget->layout()->setSpacing(1);


    this->ui->infoWidget->hide();

    this->ui->sizingFieldWidget->setAcceptDrops(true);

    //this->ui->materialsWidget->setAcceptDrops(true);
    //this->ui->materialsWidget->installEventFilter(this);

    connect(this->ui->detailViewButton, SIGNAL(clicked(bool)), this, SLOT(showInfoClicked(bool)));

    connect(this->ui->sizingFieldWidget, SIGNAL(changeRequest(cleaver::AbstractScalarField*)), this, SLOT(replaceSizingField(cleaver::AbstractScalarField*)));
    connect(this->ui->sizingFieldWidget, SIGNAL(removeRequest(cleaver::AbstractScalarField*)), this, SLOT(removeSizingField(cleaver::AbstractScalarField*)));

    //-------------------------------------------------//
    //            Construct The Styles                 //
    //-------------------------------------------------//

    openStyle =
    "QWidget { background-color:rgb(177, 177, 177); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n       "
    "        }                                        ";

    closedStyle =
    "QWidget { background-color:rgb(177, 177, 177); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n\n\n   "
    "          border-bottom-left-radius: 10px;\n     "
    "          border-bottom-right-radius: 10px;\n    "
    "         }                                       ";

    selectedOpenStyle =
    //"QWidget { background-color:rgb(109, 124, 152); \n"
    "QWidget { background-color:rgb(137, 156, 191); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n       "
    "        }                                        ";


    selectedClosedStyle =
    //"QWidget { background-color:rgb(109, 124, 152); \n"
    "QWidget { background-color:rgb(137, 156, 191); \n"
    "          border: 1px solid black;\n             "
    "          border-top-left-radius: 10px;\n        "
    "          border-top-right-radius: 10px;\n\n\n   "
    "          border-bottom-left-radius: 10px;\n     "
    "          border-bottom-right-radius: 10px;\n    "
    "         }                                       ";


    normalInfoStyle =
    "QWidget { background-color: rgb(228, 228, 228);"
    "          border: 1px solid black;             "
    "          border-top: none;                    "
    "          border-bottom-left-radius: 10px;     "
    "          border-bottom-right-radius: 10px;    "
    "        }                                      ";

    selectedInfoStyle =
    "QWidget { background-color: rgb(205, 220, 255);"
    "          border: 1px solid black;             "
    "          border-top: none;                    "
    "          border-bottom-left-radius: 10px;     "
    "          border-bottom-right-radius: 10px;    "
    "        }                                      ";

    highlightedInfoStyle =
    "QWidget { background-color: rgb(240, 240, 128);"
    "          border: 1px solid black;             "
    "          border-top: none;                    "
    "          border-bottom-left-radius: 10px;     "
    "          border-bottom-right-radius: 10px;    "
    "        }                                      ";


//    normalMaterialWidgetStyle =
//    "QWidget  {  background-color: rgb(240, 240, 240);"
//    "           border: 1.0px solid black;           "
//    "           border-radius: 10px;                 "
//    "        }                                       ";

    selectedMaterialWidgetStyle =
    //"QLabel  {  background-color: rgb(208, 255, 181);"
    "QWidget  {  background-color: rgb(240, 240, 240);"
    "           border: 1.0px solid black;           "
    "           border-radius: 10px;                 "
    "        }                                       ";

    //materialLabelStyle =
    //"border: none; background-color: rgba(255, 255, 255, 0);";

    ui->headerWidget->setStyleSheet(closedStyle.c_str());


    //-------------------------------------------------//
    //             Set Widget Title                    //
    //-------------------------------------------------//
    setTitle(volume->name());

    //-------------------------------------------------//
    //             Set Dimensions Label                //
    //-------------------------------------------------//
    updateBoundsLabels();

    //-------------------------------------------------//
    //             Set Materials Label                 //
    //-------------------------------------------------//
    updateMaterialsCountLabel();

    //-------------------------------------------------//
    //             Create Materials Widgets            //
    //-------------------------------------------------//
    updateMaterialsWidgets();

    //-------------------------------------------------//
    //              Set Sizing Field Widget
    //-------------------------------------------------//
    if(volume->getSizingField())
        ui->sizingFieldWidget->setField(volume->getSizingField());
    else
        ui->sizingFieldWidget->setField(NULL);

    selected = false;
    open = false;
}

VolumeDataWidget::~VolumeDataWidget()
{
    delete ui;
}

void VolumeDataWidget::updateBoundsLabels()
{
    if(this->volume->numberOfMaterials() > 0 || volume->getSizingField())
    {
        std::stringstream ss;
        ss << "[" << volume->width() << ", " << volume->height() << ", " << volume->depth() << "]";
        std::string dimensionsString = ss.str();
        ui->dimensionsValueLabel->setText(dimensionsString.c_str());
        ui->originValueLabel->setText(volume->bounds().origin.toString().c_str());
    }
    else
    {
        ui->dimensionsValueLabel->setText("null");
        ui->originValueLabel->setText("null");
    }
}

void VolumeDataWidget::updateMaterialsCountLabel()
{
    std::stringstream ss;
    ss << volume->numberOfMaterials();
    std::string materialsString = ss.str();
    ui->materialsCountValueLabel->setText(materialsString.c_str());
}

void VolumeDataWidget::updateMaterialsWidgets()
{
    // clear dummy widget
    if(ui->dummyMaterial){
        delete ui->dummyMaterial;
        ui->dummyMaterial = NULL;
    }

    MaterialMap tmp_material_map = this->materialMap;
    this->materialMap.clear();

    for(int m=0; m < volume->numberOfMaterials(); m++)
    {
        cleaver::AbstractScalarField *field = volume->getMaterial(m);
        MiniFieldWidget *materialWidget = NULL;

        MaterialMap::iterator iter = tmp_material_map.find(reinterpret_cast<ulong>(field));
        if (iter != tmp_material_map.end())
        {
            // found existing
            materialWidget = iter->second;

            // update text label
            materialWidget->setField(field);

            // remove from temporary
            tmp_material_map.erase(iter);
        }
        else{

            // create a material widget
            materialWidget = new MiniFieldWidget(field, this);
            connect(materialWidget, SIGNAL(removeRequest(cleaver::AbstractScalarField*)), this, SLOT(removeMaterial(cleaver::AbstractScalarField*)));
//            materialWidget->setStyleSheet(normalMaterialWidgetStyle.c_str());
//            materialWidget->setLayout(new QHBoxLayout());
//            materialWidget->layout()->setMargin(0);
//            materialWidget->layout()->setContentsMargins(10, 0, 10, 0);
//            materialWidget->layout()->setSpacing(0);
//            materialWidget->setMinimumHeight(20);
//            materialWidget->setMaximumHeight(20);

            // create label widget
//            QLabel *labelWidget = new QLabel(materialWidget);
//            labelWidget->setStyleSheet(materialLabelStyle.c_str());
//            labelWidget->setText(volume->getMaterial(m)->name().c_str());
//            labelWidget->setMinimumHeight(20);

            // Put the widget in the layout
            //materialWidget->layout()->addWidget(labelWidget);
            ui->materialsWidget->layout()->addWidget(materialWidget);
        }



        //vbox->insertWidget((int)m, materialWidget);
        materialMap[reinterpret_cast<ulong>(field)] = materialWidget;

        //ui->materialsWidget->adjustSize();
        //ui->infoWidget->adjustSize();

    }

    // For anything left in temporary map, they are no longer needed.
    // Remove them from the layout and delete them.

    for(MaterialMap::iterator iter = tmp_material_map.begin(); iter != tmp_material_map.end(); ++iter)
    {
        MiniFieldWidget *materialWidget = iter->second;
        ui->materialsWidget->layout()->removeWidget(materialWidget);
        delete materialWidget;
    }
}

void VolumeDataWidget::removeMaterial(cleaver::AbstractScalarField *field)
{
    volume->removeMaterial(field);
    updateFields();
}

void VolumeDataWidget::removeSizingField(cleaver::AbstractScalarField *field)
{
    volume->setSizingField(NULL);
    updateFields();
}

void VolumeDataWidget::replaceSizingField(cleaver::AbstractScalarField *field)
{
    volume->setSizingField(field);
    updateFields();
}

void VolumeDataWidget::showInfoClicked(bool checked)
{
    open = checked;
    updateStyleSheet();

    if(checked){
        ui->infoWidget->show();
        this->setAcceptDrops(true);
    }
    else{
        ui->infoWidget->hide();
        this->setAcceptDrops(false);
    }
}

void VolumeDataWidget::setSelected(bool value)
{
    selected = value;
    updateStyleSheet();
}

void VolumeDataWidget::setDataName(const QString &name)
{
    volume->setName(std::string(name.toLatin1()));
    MainWindow::dataManager()->update();
}

void VolumeDataWidget::setTitle(const std::string &title)
{
    ui->dataLabel->setText(title.c_str());
}


void VolumeDataWidget::updateStyleSheet()
{
    if(!open)
    {
        // update header
        if(selected)
            ui->headerWidget->setStyleSheet(selectedClosedStyle.c_str());
        else
            ui->headerWidget->setStyleSheet(closedStyle.c_str());
    }
    else{

        if(selected)
        {
            // update header
            ui->headerWidget->setStyleSheet(selectedOpenStyle.c_str());

            // update info
            ui->infoWidget->setStyleSheet(selectedInfoStyle.c_str());



            // update material widgets
            /*
            QObjectList children = ui->materialsWidget->children();
            for(size_t i=0; i < children.size(); i++)
            {
                MiniFieldWidget *materialWidget = qobject_cast<MiniFieldWidget*>(children[i]);
                if(materialWidget)
                    //materialWidget->setField(materialWidget->field());
                    materialWidget->setStyleSheet(selectedMaterialWidgetStyle.c_str());
            }
            */

        }
        else
        {
            // update header
            ui->headerWidget->setStyleSheet(openStyle.c_str());

            // update info
            ui->infoWidget->setStyleSheet(normalInfoStyle.c_str());

            // update material widgets
            /*
            QObjectList children = ui->materialsWidget->children();
            for(size_t i=0; i < children.size(); i++)
            {
                QWidget *materialWidget = qobject_cast<QWidget*>(children[i]);
                if(materialWidget)
                    materialWidget->setStyleSheet(normalMaterialWidgetStyle.c_str());
            }
            */
        }

    }
}

void VolumeDataWidget::updateFields()
{
    updateBoundsLabels();
    updateMaterialsCountLabel();
    updateMaterialsWidgets();

    //------------------------------------
    // Make sure sizing field matches
    //------------------------------------
    ui->sizingFieldWidget->setField(volume->getSizingField());
}

//============================================
//  Event Handlers
//============================================

void VolumeDataWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!event->modifiers().testFlag(Qt::ControlModifier))
        ui->detailViewButton->click();
}

void VolumeDataWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if(event->modifiers().testFlag(Qt::ControlModifier))
            MainWindow::dataManager()->toggleAddSelection(reinterpret_cast<ulong>(volume));
        else
            MainWindow::dataManager()->setSelection(reinterpret_cast<ulong>(volume));


        updateStyleSheet();
    }

    else if(event->button() == Qt::RightButton)
    {
        QMenu contextMenu;
        //QAction *deleteAction = contextMenu.addAction("Delete Volume");
        QAction *renameAction = contextMenu.addAction("Rename Volume");

        QAction *selectedItem = contextMenu.exec(mapToGlobal(event->pos()));
        if(selectedItem)
        {
//            if(selectedItem == deleteAction){
//                MainWindow::dataManager()->removeVolume(volume);
//            }
//            else
            if(selectedItem == renameAction){

                QDialog dialog;
                dialog.setWindowTitle("Rename Volume");
                dialog.resize(250,30);

                QVBoxLayout layout(&dialog);
                layout.setMargin(0);
                QLineEdit lineEdit;
                lineEdit.setText(ui->dataLabel->text());
                lineEdit.resize(ui->dataLabel->size().width(), ui->headerWidget->width() - 55);

                layout.addWidget(&lineEdit);
                QObject::connect(&lineEdit, SIGNAL(returnPressed()), &dialog, SLOT(accept()));

                if(dialog.exec() == QDialog::Accepted){
                    ui->dataLabel->setText(lineEdit.text());
                    MainWindow::dataManager()->update();
                }
            }
        }
    }
}

void VolumeDataWidget::mouseReleaseEvent(QMouseEvent *event)
{

}

void VolumeDataWidget::mouseMoveEvent(QMouseEvent *event)
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setText("text..");
    drag->setMimeData(mimeData);

    QPixmap pixmap(this->size());    
    //QPoint offset(30,0);
    //QRegion region(30, 0, this->width-30, this->height());

    this->render(&pixmap); // offset, region);

    drag->setPixmap(pixmap);

    Qt::DropAction dropAction = drag->exec();
}


void VolumeDataWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("data/scalar-field"))
    {
        ui->infoWidget->setStyleSheet(highlightedInfoStyle.c_str());
        event->accept();
    }
    else
        event->ignore();
}

void VolumeDataWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    if(selected)
        ui->infoWidget->setStyleSheet(selectedInfoStyle.c_str());
    else
        ui->infoWidget->setStyleSheet(normalInfoStyle.c_str());

    event->accept();
}

void VolumeDataWidget::dropEvent(QDropEvent *event)
{
    if(selected)
        ui->infoWidget->setStyleSheet(selectedInfoStyle.c_str());
    else
        ui->infoWidget->setStyleSheet(normalInfoStyle.c_str());

    if (event->mimeData()->hasFormat("data/scalar-field"))
    {
        QByteArray itemData = event->mimeData()->data("data/scalar-field");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);
        quint64 field_ptr;
        dataStream >> field_ptr;

        cleaver::AbstractScalarField *field = (reinterpret_cast<cleaver::AbstractScalarField*>(field_ptr));

        for(int m=0; m < volume->numberOfMaterials(); m++)
        {
            // check it's not already in the list
            if(volume->getMaterial(m) == field)
            {
                event->ignore();
                return;
            }
        }

        volume->addMaterial(field);
        MainWindow::dataManager()->update();


        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
        event->ignore();
}

bool VolumeDataWidget::eventFilter(QObject *target, QEvent *event)
{
    if(target == ui->materialsWidget)
    {
        if(event->type() == QEvent::DragEnter)
        {
            ui->materialsWidget->setStyleSheet("background-color: yellow;");
            return true;
        }
    }
    return QWidget::eventFilter(target, event);
}
