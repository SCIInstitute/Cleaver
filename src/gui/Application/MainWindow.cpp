#include "MainWindow.h"
#include <Cleaver/Cleaver.h>
#include <Cleaver/ConstantField.h>
#include <Cleaver/InverseField.h>
#include <cstdio>
#include <fstream>
#include <QProgressDialog>
#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <NRRDTools.h>
#include <QStatusBar>
#include <QLayoutItem>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent) {}

MainWindow::MainWindow(const QString &title) : meshSaved_(true), sizingFieldSaved_(true)
{
  this->setWindowTitle(title);
  // Create Menus/Windows
  this->createDockWindows();
  this->createActions();
  this->setCentralWidget(this->window_);
  this->createMenus();

  exePath_ = QFileInfo( QCoreApplication::applicationFilePath()).path().toStdString();

  std::ifstream path((exePath_ + "/.path").c_str());
  if(path.is_open()) {
    path >> lastPath_ >> scirun_path_ >> python_path_;
    path.close();
  }
  this->enablePossibleActions();
}

MainWindow::~MainWindow() {
  std::ofstream path((exePath_ + "/.path").c_str());
  path << lastPath_ << std::endl << scirun_path_  << std::endl << python_path_;
  path.close();
}

void MainWindow::createDockWindows()
{
  this->window_ = this->createWindow("Mesh Window");
  this->window_->setMesher(&this->mesher_);
  this->m_dataManagerWidget = new DataManagerWidget(this);
  this->m_cleaverWidget = new CleaverWidget(this->mesher_, this);
  this->m_sizingFieldWidget = new SizingFieldWidget(this->mesher_, this);
  this->m_meshViewOptionsWidget =
    new MeshViewOptionsWidget(this->mesher_, this->window_, this);

  addDockWidget(Qt::LeftDockWidgetArea, this->m_sizingFieldWidget);
  addDockWidget(Qt::LeftDockWidgetArea, this->m_cleaverWidget);
  addDockWidget(Qt::RightDockWidgetArea, this->m_dataManagerWidget);
  addDockWidget(Qt::RightDockWidgetArea, this->m_meshViewOptionsWidget);
  this->progressBar_ = new QProgressBar(this);
  this->statusBar()->addPermanentWidget(this->progressBar_);
  this->statusBar()->show();
  this->progressBar_->setVisible(false);
  this->handleMessage("Program Cleaver started normally.");

  connect(this->m_cleaverWidget, SIGNAL(message(std::string)), this, SLOT(handleMessage(std::string)));
  connect(this->m_cleaverWidget, SIGNAL(errorMessage(std::string)), this, SLOT(handleError(std::string)));
  connect(this->m_cleaverWidget, SIGNAL(progress(int)), this, SLOT(handleProgress(int)));

  connect(this->m_sizingFieldWidget, SIGNAL(message(std::string)), this, SLOT(handleMessage(std::string)));
  connect(this->m_sizingFieldWidget, SIGNAL(errorMessage(std::string)), this, SLOT(handleError(std::string)));
  connect(this->m_sizingFieldWidget, SIGNAL(progress(int)), this, SLOT(handleProgress(int)));
}

