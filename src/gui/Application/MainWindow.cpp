#include "MainWindow.h"

#include <Cleaver/Cleaver.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/InverseField.h>
#include <nrrd2cleaver/nrrd2cleaver.h>

MainWindow* MainWindow::m_instance = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    m_instance = this;
}

MainWindow::MainWindow(const QString &title)
{
    m_instance = this;
    setWindowTitle(title);

    // Create Work Space
    m_workspace = new QMdiArea(this);
    setCentralWidget(m_workspace);

    setMinimumWidth(1024);

    // Create Data Manager
    m_dataManager = new DataManager();

    // Create Menus/Windows
    createDockWindows();
    createActions();
    createMenus();

    // Setup handling of windows
    connect(m_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(focus(QMdiSubWindow*)));
    connect(m_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)), m_meshViewOptionsWidget, SLOT(focus(QMdiSubWindow*)));
    connect(m_workspace, SIGNAL(subWindowActivated(QMdiSubWindow *)), m_cleaverWidget, SLOT(focus(QMdiSubWindow*)));
    connect(m_workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), m_sizingFieldWidget, SLOT(focus(QMdiSubWindow*)));
    connect(m_workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), m_dataManagerWidget, SLOT(focus(QMdiSubWindow*)));
    m_iNumOpenWindows = 0;
}


void MainWindow::createDockWindows()
{
    m_dataManagerWidget = new DataManagerWidget(this);

    m_cleaverWidget = new CleaverWidget(this);
    m_sizingFieldWidget = new SizingFieldWidget(this);
    m_meshViewOptionsWidget = new MeshViewOptionsWidget(this);

    addDockWidget(Qt::LeftDockWidgetArea, m_sizingFieldWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_cleaverWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_dataManagerWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_meshViewOptionsWidget);
}


