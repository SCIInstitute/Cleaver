#include "MeshDataWidget.h"
#include "ui_MeshDataWidget.h"
#include <iostream>
#include <sstream>
#include <QMouseEvent>
#include <QMenu>
#include <MainWindow.h>


MeshDataWidget::MeshDataWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MeshDataWidget),
    mesh(NULL)
{
    ui->setupUi(this);
    ui->nodesWidget->layout()->setMargin(0);
    ui->nodesWidget->layout()->setSpacing(4);
    ui->elementsWidget->layout()->setMargin(0);
    ui->elementsWidget->layout()->setSpacing(4);
    ui->minBoundsWidget->layout()->setMargin(0);
    ui->minBoundsWidget->layout()->setSpacing(4);
    ui->maxBoundsWidget->layout()->setMargin(4);
    ui->maxBoundsWidget->layout()->setSpacing(1);

    this->ui->infoWidget->hide();

    //QObject::connect(this->ui->infoButton, SIGNAL(clicked(bool)),
    //        this, SLOT(showInfoClicked(bool)));

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

MeshDataWidget::MeshDataWidget(cleaver::TetMesh *mesh, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MeshDataWidget),
    mesh(mesh)
{
    ui->setupUi(this);
    ui->nodesWidget->layout()->setMargin(0);
    ui->nodesWidget->layout()->setSpacing(4);
    ui->elementsWidget->layout()->setMargin(0);
    ui->elementsWidget->layout()->setSpacing(4);
    ui->minBoundsWidget->layout()->setMargin(0);
    ui->minBoundsWidget->layout()->setSpacing(4);
    ui->maxBoundsWidget->layout()->setMargin(0);
    ui->maxBoundsWidget->layout()->setSpacing(4);
    ui->infoWidget->layout()->setMargin(4);
    ui->infoWidget->layout()->setSpacing(1);

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
    setTitle(mesh->name);

    //-------------------------------------------------//
    //             Set Vertices Label                  //
    //-------------------------------------------------//
    std::stringstream ss;
    ss << mesh->verts.size();
    std::string vertString = ss.str();
    ui->vertsValueLabel->setText(vertString.c_str());

    //-------------------------------------------------//
    //             Set Tetrahera Label                 //
    //-------------------------------------------------//
    ss.clear(); ss.str("");
    ss << mesh->tets.size();
    std::string tetsString = ss.str();
    ui->tetsValueLabel->setText(tetsString.c_str());

    //-------------------------------------------------//
    //             Set Min BoundsLabel                 //
    //-------------------------------------------------//
    ss.clear(); ss.str("");
    ui->minBoundsValueLabel->setText(mesh->bounds.minCorner().toString().c_str());

    //-------------------------------------------------//
    //             Set Max BoundsLabel                 //
    //-------------------------------------------------//
    ss.clear(); ss.str("");
    ui->maxBoundsValueLabel->setText(mesh->bounds.maxCorner().toString().c_str());

    selected = false;
    open = false;

    //grabGesture(Qt::TapAndHoldGesture);      Takes too long
}



MeshDataWidget::~MeshDataWidget()
{
    delete ui;
}



void MeshDataWidget::showInfoClicked(bool checked)
{
    open = checked;

    if(checked){
        updateStyleSheet();
        ui->infoWidget->show();
    }
    else{
        updateStyleSheet();
        ui->infoWidget->hide();
    }
}

void MeshDataWidget::setSelected(bool value)
{
    selected = value;
    updateStyleSheet();
}

void MeshDataWidget::setDataName(const QString &name)
{
    mesh->name = std::string(name.toLatin1());
    MainWindow::dataManager()->update();
}

void MeshDataWidget::setTitle(const std::string &title)
{
    ui->dataLabel->setText(title.c_str());
}

/*
void MeshDataWidget::mousePressEvent(QMouseEvent *event)
{


    if (event->button() == Qt::LeftButton)// &&
            //iconLabel->geometry().contains(event->pos())
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
}
*/


void MeshDataWidget::updateStyleSheet()
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


void MeshDataWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!event->modifiers().testFlag(Qt::ControlModifier))
        ui->detailViewButton->click();
}

void MeshDataWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if(event->modifiers().testFlag(Qt::ControlModifier))
            MainWindow::dataManager()->toggleAddSelection(reinterpret_cast<ulong>(mesh));
        else
            MainWindow::dataManager()->setSelection(reinterpret_cast<ulong>(mesh));

        //selected = !selected;

        updateStyleSheet();
    }
    else if(event->button() == Qt::RightButton)
    {
        QMenu contextMenu;
        QAction *exportAction = contextMenu.addAction("Export Mesh");
        //QAction *deleteAction = contextMenu.addAction("Delete Mesh");
        QAction *renameAction = contextMenu.addAction("Rename Mesh");

        QAction *selectedItem = contextMenu.exec(mapToGlobal(event->pos()));
        if(selectedItem)
        {
//            if(selectedItem == deleteAction) {
//                MainWindow::dataManager()->removeMesh(this->mesh);
//            }
//            else
            if(selectedItem == renameAction){

                QDialog dialog;
                dialog.setWindowTitle("Rename Mesh");
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
            else if(selectedItem == exportAction){
                MainWindow::instance()->exportMesh(this->mesh);
            }
        }
    }
}

void MeshDataWidget::mouseReleaseEvent(QMouseEvent *event)
{

}

void MeshDataWidget::mouseMoveEvent(QMouseEvent *event)
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
