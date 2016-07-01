#include "MeshWindow.h"
#include "TrackballCamera.h"
#include <Cleaver/BoundingBox.h>
#include <Cleaver/vec3.h>
#include "../../lib/cleaver/Plane.h"
#include <QMouseEvent>
#include <QFileDialog>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "Shaders/Shaders.h"
#include <QMatrix4x4>
#include <array>
#include <boost/math/special_functions/fpclassify.hpp>


#ifndef M_PI
#define M_PI 3.14159265359
#endif

std::vector<cleaver::vec3> viol_point_list;
std::vector<cleaver::Plane> viol_plane_list;

enum
{
  VERTEX_OBJECT = 0,
  COLOR_OBJECT = 1,
  NORMAL_OBJECT = 2
};

static bool ctrl_down = false;
float color_for_label[][4] = { { 0.0f, 174 / 255.0f, 239 / 255.0f, 1.0f },   // label 0
  { 0.0f, 166 / 255.0f, 81 / 255.0f, 1.0f },   // label 1
  { 1.0f, 242 / 255.0f, 0.0f, 1.0f },         // label 2
  { 237 / 255.0f, 28 / 255.0f, 36 / 255.0f, 1.0f },   // label 3
  { 1.0f, 0.0f, 1.0f, 1.0f },          // label 4
  { 0.5f, 1.0f, 0.5f, 1.0f },          // label 5
  { 1.0f, 0.5f, 0.5f, 1.0f } };         // label 6

const char* starModeString[4] = { "No-Star Mode",
  "Vertex-Star Mode", "Edge-Star Mode", "Face-Star Mode" };

extern std::vector<cleaver::vec3> badEdges;

MeshWindow::MeshWindow(const QGLFormat& format, QObject *parent) :
  QGLWidget(format, qobject_cast<QWidget *>(parent)),
  faceProg_(this), edgeProg_(this), axisProg_(this),
  bboxVBO_(QOpenGLBuffer::VertexBuffer), bboxVAO_(this)
{
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setMouseTracking(true);
  this->setFocusPolicy(Qt::ClickFocus);
  //this->setFocusPolicy(Qt::StrongFocus);
  this->faceVAO_ = this->edgeVAO_ = this->cutVAO_ = this->violVAO_ = nullptr;
  this->faceVBO_ = this->edgeVBO_ = this->cutVBO_ = this->violVBO_ = nullptr;
  this->volume_ = nullptr;
  this->mesh_ = nullptr;
  this->mesher_ = nullptr;
  init = false;

  initializeOptions();
}

MeshWindow::~MeshWindow()
{
  if (this->faceVBO_) {
    this->faceVBO_->destroy();
  }
  if (this->faceVAO_) {
    this->faceVBO_->destroy();
  }
  if (this->edgeVBO_) {
    this->edgeVBO_->destroy();
  }
  if (this->edgeVAO_) {
    this->edgeVAO_->destroy();
  }
  emit closed(this);
}

QSize MeshWindow::sizeHint() const
{
  return QSize(800, 600);
}

void MeshWindow::setDefaultOptions()
{
  initializeOptions();
}

void MeshWindow::resetView()
{
  auto &bb = this->m_dataBounds;
  //bbox lines
  this->bboxData_.clear();
  //origin to X
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y);
  this->bboxData_.push_back(bb.origin.z);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y);
  this->bboxData_.push_back(bb.origin.z);
  //origin to Y
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y);
  this->bboxData_.push_back(bb.origin.z);
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z);
  //origin to Z
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y);
  this->bboxData_.push_back(bb.origin.z);
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  //back to -X
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + 0);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  //back to -Y
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  //back to -Z
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + 0);
  //O + Y -> X
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + 0);
  //O + Y -> Z
  this->bboxData_.push_back(bb.origin.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z);
  this->bboxData_.push_back(bb.origin.x + 0);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  //back - Y -> X
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + 0);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  //back - Y -> Z
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + 0);
  //O + X -> Y
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + 0);
  this->bboxData_.push_back(bb.origin.x + bb.size.x);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + 0);
  //O + Z -> Y
  this->bboxData_.push_back(bb.origin.x + 0);
  this->bboxData_.push_back(bb.origin.y + 0);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);
  this->bboxData_.push_back(bb.origin.x + 0);
  this->bboxData_.push_back(bb.origin.y + bb.size.y);
  this->bboxData_.push_back(bb.origin.z + bb.size.z);

  if (this->init) {
    //the 6 bb lines
    this->edgeProg_.bind();
    this->bboxVBO_.bind();
    this->bboxVBO_.write(0, &this->bboxData_[0], static_cast<int>(
          sizeof(GL_FLOAT) * this->bboxData_.size()));
    this->bboxVBO_.release();
    this->edgeProg_.release();
  }
  this->cameraMatrix_ = this->rotateMatrix_ = QMatrix4x4();
  this->cameraMatrix_.translate(-bb.center().x, -bb.center().y, -bb.center().z);
  float scale = 0.5 / std::max(bb.size.x, std::max(bb.size.y, bb.size.z));
  QMatrix4x4 scalemat;
  scalemat.scale(scale);
  this->cameraMatrix_ = scalemat * this->cameraMatrix_;
  QMatrix4x4 rotmat;
  rotmat.rotate(30, 1, 0, 0);
  rotmat.rotate(-10, 0, 1, 0);
  this->cameraMatrix_ = rotmat * this->cameraMatrix_;
  this->rotateMatrix_ = rotmat;

  this->updateGL();
}

void MeshWindow::saveView()
{
  std::cout << "Save View to file" << std::endl;

  //m_camera->writeToFile("filename");
  const float *viewmatrix = this->cameraMatrix_.constData();
  const float *rotmatrix = this->rotateMatrix_.constData();
  std::ofstream file("camera.dat", std::ios::out | std::ios::binary);
  file.write((char*)viewmatrix, 16 * sizeof(float));
  file.write((char*)rotmatrix, 16 * sizeof(float));
  file.close();
}

void MeshWindow::loadView()
{
  std::cout << "Loading View from file" << std::endl;

  float matrix[16];
  std::ifstream file("camera.dat", std::ios::in | std::ios::binary);

  file.read((char*)matrix, 16 * sizeof(float));
  this->cameraMatrix_ = QMatrix4x4(matrix);
  file.read((char*)matrix, 16 * sizeof(float));
  this->rotateMatrix_ = QMatrix4x4(matrix);
  m_bLoadedView = true;
  this->updateGL();
}

void MeshWindow::initializeOptions()
{
  m_bShowAxis = true;
  m_bShowBBox = true;
  m_bShowFaces = true;
  m_bShowEdges = true;
  m_bShowCuts = false;
  m_bClipping = false;
  m_bShowClippingPlane = false;
  m_bSyncedClipping = false;
  m_bSurfacesOnly = false;
  m_bShowViolationPolytopes = false;
  m_bColorByQuality = false;
  m_bOpenGLError = false;
  m_bImportedBeta = false;
  m_bLoadedView = false;

  // for adjacency visualization and debugging
  m_starmode = NoStar;
  m_currentVertex = 0;
  m_currentEdge = 0;
  m_currentFace = 0;
  m_shrinkscale = 0.0;
  //m_vertexData = nullptr;

  this->resize(this->maximumSize());
}

