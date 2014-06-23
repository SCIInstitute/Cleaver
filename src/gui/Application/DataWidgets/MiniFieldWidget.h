#ifndef MINIFIELDWIDGET_H
#define MINIFIELDWIDGET_H

#include <QWidget>
#include <Cleaver/ScalarField.h>

namespace Ui {
class MiniFieldWidget;
}

class MiniFieldWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit MiniFieldWidget(QWidget *parent = 0);
    MiniFieldWidget(cleaver::AbstractScalarField *field, QWidget *parent = 0);
    ~MiniFieldWidget();

    void setField(cleaver::AbstractScalarField *field);
    cleaver::AbstractScalarField* field() const {  return m_field; }

signals:

    void removeRequest(cleaver::AbstractScalarField*);
    void changeRequest(cleaver::AbstractScalarField*);

protected:

    void paintEvent(QPaintEvent *);

    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);


    void startDrag(Qt::DropActions supportedActions);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

    
private:
    Ui::MiniFieldWidget *ui;
    cleaver::AbstractScalarField *m_field;

    std::string normalStyle;
    std::string dragEnterStyle;
};

#endif // MINIFIELDWIDGET_H
