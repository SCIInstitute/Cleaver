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
#include "Data/DataManager.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(const QString &title);
    explicit MainWindow(QWidget *parent = 0);

    static MainWindow* instance(){ return m_instance; }
    static DataManager* dataManager(){ return instance()->m_dataManager; }

    void createWindow(cleaver::Volume *volume, const QString &title);
    void createWindow(cleaver::TetMesh  *mesh, const QString &title);
    MeshWindow* activeWindow() const;
	void enableMeshedVolumeOptions();
    
signals:
    
public slots:

    void importVolume();
    void importSizingField();
    void importMesh();
    void exportField(cleaver::FloatField *field);
    void exportMesh(cleaver::TetMesh *mesh = NULL);
    void subWindowClosed();
    void closeSubWindow();
    void closeSubWindow(MeshWindow *win);
    void closeAllSubWindows();
    void focus(QMdiSubWindow*);
    void about();

    // camera slots
    void resetCamera();
    void saveCamera();
    void loadCamera();


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
    static MainWindow *m_instance;
    QMdiArea *m_workspace;

    DataManager *m_dataManager;
    MeshViewOptionsWidget *m_meshViewOptionsWidget;
    CleaverWidget *m_cleaverWidget;
    SizingFieldWidget *m_sizingFieldWidget;
    DataManagerWidget *m_dataManagerWidget;

    // File Menu Actions
    QAction *importVolumeAct;
    QAction *importSizingFieldAct;
    QAction *importMeshAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *exitAct;

    QAction *exportAct;

    // Edit Menu Actions
    QAction *removeExternalTetsAct;
    QAction *removeLockedTetsAct;

    // Compute Menu Action
    QAction *computeAnglesAct;

    // View Menu Actions
    QAction *resetCameraAct;
    QAction *saveCameraAct;
    QAction *loadCameraAct;

    // Tool Menu Actions    
    QAction *cleaverAction;
    QAction *meshViewOptionsAction;
    QAction *sizingFieldAction;

    // About Menu Actions
    QAction *aboutAct;

    // Top Level Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_computeMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_windowsMenu;
    QMenu *m_helpMenu;


    int m_iNumOpenWindows;
};

#endif // MAINWINDOW_H
