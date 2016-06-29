#include "FieldDataWidget.h"
#include "ui_FieldDataWidget.h"
#include <sstream>
#include "MainWindow.h"
#include <Cleaver/ScalarField.h>
#include <Cleaver/BoundingBox.h>

FieldDataWidget::FieldDataWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FieldDataWidget),
    field(NULL)
{
    ui->setupUi(this);

    this->ui->infoWidget->hide();

    QObject::connect(this->ui->detailViewButton, SIGNAL(clicked(bool)),
            this, SLOT(showInfoClicked(bool)));


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


FieldDataWidget::FieldDataWidget(cleaver::AbstractScalarField *field, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FieldDataWidget),
    field(field)
{    
    ui->setupUi(this);
    ui->dimensionsWidget->layout()->setMargin(0);
    ui->dimensionsWidget->layout()->setSpacing(4);
    ui->originWidget->layout()->setMargin(0);
    ui->originWidget->layout()->setSpacing(4);
    ui->spacingWidget->layout()->setMargin(0);
    ui->spacingWidget->layout()->setSpacing(4);
    ui->infoWidget->layout()->setMargin(4);
    ui->infoWidget->layout()->setSpacing(4);


    this->ui->infoWidget->hide();

    //QObject::connect(this->ui->infoButton, SIGNAL(clicked(bool)),
    //        this, SLOT(showInfoClicked(bool)));

    QObject::connect(this->ui->detailViewButton, SIGNAL(clicked(bool)),
            this, SLOT(showInfoClicked(bool)));

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
    #ifdef __APPLE__
    "          font-size: 13;                       "
    #endif
    "          border-bottom-right-radius: 10px;    "
    "        }                                      ";

    selectedInfoStyle =
    "QWidget { background-color: rgb(205, 220, 255);"
    "          border: 1px solid black;             "
    "          border-top: none;                    "
    "          border-bottom-left-radius: 10px;     "
    "          border-bottom-right-radius: 10px;    "
    "        }                                      ";


    ui->headerWidget->setStyleSheet(closedStyle.c_str());

    //-------------------------------------------------//
    //             Set Widget Title                    //
    //-------------------------------------------------//
    setTitle(field->name());

    //-------------------------------------------------//
    //             Set Dimensions Label                //
    //-------------------------------------------------//
    std::stringstream ss;
    ss << "[" << field->bounds().size.x << ", " << field->bounds().size.y << ", " << field->bounds().size.z << "]";
    std::string dimensionsString = ss.str();
    ui->dimensionsValueLabel->setText(dimensionsString.c_str());

    //-------------------------------------------------//
    //             Set Spacing Label                   //
    //-------------------------------------------------//

    cleaver::FloatField *floatField = dynamic_cast<cleaver::FloatField*>(field);
    ss.clear();
    ss.str("");

    if(floatField)
        ss << floatField->scale().toString();
    else
        ss << "Custom";

    std::string spacingString = ss.str();
    ui->spacingValueLabel->setText(spacingString.c_str());

    selected = false;
    open = false;
}

FieldDataWidget::~FieldDataWidget()
{
    delete ui;
}



void FieldDataWidget::showInfoClicked(bool checked)
{
    open = checked;
    updateStyleSheet();

    if(checked)
        ui->infoWidget->show();
    else
        ui->infoWidget->hide();
}

void FieldDataWidget::setSelected(bool value)
{
    selected = value;
    updateStyleSheet();
}

void FieldDataWidget::setDataName(const QString &name)
{
    field->setName(std::string(name.toLatin1()));
}

void FieldDataWidget::setTitle(const std::string &title)
{
    ui->dataLabel->setText(title.c_str());
}


void FieldDataWidget::updateStyleSheet()
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
        }
        else
        {
            // update header
            ui->headerWidget->setStyleSheet(openStyle.c_str());

            // update info
            ui->infoWidget->setStyleSheet(normalInfoStyle.c_str());
        }

    }
}


//============================================
//  Event Handlers
//============================================

void FieldDataWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!event->modifiers().testFlag(Qt::ControlModifier))
        ui->detailViewButton->click();
}


void FieldDataWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        //if(event->modifiers().testFlag(Qt::ControlModifier))
        //    MainWindow::dataManager()->toggleAddSelection(reinterpret_cast<ulong>(field));
        //else
        //    MainWindow::dataManager()->setSelection(reinterpret_cast<ulong>(field));


        updateStyleSheet();
    }

    else if(event->button() == Qt::RightButton)
    {
        QMenu contextMenu;
        QAction *exportAction = contextMenu.addAction("Export Field");
        QAction *deleteAction = contextMenu.addAction("Delete Field");
        QAction *renameAction = contextMenu.addAction("Rename Field");

        cleaver::FloatField *floatField = dynamic_cast<cleaver::FloatField*>(field);
        if(!floatField)
            exportAction->setDisabled(true);

        QAction *selectedItem = contextMenu.exec(mapToGlobal(event->pos()));
        if(selectedItem)
        {
            if(selectedItem == deleteAction){
                //MainWindow::dataManager()->removeField(field);
            } else if(selectedItem == renameAction){

                QDialog dialog;
                dialog.setWindowTitle("Rename Field");
                dialog.resize(250,30);

                QVBoxLayout layout(&dialog);
                layout.setMargin(0);
                QLineEdit lineEdit;
                lineEdit.setText(ui->dataLabel->text());
                lineEdit.resize(ui->dataLabel->size().width(), ui->headerWidget->width() - 55);

                layout.addWidget(&lineEdit);
                QObject::connect(&lineEdit, SIGNAL(returnPressed()), &dialog, SLOT(accept()));

                if(dialog.exec() == QDialog::Accepted){
                    field->setName(std::string(lineEdit.text().toLatin1()));
                    ui->dataLabel->setText(lineEdit.text());
                }                
            }
            else if(selectedItem == exportAction){

                cleaver::FloatField *floatField = dynamic_cast<cleaver::FloatField*>(field);
                if(!floatField)
                {
                    // give error
                }
                //this->exportField(floatField);
            }
        }
    }
}

void FieldDataWidget::mouseReleaseEvent(QMouseEvent *event)
{

}

void FieldDataWidget::mouseMoveEvent(QMouseEvent *event)
{
    quint64 field_ptr = reinterpret_cast<quint64>(field);

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << field_ptr;

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("data/scalar-field", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    QPixmap pixmap(this->size());

    this->render(&pixmap); // offset, region);

    drag->setPixmap(pixmap);

    Qt::DropAction dropAction = drag->exec();
}

 void FieldDataWidget::startDrag(Qt::DropActions /*supportedActions*/)
 {
     std::cout << "HAHAHA" << std::endl;
 }