void MainWindow::createActions()
{
  // File Menu Actions
  this->importVolumeAct = new QAction(tr("Import &Volume"), this);
  this->importVolumeAct->setShortcut(tr("Ctrl+v"));
  connect(this->importVolumeAct, SIGNAL(triggered()), this, SLOT(importVolume()));

  this->importSizingFieldAct = new QAction(tr("Import Sizing &Field"), this);
  this->importSizingFieldAct->setShortcut(tr("Ctrl+f"));
  connect(this->importSizingFieldAct, SIGNAL(triggered()), this, SLOT(importSizingField()));
  this->importSizingFieldAct->setDisabled(true);

  this->importMeshAct = new QAction(tr("Import &Mesh"), this);
  this->importMeshAct->setShortcut(tr("Ctrl+m"));
  connect(this->importMeshAct, SIGNAL(triggered()), this, SLOT(importMesh()));

  this->exportAct = new QAction(tr("&Export Mesh"), this);
  this->exportAct->setShortcut(tr("Ctrl+S"));
  this->exportAct->setDisabled(true);
  connect(this->exportAct, SIGNAL(triggered()), this, SLOT(exportMesh()));

  this->exitAct = new QAction(tr("E&xit"), this);
  this->exitAct->setShortcut(tr("Ctrl+Q"));
  connect(this->exitAct, SIGNAL(triggered()), this, SLOT(close()));

  // Edit Menu Actions
  this->removeExternalTetsAct = new QAction(tr("Remove &External Tets"), this);
  connect(this->removeExternalTetsAct, SIGNAL(triggered()), this, SLOT(removeExternalTets()));
  this->removeExternalTetsAct->setEnabled(false);

  this->removeLockedTetsAct = new QAction(tr("Remove &Locked Tets"), this);
  connect(this->removeLockedTetsAct, SIGNAL(triggered()), this, SLOT(removeLockedTets()));
  this->removeLockedTetsAct->setEnabled(false);

  // Compute Menu Actions
  this->computeAnglesAct = new QAction(tr("Dihedral Angles"), this);
  connect(this->computeAnglesAct, SIGNAL(triggered()), this, SLOT(computeMeshAngles()));
  this->computeAnglesAct->setEnabled(false);

  // Tool Menu Actions
  this->cleaverAction = this->m_cleaverWidget->toggleViewAction();
  this->cleaverAction->setCheckable(true);

  this->meshViewOptionsAction = this->m_meshViewOptionsWidget->toggleViewAction();
  this->meshViewOptionsAction->setCheckable(true);

  this->sizingFieldAction = this->m_sizingFieldWidget->toggleViewAction();
  this->sizingFieldAction->setCheckable(true);

  this->dataViewAction = this->m_dataManagerWidget->toggleViewAction();
  this->dataViewAction->setCheckable(true);

  // About Menu Actions
  this->aboutAct = new QAction(tr("&About"), this);
  this->aboutAct->setStatusTip(tr("Show the About box"));
  connect(this->aboutAct, SIGNAL(triggered()), this, SLOT(about()));

  //other actions
  connect(this->m_cleaverWidget, SIGNAL(doneMeshing()), this, SLOT(handleDoneMeshing()));
  connect(this->m_cleaverWidget, SIGNAL(newMesh()), this, SLOT(handleNewMesh()));
  connect(this->m_cleaverWidget, SIGNAL(repaintGL()), this, SLOT(handleRepaintGL()));
  connect(this->m_sizingFieldWidget, SIGNAL(sizingFieldDone()),
    this, SLOT(handleSizingFieldDone()));
  connect(this->m_dataManagerWidget, SIGNAL(exportField(void*)),
    this, SLOT(handleExportField(void*)));
  connect(this->m_dataManagerWidget, SIGNAL(exportMesh(void*)),
    this, SLOT(handleExportMesh(void*)));
  connect(this->m_dataManagerWidget, SIGNAL(disableMeshing()),
    this, SLOT(handleDisableMeshing()));
  connect(this->m_dataManagerWidget, SIGNAL(disableSizingField()),
    this, SLOT(handleDisableSizingField()));
}

