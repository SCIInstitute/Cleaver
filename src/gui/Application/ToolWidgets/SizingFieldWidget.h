#ifndef DATALOADERWIDGET_H
#define DATALOADERWIDGET_H

#include <QDockWidget>
#include <Cleaver/CleaverMesher.h>
#include <QThread>

namespace Ui {
class SizingFieldWidget;
}

//thread class for cleaver widget
class SizingFieldThread : public QThread {
  Q_OBJECT
public:
  SizingFieldThread(cleaver::CleaverMesher& mesher,
    QObject * parent, float refinementFactor,
    float sizeMultiplier, float lipschitz, int padding, bool adapt);
  ~SizingFieldThread();
  void run();
signals:
  void sizingFieldDone();
  void message(std::string);
  void progress(int);
  void errorMessage(std::string);
private:
  cleaver::CleaverMesher& mesher_;
  float sizeMultiplier_, lipschitz_, refinementFactor_;
  int padding_;
  bool adapt_;
};


class SizingFieldWidget : public QDockWidget {
    Q_OBJECT
public:
    explicit SizingFieldWidget(
      cleaver::CleaverMesher& mesher,
      QWidget *parent = NULL);
    ~SizingFieldWidget();
    void setCreateButtonEnabled(bool b);
  signals:
    void sizingFieldDone();
    void message(std::string);
    void progress(int);
    void errorMessage(std::string);
public slots:
    void loadSizingField();
    void computeSizingField();
    void handleSizingFieldDone();
    void handleMessage(std::string);
    void handleProgress(int);
    void handleErrorMessage(std::string);

private:
    Ui::SizingFieldWidget *ui;
    cleaver::CleaverMesher& mesher_;
};

#endif // DATALOADERWIDGET_H
