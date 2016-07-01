#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QMainWindow>
#include <QMdiArea>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include "ViewWidgets/MeshWindow.h"
#include "ViewWidgets/MeshViewOptionsWidget.h"
#include "ToolWidgets/CleaverWidget.h"
#include "ToolWidgets/SizingFieldWidget.h"
#include "DataWidgets/DataManagerWidget.h"
#include <Cleaver/Cleaver.h>
#include <Cleaver/CleaverMesher.h>
#include "Data/DataManager.h"
#include <QProgressBar>


class MainWindow : public QMainWindow
{
  Q_OBJECT
  enum  CleaverGUIDataType {VOLUME, SIZING_FIELD, MESH};
public:
  MainWindow(const QString &title);
  ~MainWindow();
  explicit MainWindow(QWidget *parent = 0);
  MeshWindow * createWindow(const QString &title);
  void enableMeshedVolumeOptions();

  void disableAllActions();
  void enablePossibleActions();

public slots :
  void importVolume();
  void importSizingField();
  void importMesh();
  void exportField(cleaver::FloatField *field);
  void exportMesh(cleaver::TetMesh *mesh = NULL);
  void about();
  void handleNewMesh();
  void handleDoneMeshing();
  void handleNewData(CleaverGUIDataType type);
  void handleRepaintGL();
  void handleSizingFieldDone();
  void handleExportField(void*);
  void handleExportMesh(void*);
  void handleDisableSizingField();
  void handleDisableMeshing();
  void handleProgress(int);
  void handleError(std::string);
  void handleMessage(std::string);
  // edit functions
  void removeExternalTets();
  void removeLockedTets();
  // compute functions
  void computeMeshAngles();

private:
  void createDockWindows();
  void createActions();
  void createMenus();

private:
  MeshWindow *window_;
  MeshViewOptionsWidget *m_meshViewOptionsWidget;
  CleaverWidget *m_cleaverWidget;
  SizingFieldWidget *m_sizingFieldWidget;
  DataManagerWidget *m_dataManagerWidget;

  cleaver::CleaverMesher mesher_;

  // File Menu Actions
  QAction *importVolumeAct;
  QAction *importSizingFieldAct;
  QAction *importMeshAct;
  QAction *exitAct;

  QAction *exportAct;

  // Edit Menu Actions
  QAction *removeExternalTetsAct;
  QAction *removeLockedTetsAct;

  // Compute Menu Action
  QAction *computeAnglesAct;

  // Tool Menu Actions    
  QAction *cleaverAction;
  QAction *meshViewOptionsAction;
  QAction *sizingFieldAction;
  QAction *dataViewAction;

  // About Menu Actions
  QAction *aboutAct;

  // Top Level Menus
  QMenu *m_fileMenu;
  QMenu *m_editMenu;
  QMenu *m_viewMenu;
  QMenu *m_helpMenu;
  
  //status bar items
  QProgressBar * progressBar_;

  std::string lastPath_, exePath_, scirun_path_, python_path_;
};

#endif // MAINWINDOW_H