void MeshWindow::initializeShaders()
{
  //AXIS SETUP
  std::string vs;
  vs = "#version 330\n";
  vs = vs + "layout (location = 0) in vec3 aPos;\n";
  vs = vs + "layout (location = 1) in vec3 aColor;\n";
  vs = vs + "uniform mat4 uRotation;\n";
  vs = vs + "uniform float uAspect;\n";
  vs = vs + "uniform float uWidth;\n";
  vs = vs + "out vec4 oColor;\n";
  vs = vs + "void main() {\n";
  vs = vs + "  vec4 ans = uRotation * vec4(aPos.x, aPos.y, aPos.z ,1.0);\n";
  vs = vs + "  gl_Position = vec4((.4 * ans.x * 300./uWidth - 1. + 200./uWidth), ";
  vs = vs + "                .4 * ans.y * 300. * uAspect / uWidth + 1. - 200. ";
  vs = vs + "                 * uAspect / uWidth,";
  vs = vs + "                 0,1.);\n";
  vs = vs + "  oColor = vec4(aColor,1.);\n";
  vs = vs + "}\n";
  QOpenGLShader vshader0(QOpenGLShader::Vertex);
  if (!vshader0.compileSourceCode(vs.c_str())) {
    qWarning() << vshader0.log();
  }
  if (!this->axisProg_.addShader(&vshader0)) {
    qWarning() << this->axisProg_.log();
  }
  std::string fs;
  fs = "#version 330\n";
  fs = fs + "in vec4 oColor;\n";
  fs = fs + "out vec4 fragColor;\n";
  fs = fs + "void main() {\n";
  fs = fs + "  fragColor = oColor;\n";
  fs = fs + "}\n";
  QOpenGLShader fshader0(QOpenGLShader::Fragment);
  if (!fshader0.compileSourceCode(fs.c_str())) {
    qWarning() << fshader0.log();
  }
  if (!this->axisProg_.addShader(&fshader0)) {
    qWarning() << this->axisProg_.log();
  }
  if (!this->axisProg_.link()) {
    qWarning() << this->axisProg_.log();
  }
  //create VBO for the axis

  std::array<float, 132> axis_data;
  axis_data = { {
    //aPos...           aColor...
    // X
    0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
      1.f, 0.f, 0.f, 1.f, 0.f, 0.f,
      1.2f, 0.2f, 0.f, 1.f, 0.f, 0.f,
      1.4f, -0.2f, 0.f, 1.f, 0.f, 0.f,
      1.4f, 0.2f, 0.f, 1.f, 0.f, 0.f,
      1.2f, -0.2f, 0.f, 1.f, 0.f, 0.f,
      // Y
      0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
      0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
      0.0f, 1.2f, 0.f, 0.f, 1.f, 0.f,
      0.0f, 1.4f, 0.f, 0.f, 1.f, 0.f,
      0.0f, 1.4f, 0.f, 0.f, 1.f, 0.f,
      0.1f, 1.6f, 0.f, 0.f, 1.f, 0.f,
      0.0f, 1.4f, 0.f, 0.f, 1.f, 0.f,
      -0.1f, 1.6f, 0.f, 0.f, 1.f, 0.f,
      // Z
      0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
      0.f, 0.f, 1.f, 0.f, 0.f, 1.f,
      0.0f, 0.2f, 1.4f, 0.f, 0.f, 1.f,
      0.0f, 0.2f, 1.2f, 0.f, 0.f, 1.f,
      0.0f, 0.2f, 1.2f, 0.f, 0.f, 1.f,
      0.0f, -0.2f, 1.4f, 0.f, 0.f, 1.f,
      0.0f, -0.2f, 1.4f, 0.f, 0.f, 1.f,
      0.0f, -0.2f, 1.2f, 0.f, 0.f, 1.f
  } };
  QOpenGLBuffer axis_vbo(QOpenGLBuffer::VertexBuffer);
  axis_vbo.create();
  axis_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!axis_vbo.bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  axis_vbo.allocate(&axis_data, sizeof(GL_FLOAT) * 132);

  this->axisVAO_.create();
  this->axisVAO_.bind();
  this->axisProg_.enableAttributeArray(0);
  this->axisProg_.enableAttributeArray(1);
  this->axisProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 6);
  this->axisProg_.setAttributeBuffer(1, GL_FLOAT, sizeof(GL_FLOAT) * 3, 3, sizeof(GL_FLOAT) * 6);

  this->axisVAO_.release();
  this->axisVAO_.release();
  this->axisProg_.release();
  //setup axis transform matrix and send down.
  auto aspect = static_cast<float>(
      this->width()) / static_cast<float>(this->height());
  this->axisProg_.bind();
  this->axisProg_.setUniformValue("uAspect", aspect);
  this->axisProg_.setUniformValue("uWidth", static_cast<float>(
        this->width()));
  this->axisProg_.release();

  // face shaders
  vs = "#version 330\n";
  vs = vs + "layout (location = 0) in vec3 aPos;\n";
  vs = vs + "layout (location = 1) in vec3 aNormal;\n";
  vs = vs + "layout (location = 2) in vec3 aColor;\n";
  vs = vs + "uniform mat4 uProjection;\n";
  vs = vs + "uniform mat4 uTransform;\n";
  vs = vs + "out vec3 oPos;\n";
  vs = vs + "out vec3 oNormal;\n";
  vs = vs + "out vec3 oColor;\n";
  vs = vs + "void main() {\n";
  vs = vs + "  vec4 ans = uTransform * vec4(aPos.x, aPos.y,aPos.z,1.0);\n";
  vs = vs + "  gl_Position = uProjection * vec4(ans.x,ans.y,ans.z - 2, 1.);\n ";
  vs = vs + "  oPos = ans.xyz;\n ";
  vs = vs + "  oNormal = normalize((uTransform * vec4(aNormal,0.)).xyz);\n";
  vs = vs + "  oColor = aColor;\n";
  vs = vs + "}\n";
  //fragment
  fs = "#version 330\n";
  fs = fs + "in vec3 oPos;\n";
  fs = fs + "in vec3 oNormal;\n";
  fs = fs + "in vec3 oColor;\n";
  fs = fs + "out vec4 fragColor;\n";
  //the main
  fs = fs + "void main() {\n";
  fs = fs + "  vec3 light_pos = vec3(0.,0.,1.);\n";
  fs = fs + "  vec3 N = oNormal;\n";
  fs = fs + "  vec3 L = normalize(light_pos);\n";
  fs = fs + "  float amb = 0.2;\n";
  fs = fs + "  float ndotl = dot(N,L);\n";
  fs = fs + "  if (ndotl < 0.) ndotl = ndotl * -1.;\n";
  fs = fs + "  float dif = 0.6 * clamp(ndotl,0.,1.);\n";
  fs = fs + "  vec3 half_vector = normalize(oPos+L);\n";
  fs = fs + "  float ndoth = abs(dot(N,L));\n";
  fs = fs + "  float spc = 0.3 * pow(ndoth,50.);\n";
  fs = fs + "  fragColor = vec4(spc * vec3(1.,1.,1.) + \n";
  fs = fs + "       clamp(oColor * (amb + dif),0.,1.),1.);\n";
  fs = fs + "}\n";
  this->faceProg_.bind();
  // Create Shader And Program Objects
  QOpenGLShader vshader(QOpenGLShader::Vertex);
  if (!vshader.compileSourceCode(vs.c_str())) {
    qWarning() << vshader.log();
  }
  if (!this->faceProg_.addShader(&vshader)) {
    qWarning() << this->faceProg_.log();
  }
  QOpenGLShader fshader(QOpenGLShader::Fragment);
  if (!fshader.compileSourceCode(fs.c_str())) {
    qWarning() << fshader.log();
  }
  if (!this->faceProg_.addShader(&fshader)) {
    qWarning() << this->faceProg_.log();
  }
  if (!this->faceProg_.link()) {
    qWarning() << this->faceProg_.log();
  }
  this->faceProg_.release();

  //edge shaders
  vs = "#version 330\n";
  vs = vs + "layout (location = 0) in vec3 aPos;\n";
  vs = vs + "uniform mat4 uProjection;\n";
  vs = vs + "uniform mat4 uTransform;\n";
  vs = vs + "void main() {\n";
  vs = vs + "  vec4 ans = uTransform * vec4(aPos.x, aPos.y,aPos.z,1.0);\n";
  vs = vs + "  gl_Position = uProjection * vec4(ans.x,ans.y,ans.z - 2. + .001, 1.);\n ";
  vs = vs + "}\n";
  fs = "#version 330\n";
  fs = fs + "uniform vec4 uColor;\n";
  fs = fs + "out vec4 fragColor;\n";
  fs = fs + "void main() {\n";
  fs = fs + "  fragColor = uColor;\n";
  fs = fs + "}\n";
  this->edgeProg_.bind();
  // Create Shader And Program Objects
  QOpenGLShader vshader2(QOpenGLShader::Vertex);
  if (!vshader2.compileSourceCode(vs.c_str())) {
    qWarning() << vshader2.log();
  }
  if (!this->edgeProg_.addShader(&vshader2)) {
    qWarning() << this->edgeProg_.log();
  }
  QOpenGLShader fshader2(QOpenGLShader::Fragment);
  if (!fshader2.compileSourceCode(fs.c_str())) {
    qWarning() << fshader2.log();
  }
  if (!this->edgeProg_.addShader(&fshader2)) {
    qWarning() << this->edgeProg_.log();
  }
  if (!this->edgeProg_.link()) {
    qWarning() << this->edgeProg_.log();
  }
  this->edgeProg_.release();
}

