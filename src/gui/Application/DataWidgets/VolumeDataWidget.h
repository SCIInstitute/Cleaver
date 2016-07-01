#ifndef VOLUMEDATAWIDGET_H
#define VOLUMEDATAWIDGET_H

#include <QWidget>
#include <QStyle>
#include <Cleaver/Volume.h>
#include "MiniFieldWidget.h"
#include <map>

typedef std::map< ulong, MiniFieldWidget* > MaterialMap;

namespace Ui {
class VolumeDataWidget;
}

class VolumeDataWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit VolumeDataWidget(QWidget *parent = 0);
    VolumeDataWidget(cleaver::Volume *volume, QWidget *parent = 0);
    ~VolumeDataWidget();
    

void setTitle(const std::string &title);
signals:
    void updateDataWidget();
public slots:

    void removeMaterial(cleaver::AbstractScalarField *field);
    void removeSizingField(cleaver::AbstractScalarField *field);
    void showInfoClicked(bool checked);
    void setSelected(bool value);
    void setDataName(const QString &name);
    void updateFields();

protected:

    void mouseDoubleClickEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

    bool eventFilter(QObject *target, QEvent *event);

private:

    void updateBoundsLabels();
    void updateMaterialsCountLabel();
    void updateMaterialsWidgets();
    void updateStyleSheet();

    Ui::VolumeDataWidget *ui;

    std::string openStyle;
    std::string closedStyle;

    std::string selectedOpenStyle;
    std::string selectedClosedStyle;

    std::string normalInfoStyle;
    std::string selectedInfoStyle;
    std::string highlightedInfoStyle;

    //std::string normalMaterialWidgetStyle;
    std::string selectedMaterialWidgetStyle;

    //std::string materialLabelStyle;

    cleaver::Volume *volume;

    MaterialMap materialMap;


    bool open;
    bool selected;
};

#endif // VOLUMEDATAWIDGET_H