void MainWindow::createMenus()
{
  // Top Level Menus
  m_fileMenu = new QMenu(tr("&File"), this);
  m_editMenu = new QMenu(tr("&Edit"), this);
  m_computeMenu = new QMenu(tr("&Compute"), this);
  m_viewMenu = new QMenu(tr("&View"), this);
  m_helpMenu = new QMenu(tr("&Help"), this);

  // File Menu Actions
  m_fileMenu->addAction(importVolumeAct);
  m_fileMenu->addAction(importSizingFieldAct);
  m_fileMenu->addAction(importMeshAct);
  m_fileMenu->addSeparator();
  m_fileMenu->addAction(exportAct);
  m_fileMenu->addSeparator();
  m_fileMenu->addSeparator();
  m_fileMenu->addAction(exitAct);

  // Edit Menu Actions
  m_editMenu->addAction(removeExternalTetsAct);
  m_editMenu->addAction(removeLockedTetsAct);
  
  // Compute Menu Actions
  m_computeMenu->addAction(computeAnglesAct);

  // View Menu Actions
  m_viewMenu->addAction(sizingFieldAction);
  m_viewMenu->addSeparator();
  m_viewMenu->addAction(cleaverAction);
  m_viewMenu->addSeparator();
  m_viewMenu->addAction(meshViewOptionsAction);
  m_viewMenu->addSeparator();
  m_viewMenu->addAction(dataViewAction);
  m_viewMenu->addSeparator();

  // Help Menu Actions
  m_helpMenu->addAction(aboutAct);

  // Add Menus To MenuBar
  menuBar()->addMenu(m_fileMenu);
  menuBar()->addMenu(m_editMenu);
  menuBar()->addMenu(m_computeMenu);
  menuBar()->addMenu(m_viewMenu);
  menuBar()->addMenu(m_helpMenu);
}

void MainWindow::handleSizingFieldDone() {
  // Add new sizing field to data manager
  this->m_dataManagerWidget->setSizingField(
    this->mesher_.getVolume()->getSizingField());
  this->m_cleaverWidget->setMeshButtonEnabled(true);
  this->mesher_.cleanup();
  this->sizingFieldSaved_ = false;
}

void MainWindow::handleMessage(std::string str) {
  this->statusBar()->showMessage(QString::fromStdString(str));
}

void MainWindow::handleError(std::string str) {
  QMessageBox::critical(this, "Critical Error", str.c_str());
  this->handleMessage(str);
  this->handleProgress(100);
}

void MainWindow::handleProgress(int value) {
  if (value < 100) {
    this->progressBar_->setVisible(true);
    this->progressBar_->setValue(static_cast<int>(value));
    this->disableAllActions();
  } else {
    this->progressBar_->setValue(100);
    this->progressBar_->setVisible(false);
    this->enablePossibleActions();
  }
  qApp->processEvents();
}

void MainWindow::disableAllActions() {
  this->computeAnglesAct->setEnabled(false);
  this->removeExternalTetsAct->setEnabled(false);
  this->removeLockedTetsAct->setEnabled(false);
  this->m_cleaverWidget->setMeshButtonEnabled(false);
  this->m_sizingFieldWidget->setCreateButtonEnabled(false);
  this->importMeshAct->setEnabled(false);
  this->importVolumeAct->setEnabled(false);
  this->importSizingFieldAct->setEnabled(false);
  this->exportAct->setEnabled(false);
}

void MainWindow::enablePossibleActions() {
  this->computeAnglesAct->setEnabled(this->mesher_.getTetMesh() != nullptr);
  this->removeExternalTetsAct->setEnabled(this->mesher_.getTetMesh() != nullptr);
  this->removeLockedTetsAct->setEnabled(this->mesher_.getTetMesh() != nullptr);
  this->m_cleaverWidget->setMeshButtonEnabled(this->mesher_.getVolume() != nullptr &&
    this->mesher_.getVolume()->getSizingField() != nullptr);
  this->m_sizingFieldWidget->setCreateButtonEnabled(this->mesher_.getVolume() != nullptr);
  this->importMeshAct->setEnabled(true);
  this->importVolumeAct->setEnabled(true);
  this->importSizingFieldAct->setEnabled(this->mesher_.getVolume() != nullptr);
  this->exportAct->setEnabled(this->mesher_.getTetMesh() != nullptr);
}

void MainWindow::handleDoneMeshing() {
  this->window_->setMesh(this->mesher_.getTetMesh());
  this->m_dataManagerWidget->setMesh(this->mesher_.getTetMesh());
  this->m_meshViewOptionsWidget->setMesh(this->mesher_.getTetMesh());
  this->enableMeshedVolumeOptions();
  this->m_meshViewOptionsWidget->updateOptions();
  this->computeAnglesAct->setEnabled(true);
  this->removeExternalTetsAct->setEnabled(true);
  this->removeLockedTetsAct->setEnabled(true);
  this->meshSaved_ = false;
}