void MeshWindow::initializeGL()
{
  QGLFormat glFormat = QGLWidget::format();
  if (!glFormat.sampleBuffers()) {
    qWarning() << "Could not enable sample buffers";
  }
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);

  setup_vbos();

  initializeShaders();

  init = true;
}

void MeshWindow::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
  GLdouble aspectRatio = (GLdouble)w / (float)h;
  GLdouble zNear = 0.1;
  GLdouble zFar = 2000.0;
  GLdouble yFovInDegrees = 45;
  this->perspMat_ = QMatrix4x4();
  this->perspMat_.perspective(yFovInDegrees, aspectRatio, zNear, zFar);
  this->axisProg_.bind();
  this->axisProg_.setUniformValue("uAspect", (float)aspectRatio);
  this->axisProg_.setUniformValue("uWidth", static_cast<float>(
        this->width()));
  this->axisProg_.release();
}

void printMatrix(GLdouble *matrix)
{
  std::cout << matrix[0] << " " << matrix[1] << " " << matrix[2] << " " << matrix[3] << std::endl;
  std::cout << matrix[4] << " " << matrix[5] << " " << matrix[6] << " " << matrix[7] << std::endl;
  std::cout << matrix[8] << " " << matrix[9] << " " << matrix[10] << " " << matrix[11] << std::endl;
  std::cout << matrix[12] << " " << matrix[13] << " " << matrix[14] << " " << matrix[15] << std::endl;
}

void printMatrix(float *matrix)
{
  std::cout << matrix[0] << " " << matrix[1] << " " << matrix[2] << " " << matrix[3] << std::endl;
  std::cout << matrix[4] << " " << matrix[5] << " " << matrix[6] << " " << matrix[7] << std::endl;
  std::cout << matrix[8] << " " << matrix[9] << " " << matrix[10] << " " << matrix[11] << std::endl;
  std::cout << matrix[12] << " " << matrix[13] << " " << matrix[14] << " " << matrix[15] << std::endl;
}

void MeshWindow::paintGL()
{
  if (!this->bboxVBO_.isCreated()) {
    this->edgeProg_.bind();
    this->bboxVBO_.create();
    this->bboxVBO_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->bboxVBO_.bind();
    std::vector<float> tmp(72, 0);
    this->bboxVBO_.allocate(&tmp[0], static_cast<int>(
          sizeof(GL_FLOAT) * tmp.size()));
    this->bboxVAO_.create();
    this->bboxVAO_.bind();
    this->edgeProg_.enableAttributeArray(0);
    this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);
    this->bboxVAO_.release();
    this->bboxVBO_.release();
    this->edgeProg_.release();
  }
  if (this->perspMat_ == QMatrix4x4()) {
    this->resizeGL(this->width(), this->height());
  }
  auto matTest = this->rotateMatrix_.data();
  for(size_t i = 0; i < 16; i++) {
    if(boost::math::isnan(matTest[i])) {
      std::cout << "Recovering from a NaN matrix error..." << std::endl;
      this->resetView();
      break;
    }
  }
  qglClearColor(Qt::gray);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // DRAW THE COORDINATE AXIS
  if (m_bShowAxis) {
    glLineWidth(1.0);
    this->axisProg_.bind();
    this->axisProg_.setUniformValueArray("uRotation",
        &this->rotateMatrix_, 1);
    this->axisVAO_.bind();
    glDrawArrays(GL_LINES, 0, 132);
    this->axisVAO_.release();
    this->axisProg_.release();
  }
  //now draw the bbox lines
  if (this->m_bShowBBox && !this->bboxData_.empty()) {
    glLineWidth(2.);
    this->edgeProg_.bind();
    this->edgeProg_.setUniformValue("uColor", 0, 0, 0, 1);
    this->edgeProg_.setUniformValueArray("uProjection", &this->perspMat_, 1);
    this->edgeProg_.setUniformValueArray("uTransform", &this->cameraMatrix_, 1);
    this->bboxVAO_.bind();
    glDrawArrays(GL_LINES, 0,
        static_cast<GLsizei>(this->bboxData_.size() / 3));
    this->bboxVAO_.release();
    this->edgeProg_.release();
  }
  if (m_bShowEdges && this->mesh_) {
    drawEdges();
  }
  if (m_bShowFaces && this->mesh_) {
    drawFaces();
  }
  if (m_bShowCuts && this->mesh_ && this->volume_) {
    drawCuts();
  }

  //bad edges
  glLineWidth(3.0f);
  glColor3f(1.0f, 0.0f, 0.0f);
  glBegin(GL_LINES);
  for (size_t i = 0; i < badEdges.size() / 2; i += 2)
  {
    glVertex3f(badEdges[i].x, badEdges[i].y, badEdges[i].z);
    glVertex3f(badEdges[i + 1].x, badEdges[i + 1].y, badEdges[i + 1].z);
  }
  glEnd();
  switch (m_starmode)
  {
  case NoStar:
    break;
  case VertexStar:
    drawVertexStar(m_currentVertex);
    break;
  case EdgeStar:
    drawEdgeStar(m_currentEdge);
    break;
  case FaceStar:
    drawFaceStar(m_currentFace);
    break;
  default: break;
  }

  if (m_bShowViolationPolytopes && this->mesh_) {
    drawViolationPolytopesForVertices();
  }
  if (m_bClipping && m_bShowClippingPlane && this->mesh_) {
    drawClippingPlane();
  }
}

