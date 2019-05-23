#ifndef FIELDDATAWIDGET_H
#define FIELDDATAWIDGET_H

#include <QWidget>
#include <QStyle>
#include <cleaver/ScalarField.h>

namespace Ui {
class FieldDataWidget;
}

class FieldDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FieldDataWidget(QWidget *parent = 0);
    FieldDataWidget(cleaver::AbstractScalarField *field, QWidget *parent = 0);
    ~FieldDataWidget();

    void setTitle(const std::string &title);
  signals:
    void exportField(void*);
public slots:

    void showInfoClicked(bool checked);
    void setSelected(bool value);
    void setDataName(const QString &name);

protected:

    void mouseDoubleClickEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

    void updateStyleSheet();

    void startDrag(Qt::DropActions /*supportedActions*/);

    Ui::FieldDataWidget *ui;

    std::string openStyle;
    std::string closedStyle;

    std::string selectedOpenStyle;
    std::string selectedClosedStyle;

    std::string normalInfoStyle;
    std::string selectedInfoStyle;

    cleaver::AbstractScalarField *field;

    bool open;
    bool selected;
};

#endif // FIELDDATAWIDGET_H
