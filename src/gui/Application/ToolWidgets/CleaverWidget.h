#ifndef CLEAVERWIDGET_H
#define CLEAVERWIDGET_H

#include <QDockWidget>
#include <Cleaver/CleaverMesher.h>
#include <QThread>

Q_DECLARE_METATYPE(std::string)

namespace Ui {
class CleaverWidget;
}

//thread class for cleaver widget
class CleaverThread : public QThread {
  Q_OBJECT
public:
  CleaverThread(cleaver::CleaverMesher& mesher,
    QObject * parent = nullptr);
  ~CleaverThread();
  //------ run entire cleaving algorithm
  void run();
signals:
  void doneMeshing();
  void repaintGL();
  void newMesh();
  void message(std::string);
  void progress(int);
  void errorMessage(std::string);
private:
  cleaver::CleaverMesher& mesher_;
};

class CleaverWidget : public QDockWidget
{
    Q_OBJECT
public:
  explicit CleaverWidget(cleaver::CleaverMesher& mesher,
    QWidget *parent = NULL);
    ~CleaverWidget();
    void setMeshButtonEnabled(bool b);
  signals:
    void doneMeshing();
    void repaintGL();
    void newMesh();
    void message(std::string);
    void progress(int);
    void errorMessage(std::string);
public slots:
    void createMesh();
    void clear();
    void resetCheckboxes();
    void handleDoneMeshing();
    void handleRepaintGL();
    void handleNewMesh();
    void handleMessage(std::string);
    void handleProgress(int);
    void handleErrorMessage(std::string);
    
private:
    Ui::CleaverWidget *ui;
    cleaver::CleaverMesher& mesher_;
};

#endif // CLEAVERWIDGET_H