void MeshWindow::drawVertexStar(int v)
{
  // draw vertex
  cleaver::Vertex *vertex = this->mesh_->verts[v];
  glPointSize(4.0f);
  glBegin(GL_POINTS);
  glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
  glVertex3f(vertex->pos().x, vertex->pos().y, vertex->pos().z);
  glEnd();


  //std::vector<Cleaver::Tet*> tetlist = m_mesh->tetsAroundVertex(vertex);

  glDisable(GL_LIGHTING);


  //--- Draw Faces Around Vertex ---
  std::vector<cleaver::HalfFace*> facelist = this->mesh_->facesAroundVertex(vertex);

  glColor3f(0.7f, 0.7f, 0.7f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glEnable(GL_POLYGON_OFFSET_FILL);

  glBegin(GL_TRIANGLES);
  for (unsigned int f = 0; f < facelist.size(); f++)
  {

    for (int v = 0; v < 3; v++){
      cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
      glColor3f(v / 2.0f, 1.0f - (v / 2.0f), 1.0f);
      glVertex3f(p.x, p.y, p.z);
    }
  }
  glEnd();
  glDisable(GL_POLYGON_OFFSET_FILL);


  glColor3f(0.0f, 0.0f, 0.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glBegin(GL_TRIANGLES);
  for (unsigned int f = 0; f < facelist.size(); f++)
  {

    for (int v = 0; v < 3; v++){
      cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
      glVertex3f(p.x, p.y, p.z);
    }
  }
  glEnd();
}

void MeshWindow::drawEdgeStar(int edge)
{
  // get pointer to the current edge
  int idx = 0;
  std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = this->mesh_->halfEdges.begin();
  while (idx != edge){
    iter++; idx++;
  }
  cleaver::HalfEdge *e = (*iter).second;

  cleaver::vec3 p1 = e->vertex->pos();
  cleaver::vec3 p2 = e->mate->vertex->pos();

  // draw edge
  glDisable(GL_LIGHTING);
  glColor3f(0.0f, 0.5f, 0.0f);
  glBegin(GL_LINES);
  glVertex3f(p1.x, p1.y, p1.z);
  glVertex3f(p2.x, p2.y, p2.z);
  glEnd();

  // draw faces incident
  std::vector<cleaver::HalfFace*> facelist = this->mesh_->facesAroundEdge(e);

  glColor3f(0.7f, 0.7f, 0.7f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glEnable(GL_POLYGON_OFFSET_FILL);

  glBegin(GL_TRIANGLES);
  for (unsigned int f = 0; f < facelist.size(); f++)
  {

    for (int v = 0; v < 3; v++){
      cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
      glColor3f(v / 2.0f, 1.0f - (v / 2.0f), 1.0f);
      glVertex3f(p.x, p.y, p.z);
    }
  }
  glEnd();
  glDisable(GL_POLYGON_OFFSET_FILL);


  glColor3f(0.0f, 0.0f, 0.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glBegin(GL_TRIANGLES);
  for (unsigned int f = 0; f < facelist.size(); f++)
  {

    for (int v = 0; v < 3; v++){
      cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
      glVertex3f(p.x, p.y, p.z);
    }
  }
  glEnd();
  glDisable(GL_POLYGON_OFFSET_FILL);

  //std::cout << facelist.size() << " incident faces" << std::endl;

  // draw tets incident
  std::vector<cleaver::Tet*> tetlist = this->mesh_->tetsAroundEdge(e);
  glBegin(GL_TRIANGLES);
  for (unsigned int t = 0; t < tetlist.size(); t++){
    for (int f = 0; f < 4; f++){
      cleaver::vec3 p1 = tetlist[t]->verts[(f + 0) % 4]->pos();
      cleaver::vec3 p2 = tetlist[t]->verts[(f + 1) % 4]->pos();
      cleaver::vec3 p3 = tetlist[t]->verts[(f + 2) % 4]->pos();

      glVertex3f(p1.x, p1.y, p1.z);
      glVertex3f(p2.x, p2.y, p2.z);
      glVertex3f(p3.x, p3.y, p3.z);
    }
  }
  glEnd();
}

void MeshWindow::drawFaceStar(int face)
{
  // get pointer to face
  cleaver::HalfFace *half_face = &this->mesh_->halfFaces[face];

  cleaver::vec3 p1 = half_face->halfEdges[0]->vertex->pos();
  cleaver::vec3 p2 = half_face->halfEdges[1]->vertex->pos();
  cleaver::vec3 p3 = half_face->halfEdges[2]->vertex->pos();

  // draw face
  glDisable(GL_LIGHTING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPolygonOffset(1.0f, 1.0f);
  glEnable(GL_POLYGON_OFFSET_FILL);

  glColor3f(0.6f, 0.6f, 1.0f);
  glBegin(GL_TRIANGLES);
  glVertex3f(p1.x, p1.y, p1.z);
  glVertex3f(p2.x, p2.y, p2.z);
  glVertex3f(p3.x, p3.y, p3.z);
  glEnd();

  glDisable(GL_POLYGON_OFFSET_FILL);


  // draw face outline
  glColor3f(0.0f, 0.0f, 0.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glBegin(GL_TRIANGLES);
  glVertex3f(p1.x, p1.y, p1.z);
  glVertex3f(p2.x, p2.y, p2.z);
  glVertex3f(p3.x, p3.y, p3.z);
  glEnd();

  // get 2 incident tets
  std::vector<cleaver::Tet*> tetlist = this->mesh_->tetsAroundFace(half_face);
  //std::cout << tetlist.size() << " incident tets" << std::endl;

  // draw their lines, don't fill them
  // color them different colors?
  glBegin(GL_LINES);
  for (unsigned int t = 0; t < tetlist.size(); t++)
  {
    for (int i = 0; i < 4; i++){
      for (int j = i + 1; j < 4; j++){
        cleaver::vec3 p1 = tetlist[t]->verts[i]->pos();
        cleaver::vec3 p2 = tetlist[t]->verts[j]->pos();
        glVertex3f(p1.x, p1.y, p1.z);
        glVertex3f(p2.x, p2.y, p2.z);
      }
    }
  }
  glEnd();
}

void MeshWindow::drawViolationPolytopesForVertices()
{
  if (!this->mesher_ || !this->mesher_->alphasComputed()) {
    return;
  }
  for (size_t v = 0; v < this->mesh_->verts.size(); v++)
  {
    glColor3f(1.0f, 0.5f, 0.5f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);
    drawViolationPolytopeForVertex(static_cast<int>(v));
    glDisable(GL_POLYGON_OFFSET_FILL);

    glColor3f(0.0f, 0.0f, 0.0f);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    drawViolationPolytopeForVertex(static_cast<int>(v));


    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
  }
}

void MeshWindow::drawViolationPolytopeForVertex(int v)
{
  // get vertex
  cleaver::Vertex *vertex = this->mesh_->verts[v];

  // get adjacency data
  //std::vector<Cleaver::HalfEdge*> adjEdges = m_mesh->edgesAroundVertex(vertex);
  std::vector<cleaver::Tet*>      adjTets = this->mesh_->tetsAroundVertex(vertex);

  glDisable(GL_LIGHTING);
  glBegin(GL_TRIANGLES);
  // phase 1 - simply connect edge violations
  for (size_t t = 0; t < adjTets.size(); t++)
  {
    // each tet t has 3 edges incident to vertex v
    int count = 0;
    std::vector<cleaver::HalfEdge*> edges = this->mesh_->edgesAroundTet(adjTets[t]);
    for (int e = 0; e < 6; e++){
      cleaver::HalfEdge *edge = edges[e];
      if (edge->incidentToVertex(vertex))
      {
        count++;
        float t = edge->alphaForVertex(vertex);
        if (edge->vertex == vertex){
          cleaver::vec3 v2 = edge->vertex->pos();
          cleaver::vec3 v1 = edge->mate->vertex->pos();
          cleaver::vec3 pos = (t)*v1 + (1 - t)*v2;
          glVertex3f(pos.x, pos.y, pos.z);
        } else{
          cleaver::vec3 v1 = edge->vertex->pos();
          cleaver::vec3 v2 = edge->mate->vertex->pos();
          cleaver::vec3 pos = (t)*v1 + (1 - t)*v2;
          glVertex3f(pos.x, pos.y, pos.z);
        }
      }
    }
    if (count != 3){
      std::cout << "PROBLEM!" << std::endl;
      exit(9);
    }
  }
  glEnd();
  glEnable(GL_LIGHTING);
}

void MeshWindow::drawFaces()
{
  if (!this->mesh_ || this->faceData_.empty()) return;
  // shader pogram
  this->faceProg_.bind();
  this->faceProg_.setUniformValueArray("uTransform", &this->cameraMatrix_, 1);
  this->faceProg_.setUniformValueArray("uProjection", &this->perspMat_, 1);
  this->faceVAO_->bind();
  glDrawArrays(GL_TRIANGLES, 0,
      static_cast<GLsizei>(this->faceData_.size() / 9));
  this->faceVAO_->release();
  this->faceProg_.release();
}

void MeshWindow::drawEdges()
{
  if (!this->mesh_ || this->edgeData_.empty()) return;
  glLineWidth(1.5);
  this->edgeProg_.bind();
  this->edgeProg_.setUniformValue("uColor", 0, 0, 0, 1);
  this->edgeProg_.setUniformValueArray("uTransform", &this->cameraMatrix_, 1);
  this->edgeProg_.setUniformValueArray("uProjection", &this->perspMat_, 1);
  this->edgeVAO_->bind();
  glDrawArrays(GL_LINES, 0,
      static_cast<GLsizei>(this->edgeData_.size() / 3));
  this->edgeVAO_->release();
  this->edgeProg_.release();
}

void MeshWindow::drawCuts()
{
  if (!this->mesh_ || this->cutData_.empty()) return;
  //violating cuts
  glPointSize(4.0);
  this->edgeProg_.bind();
  this->edgeProg_.setUniformValue("uColor", 1, 0, 0, 1);
  this->cutVAO_->bind();
  glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(this->cutData_.size() / 3));
  this->cutVAO_->release();
  this->edgeProg_.release();
  if (this->violData_.empty()) return;
  //violating vectors
  this->edgeProg_.bind();
  this->violVAO_->bind();
  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(this->violData_.size() / 3));
  this->violVAO_->release();
  this->edgeProg_.release();

}

void MeshWindow::drawClippingPlane()
{
  cleaver::vec3 u, v;
  cleaver::vec3 n = cleaver::vec3(m_4fvClippingPlane[0],
      m_4fvClippingPlane[1], m_4fvClippingPlane[2]);

  u = n.cross(cleaver::vec3::unitX);
  double dotval = n.dot(cleaver::vec3::unitX);

  if (fabs(n.dot(cleaver::vec3::unitY)) <= dotval){
    u = n.cross(cleaver::vec3::unitY);
    dotval = fabs(n.dot(cleaver::vec3::unitY));
  }
  if (fabs(n.dot(cleaver::vec3::unitZ)) <= dotval)
    u = n.cross(cleaver::vec3::unitZ);

  v = n.cross(u);

  // point P0 center of plane should be center of bounding box
  int w = 0.6f*m_dataBounds.size.x;
  int h = 0.6f*m_dataBounds.size.y;

  cleaver::vec3 shift = cleaver::vec3(m_dataBounds.size.x*n.x,
      m_dataBounds.size.y*n.y,
      m_dataBounds.size.z*n.z);

  cleaver::vec3 p0 = m_dataBounds.center() - 0.5f*shift + m_4fvClippingPlane[3] * n;
  cleaver::vec3 p1 = p0 + (1.0)*w*u + (1.0)*h*v;
  cleaver::vec3 p2 = p0 + (-1.0)*w*u + (1.0)*h*v;
  cleaver::vec3 p3 = p0 + (-1.0)*w*u + (-1.0)*h*v;
  cleaver::vec3 p4 = p0 + (1.0)*w*u + (-1.0)*h*v;

  std::vector<float> pts;
  pts.push_back(p1.x);
  pts.push_back(p1.y);
  pts.push_back(p1.z);
  pts.push_back(p2.x);
  pts.push_back(p2.y);
  pts.push_back(p2.z);
  pts.push_back(p3.x);
  pts.push_back(p3.y);
  pts.push_back(p3.z);
  pts.push_back(p3.x);
  pts.push_back(p3.y);
  pts.push_back(p3.z);
  pts.push_back(p4.x);
  pts.push_back(p4.y);
  pts.push_back(p4.z);
  pts.push_back(p1.x);
  pts.push_back(p1.y);
  pts.push_back(p1.z);
  this->edgeProg_.bind();
  QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
  vbo.create();
  vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
  vbo.bind();
  vbo.allocate(pts.data(), static_cast<int>(sizeof(GL_FLOAT) * pts.size()));
  QOpenGLVertexArrayObject vao(this);
  vao.create();
  vao.bind();
  this->edgeProg_.enableAttributeArray(0);
  this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

  vao.release();
  vbo.release();
  this->edgeProg_.release();

  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  this->edgeProg_.bind();
  vao.bind();
  this->edgeProg_.setUniformValue("uColor", .5f, .5f, .5f, .6f);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(pts.size() / 3));
  this->edgeProg_.release();
  vao.release();
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}

double MeshWindow::angleBetween(const QVector3D &v1, const QVector3D &v2)
{
  return acos(QVector3D::dotProduct(v1, v2) / (v1.length()*v2.length()));
}

QVector3D MeshWindow::screenToBall(const QVector2D &s)
{
  double w = static_cast<double>(this->width());
  double h = static_cast<double>(this->height());
  double x = 2. * (static_cast<double>(s.x()) - w / 2.) / w;
  double y = 2. * (h / 2. - static_cast<double>(s.y())) / h;
  y *= h / w;

  QVector3D v(x, y, 0);
  double d = std::sqrt(x * x + y * y);
  if (d > 1.) {
    v.setZ(1. / (d * 1.41421356237309504880 * 1.41421356237309504880));
  } else {
    v.setZ(sqrt(1. - d*d));
  }
  return v;
}

void MeshWindow::mouseMoveEvent(QMouseEvent *event)
{
  Qt::MouseButtons buttonstate = event->buttons();

  if (buttonstate == Qt::RightButton){
    QMatrix4x4 trans;
    trans.translate((float)(event->x() - m_prev_x)*0.001f,
        -(float)(event->y() - m_prev_y)*0.001f, 0.);
    this->cameraMatrix_ = trans * this->cameraMatrix_;
  } else if (buttonstate == Qt::LeftButton){
    // first convert to sphere coordinates
    QVector3D p1 = screenToBall(QVector2D(m_prev_x, m_prev_y));
    QVector3D p2 = screenToBall(QVector2D(event->x(), event->y()));
    // get rotation axis
    QVector3D axis = QVector3D::crossProduct(p1, p2);
    axis = axis.normalized();

    // get angle
    float angle = this->angleBetween(p1, p2);

    // rotate the camera
    QQuaternion delta = QQuaternion::fromAxisAndAngle(axis, 180 * angle / M_PI);

    delta = delta.normalized();
    QMatrix4x4 rot;
    rot.rotate(delta);
    this->rotateMatrix_ = rot * this->rotateMatrix_;
    this->cameraMatrix_ = rot * this->cameraMatrix_;
  }

  m_prev_x = event->x();
  m_prev_y = event->y();

  this->updateGL();
}

void MeshWindow::mousePressEvent(QMouseEvent *event)
{
  m_prev_x = event->x();
  m_prev_y = event->y();
}

void MeshWindow::mouseReleaseEvent(QMouseEvent *event)
{
  m_prev_x = event->x();
  m_prev_y = event->y();
}

void MeshWindow::keyPressEvent(QKeyEvent *event)
{
  QString file;
  switch (event->key())
  {
  case Qt::Key_Space:
    std::cout << "reseting camera" << std::endl;
    resetView();
    break;
  case Qt::Key_Control:
    ctrl_down = true;
    break;
  case Qt::Key_M:
    if (m_starmode == FaceStar)
      m_starmode = NoStar;
    else
      m_starmode = (StarMode)(m_starmode + 1);
    std::cout << starModeString[m_starmode] << std::endl;
    this->updateGL();
    break;
  case Qt::Key_J:
    m_currentVertex--;
    if (m_currentVertex < 0)
      m_currentVertex = this->mesh_->verts.size() - 1;
    std::cout << "vertex index: " << m_currentVertex << std::endl;

    m_currentEdge--;
    if (m_currentEdge < 0)
      m_currentEdge = this->mesh_->halfEdges.size() - 1;
    std::cout << "edge index: " << m_currentEdge << std::endl;

    m_currentFace--;
    if (m_currentFace < 0)
      m_currentFace = (4 * this->mesh_->tets.size() - 1);
    std::cout << "face index: " << m_currentFace << std::endl;

    this->updateGL();
    break;
  case Qt::Key_K:
    m_currentVertex++;
    if (m_currentVertex >= (int)this->mesh_->verts.size())
      m_currentVertex = 0;
    std::cout << "vertex index: " << m_currentVertex << std::endl;

    m_currentEdge++;
    if (m_currentEdge >= (int)this->mesh_->halfEdges.size())
      m_currentEdge = 0;
    std::cout << "edge index: " << m_currentEdge << std::endl;

    m_currentFace++;
    if (m_currentFace >= (int)(4 * this->mesh_->tets.size()))
      m_currentFace = 0;
    std::cout << "face index: " << m_currentFace << std::endl;

    this->updateGL();
    break;
  case Qt::Key_W:
    break;
  case Qt::Key_S:
    break;
  case Qt::Key_F5:
    file = QFileDialog::getSaveFileName(this, "Save as...", "name",
        "PNG (*.png);; BMP (*.bmp);;TIFF (*.tiff *.tif);; JPEG (*.jpg *.jpeg)");
    this->grabFrameBuffer().save(file);
    break;
  case Qt::Key_0:
    m_bShowViolationPolytopes = !m_bShowViolationPolytopes;
    std::cout << "Show Violation Polytopes: " << m_bShowViolationPolytopes << std::endl;
    this->updateGL();
    break;

  }
}

void MeshWindow::keyReleaseEvent(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_Control:
    ctrl_down = false;
    break;
  }
}

void MeshWindow::wheelEvent(QWheelEvent *event)
{
  float zoom = event->delta() > 0 ? 1.05f : 1.f / 1.05f;
  if (ctrl_down){
    m_shrinkscale -= 0.0001f*event->delta();
    m_shrinkscale = std::max(m_shrinkscale, 0.0f);
    m_shrinkscale = std::min(m_shrinkscale, 1.0f);
    //std::cout << "shrinkscale = " << m_shrinkscale << std::endl;
    update_vbos();
  } else{
    QMatrix4x4 mat;
    mat.scale(zoom);
    this->cameraMatrix_ = mat * this->cameraMatrix_;
  }
  this->updateGL();
}

void MeshWindow::closeEvent(QCloseEvent *event)
{

}

void MeshWindow::setMesh(cleaver::TetMesh *mesh)
{
  this->mesh_ = mesh;
  m_dataBounds = cleaver::BoundingBox::merge(m_dataBounds, mesh->bounds);

  if (mesh->imported) {

    m_bMaterialFaceLock.clear();
    m_bMaterialCellLock.clear();

    for (int m = 0; m < mesh->material_count; m++)
    {
      m_bMaterialFaceLock.push_back(false);
      m_bMaterialCellLock.push_back(false);
    }
  }

  this->resetView();

  if (init){
    update_vbos();
    updateGL();
  }
}

void MeshWindow::setMesher(cleaver::CleaverMesher *mesher) {
  this->mesher_ = mesher;
}

void MeshWindow::setVolume(cleaver::Volume *volume)
{
  this->volume_ = volume;
  m_dataBounds = cleaver::BoundingBox::merge(m_dataBounds, volume->bounds());

  m_bMaterialFaceLock.clear();
  m_bMaterialCellLock.clear();
  for (int m = 0; m < volume->numberOfMaterials(); m++){
    m_bMaterialFaceLock.push_back(false);
    m_bMaterialCellLock.push_back(false);
  }
  this->resetView();

  if (init){
    updateGL();
  }
}

void MeshWindow::setup_vbos()
{
  update_vbos();
}

void MeshWindow::update_vbos()
{
  // 1. Generate a new buffer object with glGenBuffersARB().
  // 2. Bind the buffer object with glBindBufferARB().
  // 3. Copy vertex data to the buffer object with glBufferDataARB().
  if (this->mesh_){
    if ((this->mesher_ && this->mesher_->stencilsDone()) ||
      this->mesh_->imported)
      build_output_vbos();
    else
      build_bkgrnd_vbos();
  }

}

void MeshWindow::updateMesh()
{
  update_vbos();
}

void MeshWindow::build_bkgrnd_vbos()
{
  if (!this->init) return;
  this->faceData_.clear();
  this->edgeData_.clear();
  for (size_t f = 0; f < this->mesh_->faces.size(); f++)
  {
    int t1 = this->mesh_->faces[f]->tets[0];
    int t2 = this->mesh_->faces[f]->tets[1];


    bool clipped = false;
    bool exterior = false;
    bool clipborder = false;
    int num_adj_tets = 0;
    num_adj_tets += t1 >= 0 ? 1 : 0;
    num_adj_tets += t2 >= 0 ? 1 : 0;

    if (num_adj_tets == 1)
      exterior = true;
    if (m_bClipping)        {
      cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
      float d = m_4fvClippingPlane[3];

      // does plane cut through face?
      for (int v = 0; v < 3; v++)
      {
        // is vertex on proper side of the plane?
        cleaver::Vertex *vertex = this->mesh_->verts[
          this->mesh_->faces[f]->verts[v]];
        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

        if (n.dot(p) - d > 1E-4){
          clipped = true;
          break;
        }
      }

      // determine if face is on border of nonclipped faces
      if (!clipped)
      {
        // look at both adjacent tets
        if (this->mesh_->faces[f]->tets[0] > 0)
        {
          cleaver::Tet *tet = this->mesh_->tets[
            this->mesh_->faces[f]->tets[0]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
        if (this->mesh_->faces[f]->tets[1] > 0)
        {
          cleaver::Tet *tet = this->mesh_->tets[
            this->mesh_->faces[f]->tets[1]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
      }
    }

    cleaver::vec3 normal = this->mesh_->faces[f]->normal;

    if ((!clipped && exterior) || clipborder)
    {
      for (int v = 0; v < 3; v++)
      {
        cleaver::Vertex *vertex = this->mesh_->verts[
          this->mesh_->faces[f]->verts[v]];
        cleaver::Vertex *vertex2 = this->mesh_->verts[
          this->mesh_->faces[f]->verts[(v+1)%3]];

        this->faceData_.push_back(static_cast<float>(vertex->pos().x));
        this->faceData_.push_back(static_cast<float>(vertex->pos().y));
        this->faceData_.push_back(static_cast<float>(vertex->pos().z));

        this->edgeData_.push_back(static_cast<float>(vertex->pos().x));
        this->edgeData_.push_back(static_cast<float>(vertex->pos().y));
        this->edgeData_.push_back(static_cast<float>(vertex->pos().z));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().x));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().y));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().z));

        this->faceData_.push_back(static_cast<float>(normal.x));
        this->faceData_.push_back(static_cast<float>(normal.y));
        this->faceData_.push_back(static_cast<float>(normal.z));
        float r, g, b;
        if ((this->mesher_ && this->mesher_->samplingDone())) {
          float *color = color_for_label[vertex->label];
          r = color[0];
          g = color[1];
          b = color[2];
        } else {
          int color_label = -1;
          if (this->mesh_->faces[f]->tets[0] >= 0)
            color_label = this->mesh_->tets[
              this->mesh_->faces[f]->tets[0]]->mat_label;
          else if (this->mesh_->faces[f]->tets[1] >= 0)
            color_label = this->mesh_->tets[
              this->mesh_->faces[f]->tets[1]]->mat_label;

          if (color_label < 0){
            r = 0.9f;
            g = 0.9f;
            b = 0.9f;
          } else{
            float *color = color_for_label[color_label % 4];
            r = color[0];
            g = color[1];
            b = color[2];
          }
        }
        this->faceData_.push_back(r);
        this->faceData_.push_back(g);
        this->faceData_.push_back(b);
      }
    }
  }
  if (this->faceVBO_) {
    this->faceVBO_->destroy();
    delete this->faceVBO_;
    this->faceVBO_ = nullptr;
  }
  this->faceVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->faceVBO_->create();
  this->faceVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->faceVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->faceVBO_->allocate(this->faceData_.data(),
      sizeof(GL_FLOAT) * this->faceData_.size());

  this->faceProg_.bind();
  if (this->faceVAO_) {
    this->faceVAO_->destroy();
    delete this->faceVAO_;
    this->faceVAO_ = nullptr;
  }
  this->faceVAO_ = new QOpenGLVertexArrayObject(this);
  this->faceVAO_->create();
  this->faceVAO_->bind();
  this->faceProg_.enableAttributeArray(0);
  this->faceProg_.enableAttributeArray(1);
  this->faceProg_.enableAttributeArray(2);
  this->faceProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 9);
  this->faceProg_.setAttributeBuffer(1, GL_FLOAT, sizeof(GL_FLOAT) * 3, 3, sizeof(GL_FLOAT) * 9);
  this->faceProg_.setAttributeBuffer(2, GL_FLOAT, sizeof(GL_FLOAT) * 6, 3, sizeof(GL_FLOAT) * 9);

  this->faceVAO_->release();
  this->faceVBO_->release();
  this->faceProg_.release();

  if (this->edgeVBO_) {
    this->edgeVBO_->destroy();
    delete this->edgeVBO_;
    this->edgeVBO_ = nullptr;
  }
  this->edgeVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->edgeVBO_->create();
  this->edgeVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->edgeVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->edgeVBO_->allocate(this->edgeData_.data(),
      sizeof(GL_FLOAT) * this->edgeData_.size());

  this->edgeProg_.bind();
  if (this->edgeVAO_) {
    this->edgeVAO_->destroy();
    delete this->edgeVAO_;
    this->edgeVAO_ = nullptr;
  }
  this->edgeVAO_ = new QOpenGLVertexArrayObject(this);
  this->edgeVAO_->create();
  this->edgeVAO_->bind();
  this->edgeProg_.enableAttributeArray(0);
  this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

  this->edgeVAO_->release();
  this->edgeVBO_->release();
  this->edgeProg_.release();

  this->cutData_.clear();
  this->violData_.clear();
  // Now update Cuts List
  if ((this->mesher_ && this->mesher_->interfacesComputed()))
  {
    std::vector<GLfloat> ViolationData;
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator
      edgesIter = this->mesh_->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while (edgesIter != this->mesh_->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      edge->evaluated = false;
      edgesIter++;
    }

    // now grab each cut only once
    edgesIter = this->mesh_->halfEdges.begin();
    while (edgesIter != this->mesh_->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      if (edge->cut && edge->cut->order() == 1 && !edge->evaluated)
      {
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().x));
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().y));
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().z));

        if (edge->cut->violating){
          this->violData_.push_back(static_cast<float>(edge->cut->pos().x));
          this->violData_.push_back(static_cast<float>(edge->cut->pos().y));
          this->violData_.push_back(static_cast<float>(edge->cut->pos().z));

          this->violData_.push_back(static_cast<float>(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().x));
          this->violData_.push_back(static_cast<float>(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().y));
          this->violData_.push_back(static_cast<float>(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().z));
        }

        edge->evaluated = true;
        edge->mate->evaluated = true;
      }

      edgesIter++;
    }

    if (this->cutVBO_) {
      this->cutVBO_->destroy();
      delete this->cutVBO_;
      this->cutVBO_ = nullptr;
    }
    this->cutVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->cutVBO_->create();
    this->cutVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->cutVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->cutVBO_->allocate(this->cutData_.data(),
        sizeof(GL_FLOAT) * this->cutData_.size());

    this->edgeProg_.bind();
    if (this->cutVAO_) {
      this->cutVAO_->destroy();
      delete this->cutVAO_;
      this->cutVAO_ = nullptr;
    }
    this->cutVAO_ = new QOpenGLVertexArrayObject(this);
    this->cutVAO_->create();
    this->cutVAO_->bind();
    this->edgeProg_.enableAttributeArray(0);
    this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

    this->cutVAO_->release();
    this->cutVBO_->release();
    this->edgeProg_.release();

    if (this->violVBO_) {
      this->violVBO_->destroy();
      delete this->violVBO_;
      this->violVBO_ = nullptr;
    }
    this->violVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->violVBO_->create();
    this->violVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->violVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->violVBO_->allocate(this->violData_.data(),
        sizeof(GL_FLOAT) * this->violData_.size());

    this->edgeProg_.bind();
    if (this->violVAO_) {
      this->violVAO_->destroy();
      delete this->violVAO_;
      this->violVAO_ = nullptr;
    }
    this->violVAO_ = new QOpenGLVertexArrayObject(this);
    this->violVAO_->create();
    this->violVAO_->bind();
    this->edgeProg_.enableAttributeArray(0);
    this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

    this->violVAO_->release();
    this->violVBO_->release();
    this->edgeProg_.release();

  }
}