void MainWindow::createActions()
{
    // File Menu Actions
    importVolumeAct = new QAction(tr("Import &Volume"), this);
    importVolumeAct->setShortcut(tr("Ctrl+v"));
    connect(importVolumeAct, SIGNAL(triggered()), this, SLOT(importVolume()));

    importSizingFieldAct = new QAction(tr("Import Sizing &Field"), this);
    importSizingFieldAct->setShortcut(tr("Ctrl+f"));
    connect(importSizingFieldAct, SIGNAL(triggered()), this, SLOT(importSizingField()));
    importSizingFieldAct->setDisabled(true);

    importMeshAct = new QAction(tr("Import &Mesh"), this);
    importMeshAct->setShortcut(tr("Ctrl+m"));
    connect(importMeshAct, SIGNAL(triggered()), this, SLOT(importMesh()));

    closeAct = new QAction(tr("&Close"), this);
    connect(closeAct, SIGNAL(triggered()), this, SLOT(closeSubWindow()));
    closeAct->setDisabled(true);

    closeAllAct = new QAction(tr("Close &All"), this);
    connect(closeAllAct, SIGNAL(triggered()), this, SLOT(closeAllSubWindows()));
    closeAllAct->setDisabled(true);

    exportAct = new QAction(tr("&Export Mesh"), this);
    exportAct->setShortcut(tr("Ctrl+S"));
    exportAct->setDisabled(true);
    connect(exportAct, SIGNAL(triggered()), this, SLOT(exportMesh()));


    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    // Edit Menu Actions
    removeExternalTetsAct = new QAction(tr("Remove &External Tets"), this);
    connect(removeExternalTetsAct, SIGNAL(triggered()), this, SLOT(removeExternalTets()));

    removeLockedTetsAct = new QAction(tr("Remove &Locked Tets"), this);
    connect(removeLockedTetsAct, SIGNAL(triggered()), this, SLOT(removeLockedTets()));

    // Compute Menu Actions
    computeAnglesAct = new QAction(tr("Dihedral Angles"), this);
    connect(computeAnglesAct, SIGNAL(triggered()), this, SLOT(computeMeshAngles()));

    // View Menu Actions
    resetCameraAct = new QAction(tr("&Reset Camera"), this);
    resetCameraAct->setDisabled(true);
    connect(resetCameraAct, SIGNAL(triggered()), this, SLOT(resetCamera()));

    saveCameraAct = new QAction(tr("&Save Camera"), this);
    saveCameraAct->setDisabled(true);
    connect(saveCameraAct, SIGNAL(triggered()), this, SLOT(saveCamera()));

    loadCameraAct = new QAction(tr("&Load Camera"), this);
    loadCameraAct->setDisabled(true);
    connect(loadCameraAct, SIGNAL(triggered()), this, SLOT(loadCamera()));


    // Tool Menu Actions    
    cleaverAction = m_cleaverWidget->toggleViewAction();
    cleaverAction->setCheckable(true);
    meshViewOptionsAction = m_meshViewOptionsWidget->toggleViewAction();
    meshViewOptionsAction->setCheckable(true);

    sizingFieldAction = m_sizingFieldWidget->toggleViewAction();
    sizingFieldAction->setCheckable(true);


    /*
    cameraActGroup = new QActionGroup(this);
    perspectiveViewAct = new QAction("Perspective View", this);
    orthographicViewAct = new QAction("Orthographic View", this);
    perspectiveViewAct->setCheckable(true);
    perspectiveViewAct->setChecked(true);
    orthographicViewAct->setCheckable(true);
    cameraActGroup->addAction(perspectiveViewAct);
    cameraActGroup->addAction(orthographicViewAct);
    */

    // Windows Menu Actions
    /*
    cascadeAct = new QAction(tr("Cascade Windows"), this);
    cascadeAct->setDisabled(true);
    connect(cascadeAct, SIGNAL(triggered()), m_workspace, SLOT(cascadeSubWindows()));

    tileAct = new QAction(tr("Tile Windows"), this);
    tileAct->setDisabled(true);
    connect(tileAct, SIGNAL(triggered()), m_workspace, SLOT(tileSubWindows()));
    */

    // About Menu Actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}


void MainWindow::createMenus()
{
    // Top Level Menus
    m_fileMenu = new QMenu(tr("&File"), this);
    m_editMenu = new QMenu(tr("&Edit"), this);
    m_computeMenu = new QMenu(tr("&Compute"), this);
    m_viewMenu = new QMenu(tr("&View"), this);
    m_toolsMenu = new QMenu(tr("&Tools"), this);
    m_windowsMenu = new QMenu(tr("&Windows"), this);
    m_helpMenu = new QMenu(tr("&Help"), this);

    // File Menu Actions
    m_fileMenu->addAction(importVolumeAct);
    m_fileMenu->addAction(importSizingFieldAct);
    m_fileMenu->addAction(importMeshAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(exportAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(closeAct);
    m_fileMenu->addAction(closeAllAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(exitAct);

    // Edit Menu Actions
    m_editMenu->addAction(removeExternalTetsAct);
    m_editMenu->addAction(removeLockedTetsAct);

    // Compute Menu Actions
    m_computeMenu->addAction(computeAnglesAct);

    // View Menu Actions
    m_viewMenu->addAction(resetCameraAct);
    m_viewMenu->addAction(saveCameraAct);
    m_viewMenu->addAction(loadCameraAct);

    // Tool Menu Actions
    m_toolsMenu->addAction(sizingFieldAction);
    m_toolsMenu->addSeparator();    
    m_toolsMenu->addAction(cleaverAction);
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction(meshViewOptionsAction);
    m_toolsMenu->addSeparator();

    // Help Menu Actions
    m_helpMenu->addAction(aboutAct);

    // Add Menus To MenuBar
    menuBar()->addMenu(m_fileMenu);
    menuBar()->addMenu(m_editMenu);
    menuBar()->addMenu(m_computeMenu);
    menuBar()->addMenu(m_viewMenu);
    menuBar()->addMenu(m_toolsMenu);
    menuBar()->addMenu(m_windowsMenu);
    menuBar()->addMenu(m_helpMenu);
}

//--------------------------------------
// - removeExternalTets()
// This method grabs the current window
// and if it has a mesh, calls the method
// on the mesh to remove external tets.
//--------------------------------------
void MainWindow::removeExternalTets()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        cleaver::TetMesh *mesh   = window->mesh();
        cleaver::Volume  *volume = window->volume();
        if(mesh && volume)
        {
            cleaver::stripExteriorTets(mesh,volume,true);

            // recompute update adjacency
            mesh->constructFaces();
            mesh->constructBottomUpIncidences();

            //mesh->removeExternalTets(); // make it so old way
            window->setMesh(mesh);      // trigger update
        }
    }
}

//--------------------------------------
// - removeCementedTets()
// This method grabs the current window
// and if it has a mesh, calls the method
// on the mesh to remove cemented tets.
//--------------------------------------
void MainWindow::removeLockedTets()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        cleaver::TetMesh *mesh = window->mesh();
        if(mesh)
        {
            mesh->removeLockedTets();   // make it so
            window->setMesh(mesh);      // trigger update
        }
    }
}

void MainWindow::computeMeshAngles()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        cleaver::TetMesh *mesh = window->mesh();
        if(mesh)
        {
            mesh->computeAngles();
            std::cout << "Min Angle: " << mesh->min_angle << " degrees." << std::endl;
            std::cout << "Max Angle: " << mesh->max_angle << " degrees." << std::endl;
        }
    }
}

