#ifndef MESHDATAWIDGET_H
#define MESHDATAWIDGET_H

#include <QFrame>
#include <string>
#include <Cleaver/TetMesh.h>

namespace Ui {
class MeshDataWidget;
}

class MeshDataWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit MeshDataWidget(QWidget *parent = 0);
    MeshDataWidget(cleaver::TetMesh *mesh, QWidget *parent = 0);
    ~MeshDataWidget();

    void setTitle(const std::string &title);


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

    Ui::MeshDataWidget *ui;

    std::string openStyle;
    std::string closedStyle;

    std::string selectedOpenStyle;
    std::string selectedClosedStyle;

    std::string normalInfoStyle;
    std::string selectedInfoStyle;

    cleaver::TetMesh *mesh;

    bool open;
    bool selected;
};

#endif // MESHDATAWIDGET_H
