#include "MiniFieldWidget.h"
#include "ui_MiniFieldWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QMimeData>
#include <QDrag>
#include <QDialog>

MiniFieldWidget::MiniFieldWidget(QWidget *parent) :
    QWidget(parent),
    m_field(NULL),
    ui(new Ui::MiniFieldWidget)
{
    ui->setupUi(this);     

    //setAcceptDrops(true);

    normalStyle = "background-color: rgb(240, 240, 240); border: 1px solid black; border-top: 1px solid black; border-radius: 10px;";
    dragEnterStyle = "background-color: rgb(240, 240, 128); border: 1px solid black; border-top: 1px solid black; border-radius: 10px;";
}

MiniFieldWidget::MiniFieldWidget(cleaver::AbstractScalarField *field, QWidget *parent) :
    QWidget(parent),
    m_field(NULL),
    ui(new Ui::MiniFieldWidget)
{
    ui->setupUi(this);

    //setAcceptDrops(true);

    normalStyle = "background-color: rgb(240, 240, 240); border: 1px solid black; border-top: 1px solid black; border-radius: 10px;";
    dragEnterStyle = "background-color: rgb(240, 240, 128); border: 1px solid black; border-top: 1px solid black; border-radius: 10px;";

    setField(field);
}

MiniFieldWidget::~MiniFieldWidget()
{
    delete ui;
}

void MiniFieldWidget::setField(cleaver::AbstractScalarField *field)
{
    m_field = field;

    if(field)
        ui->label->setText(field->name().c_str());
    else
        ui->label->setText("");
}

//----------------------------------------------
// This method must be override since default
// QWidget paintEvent does not support Styleheets
//----------------------------------------------
void MiniFieldWidget::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }


void MiniFieldWidget::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton)
    {
        /*
        if(event->modifiers().testFlag(Qt::ControlModifier))
            MainWindow::dataManager()->toggleAddSelection(reinterpret_cast<ulong>(volume));
        else
            MainWindow::dataManager()->setSelection(reinterpret_cast<ulong>(volume));


        updateStyleSheet();
        */
    }

    else if(event->button() == Qt::RightButton)
    {
        if(m_field)
        {
            QMenu contextMenu;
            QAction *removeAction = contextMenu.addAction("Remove Field");
            QAction *selectedItem = contextMenu.exec(mapToGlobal(event->pos()));
            if(selectedItem)
            {
                if(selectedItem == removeAction)
                {
                    emit removeRequest(m_field);
                }
            }
        }
    }
}

void MiniFieldWidget::mouseReleaseEvent(QMouseEvent *event)
{

}

void MiniFieldWidget::mouseMoveEvent(QMouseEvent *event)
{
    quint64 field_ptr = reinterpret_cast<quint64>(m_field);

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << field_ptr;

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("data/scalar-field", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    QPixmap pixmap(this->size());
    //QPoint offset(30,0);
    //QRegion region(30, 0, this->width-30, this->height());

    this->render(&pixmap); // offset, region);

    drag->setPixmap(pixmap);

    // makes it looks like element has been picked up
//    this->setMinimumHeight(0);
//    this->setMaximumHeight(0);

    Qt::DropAction dropAction = drag->exec();

}

void MiniFieldWidget::startDrag(Qt::DropActions supportedActions)
{

}

void MiniFieldWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("data/scalar-field"))
    {
        this->setStyleSheet(dragEnterStyle.c_str());
        event->accept();
    }
    else
        event->ignore();
}

void MiniFieldWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
//    if (event->mimeData()->hasFormat("text/plain"))
    //event->
    std::cout << "Leave Event!" << std::endl;
    this->setStyleSheet(normalStyle.c_str());
}

void MiniFieldWidget::dropEvent(QDropEvent *event)
{
    this->setStyleSheet(normalStyle.c_str());

    if (event->mimeData()->hasFormat("data/scalar-field"))
    {
        QByteArray itemData = event->mimeData()->data("data/scalar-field");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);
        quint64 field_ptr;
        dataStream >> field_ptr;

        emit changeRequest(reinterpret_cast<cleaver::AbstractScalarField*>(field_ptr));

        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
        event->ignore();


}