void MainWindow::importVolume()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Indicator Functions"), QDir::currentPath(), tr("NRRD (*.nrrd)"));

    if(!fileNames.isEmpty())
    {
        std::vector<std::string> inputs;

        for(int i=0; i < fileNames.size(); i++){
            inputs.push_back(fileNames[i].toStdString());
        }

        std::vector<cleaver::AbstractScalarField*> fields = loadNRRDFiles(inputs, true);
        if(fields.empty()){
            std::cerr << "Failed to load image data." << std::endl;
            return;
        }
        else if(fields.size() == 1){
            fields.push_back(new cleaver::InverseScalarField(fields[0]));
            fields.back()->setName(fields[0]->name() + "-inverse");
        }

        // Add Fields to Data Manager
        for(size_t f=0; f < fields.size(); f++){
            dataManager()->addField(fields[f]);
        }

        cleaver::Volume *volume = new cleaver::Volume(fields);

        static int v = 0;
        std::string volumeName = std::string("Volume");
        if(v > 0){
            volumeName += std::string(" ") + QString::number(v).toStdString();
        }
        volume->setName(volumeName);

        dataManager()->addVolume(volume);
        createWindow(volume, QString(volumeName.c_str()));
    }
}

void MainWindow::importSizingField()
{
    MeshWindow *window = activeWindow();
    if(window == NULL)
        return;

    cleaver::Volume *volume = window->volume();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Sizing Field"), QDir::currentPath(), tr("NRRD (*.nrrd)"));

    if(!fileName.isEmpty())
    {
        cleaver::AbstractScalarField* sizingField = loadNRRDFile(fileName.toStdString(), true);
        volume->setSizingField(sizingField);
        std::cout << "Sizing Field Set" << std::endl;
    }
}

void MainWindow::importMesh()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import Tetgen Mesh"), QDir::currentPath(), tr("Tetgen Mesh (*.node *.ele)"));

    if(fileNames.size() == 2)
    {
        std::string elefilename;
        std::string nodefilename;

        for(int i=0; i < 2; i++)
        {
            std::string fn = fileNames[i].toStdString();
            if(fn.substr(fn.find_last_of(".") + 1) == "ele")
                elefilename = fn;
            else if(fn.substr(fn.find_last_of(".") + 1) == "node")
                nodefilename = fn;
            else
            {
                std::cerr << "Invalid input extension!\n" << std::endl;
                return;
            }
        }

        if(elefilename.length() > 0 && nodefilename.length() > 0)
        {
            cleaver::TetMesh *mesh =
                cleaver::TetMesh::createFromNodeElePair(nodefilename, elefilename,false);
            if(mesh == NULL){
                std::cerr << "Invalid Mesh" << std::endl;
                return;
            }


            mesh->constructFaces();
            mesh->constructBottomUpIncidences();
            mesh->imported = true;

            MainWindow::instance()->createWindow(mesh, QString("New Mesh"));
		MainWindow::instance()->m_meshViewOptionsWidget->setShowCutsCheckboxEnabled(false);
            m_dataManager->addMesh(mesh);
        }
    }
}


void MainWindow::exportField(cleaver::FloatField *field)
{
    std::cout << "Exporting Field!!" << std::endl;
    if(!field)
        return;

    QString ext;
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mesh As"), QDir::currentPath() + (std::string("/") + field->name()).c_str(), tr("NRRD (*.nrrd);"), &selectedFilter);

    QString filter1("NRRD (*.nrrd)");

    saveNRRDFile(field, std::string(fileName.toLatin1()));
}

void MainWindow::exportMesh(cleaver::TetMesh *mesh)
{
    // If no mesh selected, get active window mesh
    if(!mesh)
        mesh = activeWindow()->mesh();

    // If still no mesh, return (TODO: Error MessageBox)
    if(!mesh)
        return;

    QString ext;
    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mesh As"), QDir::currentPath() + (std::string("/") + mesh->name).c_str(), tr("Tetgen (*.node);;Surface PLY (*.ply);;Matlab (*.mat)"), &selectedFilter);


    QString filter1("Tetgen (*.node)");
    QString filter2("SCIRun (*.pts)");
    QString filter3("Surface PLY (*.ply)");
    QString filter4("Matlab (*.mat)");
    
    std::string f = fileName.toStdString();
    f = f.substr(0,f.rfind("."));

    if(selectedFilter == filter1){
        mesh->writeNodeEle(f, true, true);
    }
    else if(selectedFilter == filter2){
        mesh->writePtsEle(f, true);
    }
    else if(selectedFilter == filter3){
        mesh->writePly(f, true);
    }
    else if(selectedFilter == filter4){
        mesh->writeMatlab(f, true);
    }

}