void MeshWindow::build_output_vbos()
{
  if (!this->init) return;
  this->faceData_.clear();
  this->edgeData_.clear();
  for (size_t f = 0; f < this->mesh_->faces.size(); f++)
  {
    int t1 = this->mesh_->faces[f]->tets[0];
    int t2 = this->mesh_->faces[f]->tets[1];

    bool clipped = false;
    bool exterior = false;
    bool clipborder = false;
    bool surface = false;
    bool force = false;
    int num_adj_tets = 0;
    num_adj_tets += t1 >= 0 ? 1 : 0;
    num_adj_tets += t2 >= 0 ? 1 : 0;

    if (num_adj_tets == 1)
      exterior = true;

    if (num_adj_tets == 2 && 
      this->mesh_->tets[t1]->mat_label !=
      this->mesh_->tets[t2]->mat_label)
      surface = true;

    int m1 = -1;
    int m2 = -2;
    if (t1 >= 0){
      m1 = this->mesh_->tets[t1]->mat_label;
      if (m1 < (int)m_bMaterialCellLock.size() && m_bMaterialCellLock[m1])
        force = true;
    }
    if (t2 >= 0){
      m2 = this->mesh_->tets[t2]->mat_label;
      if (m2 < (int)m_bMaterialCellLock.size() && m_bMaterialCellLock[m2])
        force = true;
    }
    if (m1 == m2) {
      force = false;
    }
    if (surface) {
      force = (m2 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m2]) ||
        (m1 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m1]);
    }
    if (m_bClipping)
    {
      cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
      float d = m_4fvClippingPlane[3];

      // does plane cut through face?
      for (int v = 0; v < 3; v++)
      {
        // is vertex on proper side of the plane?
        cleaver::Vertex *vertex = this->mesh_->verts[
          this->mesh_->faces[f]->verts[v]];
        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

        if (n.dot(p) - d > 1E-4){
          clipped = true;
          break;
        }
      }

      // determine if face is on border of nonclipped faces
      if (!clipped)
      {
        // look at both adjacent tets
        if (this->mesh_->faces[f]->tets[0] > 0)
        {
          cleaver::Tet *tet = this->mesh_->tets[
            this->mesh_->faces[f]->tets[0]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
        if (this->mesh_->faces[f]->tets[1] > 0)
        {
          cleaver::Tet *tet = this->mesh_->tets[
            this->mesh_->faces[f]->tets[1]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
      }
    }
    bool draw = ((!clipped && exterior) || clipborder) && !m_bSurfacesOnly;
    draw |= force;
    draw |= m_bSurfacesOnly && !clipped && surface;
    draw |= m_colorUpdate && !clipped && (surface == m_bSurfacesOnly);
    if (draw)
    {
      cleaver::Tet *tet1 = 0;
      cleaver::Tet *tet2 = 0;

      float default_color[3] = { 0.9f, 0.9f, 0.9f };
      float good_color[3] = { 0.3f, 1.0f, 0.0f };
      float  bad_color[3] = { 1.0f, 0.3f, 0.0f };

      float *color1 = default_color, *color2 = default_color;
      if (this->mesh_->faces[f]->tets[0] >= 0){

        tet1 = this->mesh_->tets[
          this->mesh_->faces[f]->tets[0]];
        if (m_bColorByQuality){
          float t = tet1->minAngle() / 90.0f;
          color1[0] = (1 - t)*bad_color[0] + t*good_color[0];
          color1[1] = (1 - t)*bad_color[1] + t*good_color[1];
          color1[2] = (1 - t)*bad_color[2] + t*good_color[2];
        } else {
          color1 = color_for_label[(int)tet1->mat_label];
        }
      }
      if (this->mesh_->faces[f]->tets[1] >= 0){

        tet2 = this->mesh_->tets[
          this->mesh_->faces[f]->tets[1]];
        if (m_bColorByQuality){
          float t = tet2->minAngle() / 90.0f;
          color2[0] = (1 - t)*bad_color[0] + t*good_color[0];
          color2[1] = (1 - t)*bad_color[1] + t*good_color[1];
          color2[2] = (1 - t)*bad_color[2] + t*good_color[2];
        } else {
          color2 = color_for_label[(int)tet2->mat_label];
        }
      }
      if (this->mesh_->faces[f]->tets[0] < 0)
        color1 = color2;
      if (this->mesh_->faces[f]->tets[1] < 0)
        color2 = color1;

      cleaver::vec3 normal = this->mesh_->faces[f]->normal;

      // set vertex positions and colors
      for (int v = 0; v < 3; v++)
      {
        cleaver::Vertex *vertex = this->mesh_->verts[
          this->mesh_->faces[f]->verts[v]];
        cleaver::Vertex *vertex2 = this->mesh_->verts[
          this->mesh_->faces[f]->verts[(v+1)%3]];

        this->faceData_.push_back(static_cast<float>(vertex->pos().x));
        this->faceData_.push_back(static_cast<float>(vertex->pos().y));
        this->faceData_.push_back(static_cast<float>(vertex->pos().z));

        this->edgeData_.push_back(static_cast<float>(vertex->pos().x));
        this->edgeData_.push_back(static_cast<float>(vertex->pos().y));
        this->edgeData_.push_back(static_cast<float>(vertex->pos().z));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().x));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().y));
        this->edgeData_.push_back(static_cast<float>(vertex2->pos().z));

        this->faceData_.push_back(static_cast<float>(normal.x));
        this->faceData_.push_back(static_cast<float>(normal.y));
        this->faceData_.push_back(static_cast<float>(normal.z));

        faceData_.push_back(0.5*(color1[0] + color2[0]));
        faceData_.push_back(0.5*(color1[1] + color2[1]));
        faceData_.push_back(0.5*(color1[2] + color2[2]));
      }

    }
  }
  m_colorUpdate = false;

  if (this->faceVBO_) {
    this->faceVBO_->destroy();
    delete this->faceVBO_;
    this->faceVBO_ = nullptr;
  }
  this->faceVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->faceVBO_->create();
  this->faceVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->faceVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->faceVBO_->allocate(this->faceData_.data(),
      sizeof(GL_FLOAT) * this->faceData_.size());

  this->faceProg_.bind();
  if (this->faceVAO_) {
    this->faceVAO_->destroy();
    delete this->faceVAO_;
    this->faceVAO_ = nullptr;
  }
  this->faceVAO_ = new QOpenGLVertexArrayObject(this);
  this->faceVAO_->create();
  this->faceVAO_->bind();
  this->faceProg_.enableAttributeArray(0);
  this->faceProg_.enableAttributeArray(1);
  this->faceProg_.enableAttributeArray(2);
  this->faceProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 9);
  this->faceProg_.setAttributeBuffer(1, GL_FLOAT, sizeof(GL_FLOAT) * 3, 3, sizeof(GL_FLOAT) * 9);
  this->faceProg_.setAttributeBuffer(2, GL_FLOAT, sizeof(GL_FLOAT) * 6, 3, sizeof(GL_FLOAT) * 9);

  this->faceVAO_->release();
  this->faceVBO_->release();
  this->faceProg_.release();

  if (this->edgeVBO_) {
    this->edgeVBO_->destroy();
    delete this->edgeVBO_;
    this->edgeVBO_ = nullptr;
  }
  this->edgeVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->edgeVBO_->create();
  this->edgeVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->edgeVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->edgeVBO_->allocate(this->edgeData_.data(),
      sizeof(GL_FLOAT) * this->edgeData_.size());

  this->edgeProg_.bind();
  if (this->edgeVAO_) {
    this->edgeVAO_->destroy();
    delete this->edgeVAO_;
    this->edgeVAO_ = nullptr;
  }
  this->edgeVAO_ = new QOpenGLVertexArrayObject(this);
  this->edgeVAO_->create();
  this->edgeVAO_->bind();
  this->edgeProg_.enableAttributeArray(0);
  this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

  this->edgeVAO_->release();
  this->edgeVBO_->release();
  this->edgeProg_.release();

  this->cutData_.clear();
  // Now update Cuts List
  if ((this->mesher_ && this->mesher_->interfacesComputed()))
  {
    std::map<std::pair<int, int>,
      cleaver::HalfEdge*>::iterator edgesIter = this->mesh_->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while (edgesIter != this->mesh_->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      edge->evaluated = false;
      edgesIter++;
    }

    // now grab each cut only once
    edgesIter = this->mesh_->halfEdges.begin();
    while (edgesIter != this->mesh_->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      if (edge->cut && edge->cut->order() == 1 && !edge->evaluated)
      {
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().x));
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().y));
        this->cutData_.push_back(static_cast<float>(edge->cut->pos().z));
        edge->evaluated = true;
        edge->mate->evaluated = true;
      }

      edgesIter++;
    }

    if (this->cutVBO_) {
      this->cutVBO_->destroy();
      delete this->cutVBO_;
      this->cutVBO_ = nullptr;
    }
    this->cutVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->cutVBO_->create();
    this->cutVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->cutVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->cutVBO_->allocate(this->cutData_.data(),
        sizeof(GL_FLOAT) * this->cutData_.size());

    this->edgeProg_.bind();
    if (this->cutVAO_) {
      this->cutVAO_->destroy();
      delete this->cutVAO_;
      this->cutVAO_ = nullptr;
    }
    this->cutVAO_ = new QOpenGLVertexArrayObject(this);
    this->cutVAO_->create();
    this->cutVAO_->bind();
    this->edgeProg_.enableAttributeArray(0);
    this->edgeProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 3);

    this->cutVAO_->release();
    this->cutVBO_->release();
    this->edgeProg_.release();
  }
}