void MainWindow::handleNewMesh() {
  this->window_->setMesh(this->mesher_.getBackgroundMesh());
  this->m_meshViewOptionsWidget->setMesh(this->mesher_.getBackgroundMesh());
}

void MainWindow::handleRepaintGL() {
  this->window_->updateMesh();
  this->window_->updateGL();
  this->window_->repaint();
}
//--------------------------------------
// - removeExternalTets()
// This method grabs the current window
// and if it has a mesh, calls the method
// on the mesh to remove external tets.
//--------------------------------------
void MainWindow::removeExternalTets()
{
  cleaver::TetMesh *mesh = this->mesher_.getTetMesh();
  cleaver::Volume  *volume = this->mesher_.getVolume();
  if(mesh && volume)
  {
    cleaver::stripExteriorTets(mesh,volume,true);

    // recompute update adjacency
    mesh->constructFaces();
    mesh->constructBottomUpIncidences();

    this->window_->setMesh(mesh);      // trigger update
    this->meshSaved_ = false;
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
  cleaver::TetMesh *mesh = this->mesher_.getTetMesh();
  if(mesh)
  {
    mesh->removeLockedTets();   // make it so
    this->window_->setMesh(mesh);      // trigger update
    this->meshSaved_ = false;
  }
}

void MainWindow::computeMeshAngles()
{
  cleaver::TetMesh *mesh = this->mesher_.getTetMesh();
  if(mesh) {
    mesh->computeAngles();
    this->handleMessage("Min Angle: " + std::to_string(mesh->min_angle) + " degrees." +
      "Max Angle: " + std::to_string(mesh->max_angle) + " degrees.");
  }
}
//*************Custom file dialog for segmentation check
class MyFileDialog : public QFileDialog
{
  public:
    MyFileDialog(QWidget *, const QString& a,
        const QString& b, const QString& c);
    bool isSegmentation();
    double sigma();
    QSize sizeHint() const;
  private:
    QCheckBox * segmentation_check_;
    QDoubleSpinBox * sigmaValue_;
};

bool MyFileDialog::isSegmentation() {
  return !this->segmentation_check_->isChecked();
}

double MyFileDialog::sigma() {
  return this->sigmaValue_->value();
}

MyFileDialog::MyFileDialog( QWidget *parent, const QString& a,
    const QString& b, const QString& c) :
  QFileDialog( parent, a, b, c),
  segmentation_check_(nullptr)
{
  setOption(QFileDialog::DontUseNativeDialog,true);
  setFileMode(QFileDialog::ExistingFiles);
  QGridLayout* mainLayout = dynamic_cast<QGridLayout*>(layout());

  QHBoxLayout *hbl = new QHBoxLayout();

  // add some widgets
  segmentation_check_ = new QCheckBox("These are individual indicator functions", this);
  segmentation_check_->setChecked(false);
  sigmaValue_ = new QDoubleSpinBox(this);
  sigmaValue_->setValue(1.);
  sigmaValue_->setMinimum(0.);
  sigmaValue_->setMaximum(64.);
  sigmaValue_->setSingleStep(1.);
  hbl->addWidget(segmentation_check_);
  hbl->addItem(new QSpacerItem(30, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
  hbl->addWidget(new QLabel("Blending Function Sigma", this));
  hbl->addWidget(sigmaValue_);

  int numRows = mainLayout->rowCount();

  // add the new layout to the bottom of mainLayout
  // and span all columns
  mainLayout->addLayout( hbl, numRows,0,1,-1);
}

QSize MyFileDialog::sizeHint() const
{
  return QSize(800,600);
}
//*********************END custom file dialog

bool MainWindow::checkSaved() {
  if (!this->sizingFieldSaved_) {
    // save the size of the window to preferences
    QMessageBox msgBox;
    msgBox.setText("You haven't saved your computed Sizing Field.");
    msgBox.setInformativeText("Would you like to save your sizing field to file?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Save) {
      this->exportField(reinterpret_cast<cleaver::FloatField*>(
        this->mesher_.getVolume()->getSizingField()));
      this->sizingFieldSaved_ = true;
    } else if (ret == QMessageBox::Cancel) {
      return false;
    } else if (ret == QMessageBox::Discard) {
      this->sizingFieldSaved_ = true;
    }
  }
  if (!this->meshSaved_) {
    // save the size of the window to preferences
    QMessageBox msgBox;
    msgBox.setText("You haven't saved your new Tet Mesh.");
    msgBox.setInformativeText("Would you like to save your Tet Mesh to file?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Save) {
      this->exportMesh(this->mesher_.getTetMesh());
      this->meshSaved_ = true;
    } else if (ret == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

void MainWindow::importVolume() {
  if (!this->checkSaved()) {
    return;
  }
  QStringList fileNames;
  MyFileDialog dialog(this, tr("Select Indicator Functions"),
      QString::fromStdString(lastPath_), tr("NRRD (*.nrrd)"));
  if (dialog.exec())
    fileNames = dialog.selectedFiles();
  bool segmentation = dialog.isSegmentation();
  double sigma = dialog.sigma();
  if (!fileNames.isEmpty()) {
    std::string file1 = (*fileNames.begin()).toStdString();
    auto pos = file1.find_last_of('/');
    lastPath_ = file1.substr(0,pos);
    bool add_inverse = false;

    std::vector<std::string> inputs;

    for (int i=0; i < fileNames.size(); i++){
      inputs.push_back(fileNames[i].toStdString());
    }

    QProgressDialog status(QString("Loading Indicator Functions..."), QString(), 0, 100, this);
    status.show();
    status.setWindowModality(Qt::WindowModal);
    status.setValue(5);
    QApplication::processEvents();
    std::cout << " Loading input fields:" << std::endl;
    std::vector<cleaver::AbstractScalarField*> fields;
    if (inputs.empty()){
      std::cerr << "No material fields or segmentation files provided. Terminating."
        << std::endl;
      return;
    } else if (segmentation && inputs.size() == 1) {
      fields = NRRDTools::segmentationToIndicatorFunctions(inputs[0], sigma, fileNames.size());
      if(fields.empty()) {
        this->handleError("Input file cannot be read as a label map. It may be raw data.");
        status.setValue(0);
        return;
      }
      status.setValue(90);
      QApplication::processEvents();
    } else {
      if (inputs.size() > 1) {
        this->handleMessage("WARNING: More than one inputs provided for segmentation." +
          std::string(" This will be assumed to be indicator functions."));
      }
      if (inputs.size() == 1) {
        add_inverse = true;
      }

      for (size_t i = 0; i < inputs.size(); i++) {
        std::cout << " - " << inputs[i] << std::endl;
      }
      status.setValue(10);
      QApplication::processEvents();

      fields = NRRDTools::loadNRRDFiles(inputs);

      status.setValue(70);
      QApplication::processEvents();
      if (fields.empty()) {
        std::cerr << "Failed to load image data. Terminating." << std::endl;
        return;
      } else if (add_inverse) {
        fields.push_back(new cleaver::InverseScalarField(fields[0]));
        fields.back()->setName(fields[0]->name() + "-inverse");
      }
      status.setValue(90);
      QApplication::processEvents();
    }
    // Add Fields to Data Manager
    this->m_dataManagerWidget->setIndicators(fields);
    cleaver::Volume *volume = new cleaver::Volume(fields);

    static int v = 0;
    std::string volumeName = std::string("Volume");
    if (v > 0){
      volumeName += std::string(" ") + QString::number(v).toStdString();
    }
    volume->setName(volumeName);
    status.setValue(95);
    QApplication::processEvents();

    this->m_dataManagerWidget->setMesh(nullptr);
    this->m_dataManagerWidget->setSizingField(nullptr);
    this->m_dataManagerWidget->setVolume(volume);
    this->mesher_.setVolume(volume);
    this->window_->setVolume(volume);

    m_cleaverWidget->resetCheckboxes();
    status.setValue(100);
    this->enablePossibleActions();
  }
}

void MainWindow::importSizingField() {
  if (!this->checkSaved()) {
    return;
  }
  cleaver::Volume  *volume = this->mesher_.getVolume();

  QString fileName = QFileDialog::getOpenFileName(this, tr("Select Sizing Field"),
      QString::fromStdString(lastPath_), tr("NRRD (*.nrrd)"));

  if (!fileName.isEmpty())
  {
    std::string file1 = QString((*fileName.begin())).toStdString();
    auto pos = file1.find_last_of('/');
    lastPath_ = file1.substr(0,pos);
    std::vector<cleaver::AbstractScalarField*> sizingField =
      NRRDTools::loadNRRDFiles({ {fileName.toStdString()} });
    volume->setSizingField(sizingField[0]);
    this->m_dataManagerWidget->setSizingField(sizingField[0]);
    this->m_cleaverWidget->setMeshButtonEnabled(true);
    this->mesher_.cleanup();
    this->sizingFieldSaved_ = false;
    std::cout << "Sizing Field Set" << std::endl;
  }
  this->enablePossibleActions();
}

void MainWindow::importMesh() {
  if (!this->checkSaved()) {
    return;
  }
  QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import Tetgen Mesh"),
      QString::fromStdString(lastPath_), tr("Tetgen Mesh (*.node *.ele)"));
  if (fileNames.size() == 1) {
    //try to correct only 1 file added, either .node or .ele
    std::string f = fileNames.at(0).toStdString();
    f = f.substr(0,f.find_last_of("."));
    fileNames.clear();
    fileNames.push_back(QString::fromStdString(f + ".node"));
    fileNames.push_back(QString::fromStdString(f + ".ele"));
  }
  if (fileNames.size() == 2)
  {
    std::string elefilename;
    std::string nodefilename;

    for (int i=0; i < 2; i++)
    {
      std::string fn = fileNames[i].toStdString();
      if (fn.substr(fn.find_last_of(".") + 1) == "ele")
        elefilename = fn;
      else if (fn.substr(fn.find_last_of(".") + 1) == "node")
        nodefilename = fn;
      else
      {
        std::cerr << "Invalid input extension!\n" << std::endl;
        return;
      }
    }

    if (elefilename.length() > 0 && nodefilename.length() > 0)
    {
      cleaver::TetMesh *mesh =
        cleaver::TetMesh::createFromNodeElePair(nodefilename, elefilename,false);
      if (mesh == nullptr){
        std::cerr << "Invalid Mesh" << std::endl;
        return;
      }


      mesh->constructFaces();
      mesh->constructBottomUpIncidences();
      mesh->imported = true;

      this->m_meshViewOptionsWidget->setShowCutsCheckboxEnabled(false);
      this->m_dataManagerWidget->setSizingField(nullptr);
      this->m_dataManagerWidget->setVolume(nullptr);
      this->mesher_.setVolume(nullptr);
      this->window_->setMesh(mesh);
      this->m_dataManagerWidget->setMesh(mesh);
      this->m_meshViewOptionsWidget->setMesh(mesh);
      this->m_meshViewOptionsWidget->updateOptions();
      this->m_dataManagerWidget->setIndicators(std::vector<cleaver::AbstractScalarField*>());
    }
  }
  if (!fileNames.empty()) {
    std::string file1 = (*fileNames.begin()).toStdString();
    auto pos = file1.find_last_of('/');
    lastPath_ = file1.substr(0,pos);
  }
  this->enablePossibleActions();
}

void MainWindow::exportField(cleaver::FloatField *field) {
  if (!field)
    return;
  QString ext;
  QString selectedFilter;
  std::string name = field->name() == "" ? "Untitled" : field->name();
  name += ".nrrd";
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Field As"),
      (lastPath_ + "/" + name).c_str(), tr("NRRD (*.nrrd);"), &selectedFilter);
  QString filter1("NRRD (*.nrrd)");
  if (fileName.isEmpty()) {
    return;
  }

  if (!fileName.endsWith(".nrrd")) {
    fileName += ".nrrd";
  }

  NRRDTools::saveNRRDFile(field, std::string(fileName.toLatin1()));
  if (fileName != "") {
    std::string file1 = fileName.toStdString();
    auto pos = file1.find_last_of('/');
    lastPath_ = file1.substr(0,pos);
  }
}

void MainWindow::exportMesh(cleaver::TetMesh *mesh) {
  // If no mesh selected, get active window mesh
  if (!mesh)
    mesh = this->mesher_.getTetMesh();
  if (!mesh)
    return;
  QString ext;
  QString selectedFilter;
  std::string name = mesh->name == "" ? "Untitled" : mesh->name;
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mesh As"),
      (lastPath_ + "/" + name).c_str(),
      tr("Tetgen (*.node);;SCIRun (*.pts);;Surface PLY (*.ply);;Matlab (*.mat);;VTK Poly (*.vtk);;VTK Unstructured Grid (*.vtk)"), &selectedFilter);
  if (fileName.isEmpty()) {
    return;
  }
  QString filter1("Tetgen (*.node)");
  QString filter2("SCIRun (*.pts)");
  QString filter3("Surface PLY (*.ply)");
  QString filter4("Matlab (*.mat)");
  QString filter5("VTK Unstructured Grid (*.vtk)");
  QString filter6("VTK Poly (*.vtk)");

  std::string f = fileName.toStdString();
  f = f.substr(0,f.rfind("."));

  if (selectedFilter == filter1){
    mesh->writeNodeEle(f, true, true);
  }
  else if (selectedFilter == filter2){
    mesh->writePtsEle(f, true);
  }
  else if (selectedFilter == filter3){
    mesh->writePly(f, true);
  }
  else if (selectedFilter == filter4){
    mesh->writeMatlab(f, true);
  }
  else if (selectedFilter == filter5){
    mesh->writeVtkUnstructuredGrid(f, true);
  }
  else if (selectedFilter == filter6){
    mesh->writeVtkPolyData(f, true);
  }
  if (fileName != "") {
    std::string file1 = fileName.toStdString();
    auto pos = file1.find_last_of('/');
    lastPath_ = file1.substr(0,pos);
  }
}

void MainWindow::about() {
  QMessageBox::about(this, tr("About Cleaver 2"),
      tr("<b>Cleaver ") +
      tr(cleaver::VersionNumber.c_str()) +
      tr(" built ") +
      tr(cleaver::VersionDate.c_str()) +
      tr("</b><BR>"
        "<a href=\"http://www.sci.utah.edu/\">Scientific Computing & Imaging Institute</a><BR>"
        "<a href=\"http://www.cs.utah.edu/\">University of Utah, School of Computing</a><BR>"
        "<P>This program is provided AS IS with NO "
        "WARRANTY OF ANY KIND, INCLUDING THE WARRANTY"
        "OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
        "PARTICULAR PURPOSE."));
}

MeshWindow * MainWindow::createWindow(const QString &title) {
    QGLFormat glFormat;
    glFormat.setVersion(3, 3);
    glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
    glFormat.setSampleBuffers(true);
    MeshWindow *window = new MeshWindow(glFormat, this);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->showMaximized();
    return window;
}

void MainWindow::enableMeshedVolumeOptions() {

  this->m_meshViewOptionsWidget->setShowCutsCheckboxEnabled(true);
  this->exportAct->setEnabled(true);
}

void MainWindow::handleExportField(void* p) {
  this->exportField(reinterpret_cast<cleaver::FloatField*>(p));
}

void MainWindow::handleExportMesh(void* p) {
  this->exportMesh(reinterpret_cast<cleaver::TetMesh*>(p));
}

void MainWindow::handleDisableMeshing() {
  this->m_cleaverWidget->setMeshButtonEnabled(false);
  this->computeAnglesAct->setEnabled(false);
  this->removeExternalTetsAct->setEnabled(false);
  this->removeLockedTetsAct->setEnabled(false);
}

void MainWindow::handleDisableSizingField() {
  this->m_sizingFieldWidget->setCreateButtonEnabled(false);
  this->computeAnglesAct->setEnabled(false);
  this->removeExternalTetsAct->setEnabled(false);
  this->removeLockedTetsAct->setEnabled(false);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (!this->checkSaved()) {
    event->ignore();
    return;
  }
}