void MainWindow::subWindowClosed()
{
    /*
    m_iNumOpenWindows--;
    if(m_iNumOpenWindows == 0){
        cascadeAct->setDisabled(true);
        tileAct->setDisabled(true);
        closeAct->setDisabled(true);
        closeAllAct->setDisabled(true);


        m_volume_info_widget->setVolumeName("");
        m_optimize_widget->clear();
        m_meshing_widget->clear();
    }
    */
}

void MainWindow::resetCamera()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        window->resetView();
    }
}

void MainWindow::saveCamera()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        window->saveView();
    }
}

void MainWindow::loadCamera()
{
    MeshWindow *window = activeWindow();
    if(window != NULL)
    {
        window->loadView();
    }
}

void MainWindow::closeSubWindow()
{
    m_workspace->closeActiveSubWindow();
}

void MainWindow::closeAllSubWindows()
{
    m_workspace->closeAllSubWindows();
}

void MainWindow::focus(QMdiSubWindow *subwindow)
{
    if (subwindow != NULL)
    {
        MeshWindow *window = qobject_cast<MeshWindow *>(subwindow->widget());

        if(window != NULL)
        {
            if(window->mesh() != NULL)
                exportAct->setEnabled(true);
            else
                exportAct->setEnabled(false);

            importSizingFieldAct->setEnabled(true);
            resetCameraAct->setEnabled(true);
            saveCameraAct->setEnabled(true);
            loadCameraAct->setEnabled(true);
        }
        else{
            importSizingFieldAct->setEnabled(false);
            exportAct->setEnabled(false);
            resetCameraAct->setEnabled(false);
            saveCameraAct->setEnabled(false);
            loadCameraAct->setEnabled(false);
        }
    }
    else{
        importSizingFieldAct->setEnabled(false);
        exportAct->setEnabled(false);
        resetCameraAct->setEnabled(false);
        saveCameraAct->setEnabled(false);
        loadCameraAct->setEnabled(false);
    }
}

void MainWindow::about()
{
    // TODO: Make this a better Modal Frame rather than MessageBox
    QMessageBox::about(this, tr("About Cleaver 2.0 Beta"),
                                         tr("<b>Cleaver 2.0 Beta</b><BR>"
                                            "<a href=\"http://www.sci.utah.edu/\">Scientific Computing & Imaging Institute</a><BR>"
                                            "<a href=\"http://www.cs.utah.edu/\">University of Utah, School of Computing</a><BR>"
                                            "<P><b>Author:</b> Jonathan Bronson"
                                            "<P>This program is provided AS IS with NO "
                                            "WARRANTY OF ANY KIND, INCLUDING THE WARRANTY"
                                            "OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
                                            "PARTICULAR PURPOSE."));
}

void MainWindow::createWindow(cleaver::Volume *volume, const QString &title)
{
    if(volume)
    {
        MeshWindow *window = new MeshWindow(this);
        window->setVolume(volume);
        QMdiSubWindow *sw = m_workspace->addSubWindow(window);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->showMaximized();
        closeAct->setEnabled(true);
        closeAllAct->setEnabled(true);

        QAction *windowAct = new QAction(title, window);
        connect(windowAct, SIGNAL(triggered()), window, SLOT(setFocus()));
        //connect(window, SIGNAL(closed()), m_instance, SLOT(subWindowClosed()));
        m_windowsMenu->addAction(windowAct);
        m_iNumOpenWindows++;

        m_workspace->setActiveSubWindow(sw);
    }
}

void MainWindow::enableMeshedVolumeOptions() {
	
	MainWindow::instance()->m_meshViewOptionsWidget->setShowCutsCheckboxEnabled(true);
	MainWindow::instance()->exportAct->setEnabled(true);
}

void MainWindow::createWindow(cleaver::TetMesh *mesh, const QString &title)
{
    if(mesh)
    {
        MeshWindow *window = new MeshWindow(this);
        window->setCameraType(Target);
        window->setMesh(mesh);
        QMdiSubWindow *sw = m_workspace->addSubWindow(window);
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->showMaximized();
        closeAct->setEnabled(true);
        closeAllAct->setEnabled(true);

        QAction *windowAct = new QAction(title, window);
        connect(windowAct, SIGNAL(triggered()), window, SLOT(setFocus()));
        //connect(window, SIGNAL(closed()), m_instance, SLOT(subWindowClosed()));
        m_windowsMenu->addAction(windowAct);
        m_iNumOpenWindows++;

        m_workspace->setActiveSubWindow(sw);
    }
}


MeshWindow* MainWindow::activeWindow() const
{
    QMdiSubWindow *window = m_workspace->activeSubWindow();

    if(window == NULL)
        return NULL;

    MeshWindow *widget = qobject_cast<MeshWindow *>(window->widget());

    return widget;
}

