#include "MeshWindow.h"
#include "TrackballCamera.h"
#include <Cleaver/BoundingBox.h>
#include <Cleaver/vec3.h>
#include "../../lib/cleaver/Plane.h"
#include <QMouseEvent>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "Shaders/Shaders.h"
#include <QMatrix4x4>
#include "MainWindow.h"
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
float DefaultBBoxColor[4] = { 0, 0, 0, 1.0 };
float DefaultCutsColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
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

MeshWindow::MeshWindow(QObject *parent) :
QGLWidget(QGLFormat(QGL::SampleBuffers), qobject_cast<QWidget *>(parent)), 
faceProg_(this), edgeProg_(this)
{
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setMouseTracking(true);
  this->setFocusPolicy(Qt::ClickFocus);
  //this->setFocusPolicy(Qt::StrongFocus);
  this->faceVAO_ = this->edgeVAO_ = this->cutVAO_ = this->violVAO_ = NULL;
  this->faceVBO_ = this->edgeVBO_ = this->cutVBO_ = this->violVBO_ = NULL;
  m_mesh = NULL;
  m_volume = NULL;
  m_mesher = NULL;
  init = false;

  initializeOptions();
}

MeshWindow::~MeshWindow()
{
  emit closed(this);
}

QSize MeshWindow::sizeHint() const
{
  return this->maximumSize();
}

void MeshWindow::setDefaultOptions()
{
  initializeOptions();
}

void MeshWindow::resetView()
{
  cleaver::BoundingBox bb;
  if (m_volume)
    bb = m_volume->bounds();
  else
    bb = m_mesh->bounds;
  this->cameraMatrix_ = this->rotateMatrix_ = QMatrix4x4();
  float scale = 1. / std::max(bb.size.x, std::max(bb.size.y, bb.size.z));
  QMatrix4x4 trans;
  trans.translate(-bb.center().x, -bb.center().y, -bb.center().z);
  this->cameraMatrix_ = trans * this->cameraMatrix_;
  trans = QMatrix4x4();
  trans.scale(scale);
  this->cameraMatrix_ = trans * this->cameraMatrix_;
  trans = QMatrix4x4();
  trans.rotate(30, 1, 0, 0);
  trans.rotate(-10, 0, 1, 0);
  this->cameraMatrix_ = trans * this->cameraMatrix_;
  this->rotateMatrix_ = trans;

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

  memcpy(m_4fvBBoxColor, DefaultBBoxColor, 4 * sizeof(float));
  memcpy(m_4fvCutsColor, DefaultCutsColor, 4 * sizeof(float));

  // for adjacency visualization and debugging
  m_starmode = NoStar;
  m_currentVertex = 0;
  m_currentEdge = 0;
  m_currentFace = 0;
  m_shrinkscale = 0.0;
  //m_vertexData = NULL;

  this->resize(this->maximumSize());
}

void MeshWindow::initializeShaders()
{
  std::string testV((const char *)default_vert);
  std::string testF((const char *)default_frag);
  std::string fs, vs;
  // face shaders
  vs = "#version 330\n";
  vs = vs + "layout (location = 0) in vec3 aPos;\n";
  vs = vs + "layout (location = 1) in vec3 aNormal;\n";
  vs = vs + "layout (location = 2) in int aColor;\n";
  vs = vs + "uniform mat4 uProjection;\n";
  vs = vs + "uniform mat4 uTransform;\n";
  vs = vs + "out vec3 oPos;\n";
  vs = vs + "out vec3 oNormal;\n";
  vs = vs + "flat out int oColor;\n";
  vs = vs + "void main() {\n";
  vs = vs + "  vec4 ans = uTransform * vec4(aPos.x, aPos.y, aPos.z, 1.0); \n";
  vs = vs + "  oPos = ans.xyz;\n ";
  vs = vs + "  gl_Position = uProjection * vec4(aPos / 30.,1.);\n//uProjection * ans;\n";
  vs = vs + "  oColor = aColor;\n";
  vs = vs + "  oNormal = normalize((uTransform * vec4(aNormal,0.)).xyz);\n";
  vs = vs + "}\n";
  //fragment
  fs = "#version 330\n";
  fs = fs + "flat in int oColor;\n";
  fs = fs + "in vec3 oNormal;\n";
  fs = fs + "in vec3 oPos;\n";
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
  fs = fs + "  float r = float(oColor >> 24);\n";
  fs = fs + "  float g = float((oColor >> 16) & 0x000000FF);\n";
  fs = fs + "  float b = float((oColor >> 8) & 0x000000FF);\n";
  fs = fs + "  float a = float((oColor >> 0) & 0x000000FF);\n";
  fs = fs + "  vec4 col = vec4(r,g,b,a);\n";
  fs = fs + "  fragColor = vec4(spc * vec3(1.,1.,1.) + \n";
  fs = fs + "                      clamp(col.xyz * (amb + dif),0.,1.),1.);\n";
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

  fs = vs = "";
  //edge shaders
  vs = "#version 330\n";
  vs = vs + "layout (location = 0) in vec3 aPos;\n";
  vs = vs + "void main() {\n";
  vs = vs + "  gl_Position = vec4(aPos,1.);\n ";
  vs = vs + "}\n";
  fs = "uniform vec4 oColor;\n";
  fs = fs + "void main() {\n";
  fs = fs + "  gl_FragColor = oColor;\n";
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
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);

  setup_vbos();

  initializeShaders();

  init = true;
}

void createFrustumMatrix(float *matrix, float left, float right,
  float bottom, float top, float zNear, float zFar)
{
  float temp, temp2, temp3, temp4;
  temp = 2.0f * zNear;
  temp2 = right - left;
  temp3 = top - bottom;
  temp4 = zFar - zNear;
  matrix[0] = temp / temp2;
  matrix[1] = 0.0f;
  matrix[2] = 0.0f;
  matrix[3] = 0.0f;
  matrix[4] = 0.0f;
  matrix[5] = temp / temp3;
  matrix[6] = 0.0f;
  matrix[7] = 0.0f;
  matrix[8] = (right + left) / temp2;
  matrix[9] = (top + bottom) / temp3;
  matrix[10] = (-zFar - zNear) / temp4;
  matrix[11] = -1.0f;
  matrix[12] = 0.0f;
  matrix[13] = 0.0f;
  matrix[14] = (-temp * zFar) / temp4;
  matrix[15] = 0.0f;
}

void MeshWindow::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
  m_width = w;
  m_height = h;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  GLdouble aspectRatio = (GLdouble)w / (float)h;
  GLdouble zNear = 0.1;
  GLdouble zFar = 2000.0;
  GLdouble yFovInDegrees = 45;

  GLdouble top = zNear * tan(yFovInDegrees * M_PI / 360.0);
  GLdouble bottom = -top;
  GLdouble right = top*aspectRatio;
  GLdouble left = -right;

  glFrustum(left, right, bottom, top, zNear, zFar);
  float mtx[16];
  createFrustumMatrix(mtx, left, right, bottom, top, zNear, zFar);
  if (this->m_mesh) {
    QMatrix4x4 mat;
    mat.perspective(yFovInDegrees, aspectRatio, zNear, zFar);
    this->faceProg_.bind();
    this->faceProg_.setUniformValueArray("uProjection", &mat, 1);
    this->faceProg_.release();
    //this->edgeProg_.setUniformValueArray("uProjection", &mat, 1);
  }
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
#ifdef WIN32
  if (!init)
    initializeGL();
#endif

  makeCurrent();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_bOpenGLError)
    return;

  float glmat[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, glmat);
  for (size_t i = 0; i < 16; i++) {
    if (boost::math::isnan(glmat[i])) {
      std::cout << "Recovering from a NaN matrix error..." << std::endl;
      this->resetView();
      glMultMatrixf(this->cameraMatrix_.constData());
      break;
    }
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (m_bShowAxis) {
    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0);
    drawAxis();
    glEnable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
  }
  QMatrix4x4 mat;
  mat.translate(0, 0, -3);
  mat = mat * this->cameraMatrix_;
  glMultMatrixf(mat.constData());

  if (this->m_mesh) {
    this->faceProg_.bind();
    this->faceProg_.setUniformValueArray("uTransform", &mat, 1);
    this->faceProg_.release();
    //this->edgeProg_.setUniformValueArray("uTransform", &mat, 1);
  }
  if (m_bShowBBox){
    glColor4f(0.0f, 0.0f, 0.0f, 0.9f);

    glDisable(GL_LIGHTING);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0);
    drawBox((this->m_mesh!=NULL)?(this->m_mesh->bounds):(this->m_volume->bounds()));
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
  }

  if (m_mesh){
    if (m_bShowFaces)
      drawFaces();

    if (m_bShowCuts && m_volume)
      drawCuts();

    if (m_bShowEdges)
      drawEdges();

    glLineWidth(3.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < badEdges.size() / 2; i += 2)
    {
      glVertex3f(badEdges[i].x, badEdges[i].y, badEdges[i].z);
      glVertex3f(badEdges[i + 1].x, badEdges[i + 1].y, badEdges[i + 1].z);
    }
    glEnd();
  }

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

  if (m_bShowViolationPolytopes)
    drawViolationPolytopesForVertices();

  if (m_bClipping && m_bShowClippingPlane)
    drawClippingPlane();
}

void MeshWindow::drawVertexStar(int v)
{
  // draw vertex
  cleaver::Vertex *vertex = m_mesh->verts[v];
  glPointSize(4.0f);
  glBegin(GL_POINTS);
  glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
  glVertex3f(vertex->pos().x, vertex->pos().y, vertex->pos().z);
  glEnd();


  //std::vector<Cleaver::Tet*> tetlist = m_mesh->tetsAroundVertex(vertex);

  glDisable(GL_LIGHTING);


  //--- Draw Faces Around Vertex ---
  std::vector<cleaver::HalfFace*> facelist = m_mesh->facesAroundVertex(vertex);

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
  std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_mesh->halfEdges.begin();
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
  std::vector<cleaver::HalfFace*> facelist = m_mesh->facesAroundEdge(e);

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
  std::vector<cleaver::Tet*> tetlist = m_mesh->tetsAroundEdge(e);
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
  cleaver::HalfFace *half_face = &m_mesh->halfFaces[face];

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
  std::vector<cleaver::Tet*> tetlist = m_mesh->tetsAroundFace(half_face);
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
  if (m_mesher->alphasComputed())
  {
    for (size_t v = 0; v < m_mesh->verts.size(); v++)
    {
      glColor3f(1.0f, 0.5f, 0.5f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glPolygonOffset(1.0f, 1.0f);
      glEnable(GL_POLYGON_OFFSET_FILL);
      drawViolationPolytopeForVertex(v);
      glDisable(GL_POLYGON_OFFSET_FILL);

      glColor3f(0.0f, 0.0f, 0.0f);
      glEnable(GL_LINE_SMOOTH);
      glEnable(GL_BLEND);
      glDisable(GL_LIGHTING);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(1.0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

      drawViolationPolytopeForVertex(v);


      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_BLEND);
    }
  }
}

void MeshWindow::drawViolationPolytopeForVertex(int v)
{
  // get vertex
  cleaver::Vertex *vertex = m_mesh->verts[v];

  // get adjacency data
  //std::vector<Cleaver::HalfEdge*> adjEdges = m_mesh->edgesAroundVertex(vertex);
  std::vector<cleaver::Tet*>      adjTets = m_mesh->tetsAroundVertex(vertex);

  glDisable(GL_LIGHTING);
  glBegin(GL_TRIANGLES);
  // phase 1 - simply connect edge violations
  for (size_t t = 0; t < adjTets.size(); t++)
  {
    // each tet t has 3 edges incident to vertex v
    int count = 0;
    std::vector<cleaver::HalfEdge*> edges = m_mesh->edgesAroundTet(adjTets[t]);
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
  if (!this->m_mesh || this->faceData_.empty()) return;
  // shader pogram
  this->faceProg_.bind();
  this->faceVAO_->bind();
  QMatrix4x4 mat;
  mat.translate(0, 0, -3);
  mat = mat * this->cameraMatrix_;
  QMatrix4x4 pmat;
  GLdouble aspectRatio = (GLdouble)m_width / (float)m_height;
  GLdouble zNear = 0.1;
  GLdouble zFar = 2000.0;
  GLdouble yFovInDegrees = 45;
  pmat.perspective(yFovInDegrees, aspectRatio, zNear, zFar);
  this->faceProg_.setUniformValueArray("uTransform", &mat, 1);
  this->faceProg_.setUniformValueArray("uPerspective", &pmat, 1);
  glDrawArrays(GL_TRIANGLES, 0, this->faceData_.size() / 7);
  this->faceVAO_->release();
  this->faceProg_.release();
}

void MeshWindow::drawEdges()
{
  if (!this->m_mesh || this->edgeData_.empty()) return;
  glLineWidth(1.5);
  this->edgeProg_.bind();
  this->edgeProg_.setUniformValue("oColor", 0.0f, 0.0f, 0.0f, 0.2f);
  this->edgeVAO_->bind();
  glDrawArrays(GL_LINES, 0,
    static_cast<GLsizei>(this->edgeData_.size() / 6));
  this->edgeVAO_->release();
  this->edgeProg_.release();
}

void MeshWindow::drawCuts()
{
  if (!this->m_mesh || this->cutData_.empty() || this->violData_.empty()) return;
  //violating cuts
  glPointSize(4.0);

  this->edgeProg_.bind();
  this->edgeProg_.setUniformValue("oColor", m_4fvCutsColor[0],
    m_4fvCutsColor[1], m_4fvCutsColor[2], m_4fvCutsColor[3]);
  this->cutVAO_->bind();
  glDrawArrays(GL_POINTS, 0, this->cutData_.size() / 3);
  this->cutVAO_->release();
  this->edgeProg_.release();

  //violating vectors
  this->edgeProg_.bind();
  this->edgeProg_.setUniformValue("oColor", 1.0f, 0.5f, 0.5f, 1.0f);
  this->violVAO_->bind();
  glDrawArrays(GL_LINES, 0,
    static_cast<GLsizei>(this->violData_.size() / 3));
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

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glColor4f(0.5f, 0.5f, 0.5f, 0.3f);
  glBegin(GL_TRIANGLES);
  glVertex3f(p1.x, p1.y, p1.z);
  glVertex3f(p2.x, p2.y, p2.z);
  glVertex3f(p3.x, p3.y, p3.z);
  glVertex3f(p3.x, p3.y, p3.z);
  glVertex3f(p4.x, p4.y, p4.z);
  glVertex3f(p1.x, p1.y, p1.z);
  glEnd();
  glDisable(GL_BLEND);
}


void MeshWindow::drawBox(const cleaver::BoundingBox &box)
{
  float e = 0.01f;
  float x = box.origin.x - e;
  float y = box.origin.y - e;
  float z = box.origin.z - e;
  float w = box.size.x + 2 * e;
  float h = box.size.y + 2 * e;
  float d = box.size.z + 2 * e;

  glPushMatrix();
  glTranslatef(x, y, z);

  glBegin(GL_LINES);

  // front face
  glVertex3f(0, 0, 0);
  glVertex3f(w, 0, 0);

  glVertex3f(w, 0, 0);
  glVertex3f(w, h, 0);

  glVertex3f(w, h, 0);
  glVertex3f(0, h, 0);

  glVertex3f(0, h, 0);
  glVertex3f(0, 0, 0);

  // back face
  glVertex3f(0, 0, d);
  glVertex3f(w, 0, d);

  glVertex3f(w, 0, d);
  glVertex3f(w, h, d);

  glVertex3f(w, h, d);
  glVertex3f(0, h, d);

  glVertex3f(0, h, d);
  glVertex3f(0, 0, d);

  // remaining edges
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, d);

  glVertex3f(w, 0, 0);
  glVertex3f(w, 0, d);

  glVertex3f(w, h, 0);
  glVertex3f(w, h, d);

  glVertex3f(0, h, 0);
  glVertex3f(0, h, d);
  glEnd();
  glPopMatrix();
}

void MeshWindow::drawAxis()
{
  float ratio = (float)m_width / (float)m_height;
  glPushMatrix();

  glTranslatef(-4.2*ratio, +4.0, -15);

  glMultMatrixf(this->rotateMatrix_.constData());

  glBegin(GL_LINES);

  glColor4f(1.f, 0.f, 0.f, 1.f);
  glVertex3f(0, 0, 0);
  glVertex3f(1, 0, 0);

  glVertex3f(1.2f, 0.2f, 0);
  glVertex3f(1.4f, -0.2f, 0);

  glVertex3f(1.4f, 0.2f, 0);
  glVertex3f(1.2f, -0.2f, 0);

  glColor4f(0.f, 1.f, 0.f, 1.f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 1, 0);

  glVertex3f(0, 1.2f, 0);
  glVertex3f(0, 1.4f, 0);

  glVertex3f(0, 1.4f, 0);
  glVertex3f(0.1f, 1.6f, 0);

  glVertex3f(0, 1.4f, 0);
  glVertex3f(-0.1f, 1.6f, 0);

  glColor4f(0.f, 0.f, 1.f, 1.f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 1);

  glVertex3f(0, 0.2f, 1.4f);
  glVertex3f(0, 0.2f, 1.2f);

  glVertex3f(0, 0.2f, 1.2f);
  glVertex3f(0, -0.2f, 1.4f);

  glVertex3f(0, -0.2f, 1.4f);
  glVertex3f(0, -0.2f, 1.2f);

  glEnd();
  glPopMatrix();
}

void MeshWindow::drawOTCell(cleaver::OTCell *cell)
{
  if (cell){
    float *color = color_for_label[cell->level % 8];
    glColor3fv(color);
    drawBox(cell->bounds);
    for (int i = 0; i < 8; i++)
      drawOTCell(cell->children[i]);
  }
}

void MeshWindow::drawTree()
{
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);

  cleaver::Octree *tree = m_mesher->getTree();
  drawOTCell(tree->root());

  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
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
      m_currentVertex = m_mesh->verts.size() - 1;
    std::cout << "vertex index: " << m_currentVertex << std::endl;

    m_currentEdge--;
    if (m_currentEdge < 0)
      m_currentEdge = m_mesh->halfEdges.size() - 1;
    std::cout << "edge index: " << m_currentEdge << std::endl;

    m_currentFace--;
    if (m_currentFace < 0)
      m_currentFace = (4 * m_mesh->tets.size() - 1);
    std::cout << "face index: " << m_currentFace << std::endl;

    this->updateGL();
    break;
  case Qt::Key_K:
    m_currentVertex++;
    if (m_currentVertex >= (int)m_mesh->verts.size())
      m_currentVertex = 0;
    std::cout << "vertex index: " << m_currentVertex << std::endl;

    m_currentEdge++;
    if (m_currentEdge >= (int)m_mesh->halfEdges.size())
      m_currentEdge = 0;
    std::cout << "edge index: " << m_currentEdge << std::endl;

    m_currentFace++;
    if (m_currentFace >= (int)(4 * m_mesh->tets.size()))
      m_currentFace = 0;
    std::cout << "face index: " << m_currentFace << std::endl;

    this->updateGL();
    break;
  case Qt::Key_W:
    break;
  case Qt::Key_S:
    break;
  case Qt::Key_F5:
    dumpSVGImage("screenshot");
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
  m_mesh = mesh;
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

void MeshWindow::setVolume(cleaver::Volume *volume)
{
  m_volume = volume;
  m_dataBounds = cleaver::BoundingBox::merge(m_dataBounds, volume->bounds());

  if (!m_mesher)
    m_mesher = new cleaver::CleaverMesher(volume);
  else{
    m_mesher->setVolume(volume);
  }

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

  if (m_mesh){
    if ((m_mesher && mesher()->stencilsDone()) || m_mesh->imported)
      build_output_vbos();
    else
      build_bkgrnd_vbos();
  }

}

void MeshWindow::updateMesh()
{
  update_vbos();
}

cleaver::vec3 computeIncenter(const cleaver::vec3 &v1,
  const cleaver::vec3 &v2, const cleaver::vec3 &v3)
{
  cleaver::vec3 result = cleaver::vec3::zero;

  float a = length(v2 - v3);
  float b = length(v3 - v1);
  float c = length(v1 - v2);
  float perimeter = a + b + c;

  result.x = (a*v1.x + b*v2.x + c*v3.x) / perimeter;
  result.y = (a*v1.y + b*v2.y + c*v3.y) / perimeter;
  result.z = (a*v1.z + b*v2.z + c*v3.z) / perimeter;

  return result;
}

cleaver::vec3 computeIncenter(cleaver::Tet *tet)
{
  cleaver::vec3 result = cleaver::vec3::zero;

  cleaver::vec3 v1 = tet->verts[0]->pos();
  cleaver::vec3 v2 = tet->verts[1]->pos();
  cleaver::vec3 v3 = tet->verts[2]->pos();
  cleaver::vec3 v4 = tet->verts[3]->pos();

  float a = 0.5f*length(cross(v2 - v3, v4 - v3));
  float b = 0.5f*length(cross(v3 - v1, v4 - v1));
  float c = 0.5f*length(cross(v1 - v2, v4 - v2));
  float d = 0.5f*length(cross(v3 - v2, v1 - v2));
  float total_area = a + b + c + d;

  result.x = (a*v1.x + b*v2.x + c*v3.x + d*v4.x) / total_area;
  result.y = (a*v1.y + b*v2.y + c*v3.y + d*v4.y) / total_area;
  result.z = (a*v1.z + b*v2.z + c*v3.z + d*v4.z) / total_area;

  return result;
}

void MeshWindow::build_bkgrnd_vbos()
{
  this->faceData_.clear();
  this->edgeData_.clear();
  for (size_t f = 0; f < m_mesh->faces.size(); f++)
  {
    int t1 = m_mesh->faces[f]->tets[0];
    int t2 = m_mesh->faces[f]->tets[1];


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
        cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
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
        if (m_mesh->faces[f]->tets[0] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
        if (m_mesh->faces[f]->tets[1] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

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

    cleaver::vec3 normal = m_mesh->faces[f]->normal;

    if ((!clipped && exterior) || clipborder)
    {
      for (int v = 0; v < 3; v++)
      {
        cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
        cleaver::Vertex *vertex2 = m_mesh->verts[m_mesh->faces[f]->verts[(v+1)%3]];

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
        uint32_t r, g, b, a;
        if (m_mesher && m_mesher->samplingDone())
        {
          float *color = color_for_label[vertex->label];
          r = static_cast<uint32_t>(color[0] * 255);
          g = static_cast<uint32_t>(color[1] * 255);
          b = static_cast<uint32_t>(color[2] * 255);
          a = 255;
        } else{
          int color_label = -1;
          if (m_mesh->faces[f]->tets[0] >= 0)
            color_label = m_mesh->tets[m_mesh->faces[f]->tets[0]]->mat_label;
          else if (m_mesh->faces[f]->tets[1] >= 0)
            color_label = m_mesh->tets[m_mesh->faces[f]->tets[1]]->mat_label;

          if (color_label < 0){
            r = static_cast<uint32_t>(0.9f * 255);
            g = static_cast<uint32_t>(0.9f * 255);
            b = static_cast<uint32_t>(0.9f * 255);
            a = 255;
          } else{
            float *color = color_for_label[color_label % 4];
            r = static_cast<uint32_t>(color[0] * 255);
            g = static_cast<uint32_t>(color[1] * 255);
            b = static_cast<uint32_t>(color[2] * 255);
            a = 255;
          }
        }
        uint32_t col = (r << 24) | (g << 16) | (b << 8) & a;
        float val;
        memcpy(&val, &col, 4);
        this->faceData_.push_back(val);
      }
    }
  }
  if (this->faceVBO_) {
    this->faceVBO_->destroy();
    delete this->faceVBO_;
    this->faceVBO_ = NULL;
  }
  this->faceVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->faceVBO_->create();
  this->faceVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->faceVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->faceVBO_->allocate(this->faceData_.data(),
    sizeof(GL_FLOAT) * this->faceData_.size());

  if (this->faceVAO_) {
    this->faceVAO_->destroy();
    delete this->faceVAO_;
    this->faceVAO_ = NULL;
  }
  this->faceVAO_ = new QOpenGLVertexArrayObject(this);
  this->faceVAO_->create();
  this->faceVAO_->bind();
  this->faceProg_.enableAttributeArray(0);
  this->faceProg_.enableAttributeArray(1);
  this->faceProg_.enableAttributeArray(2);
  this->faceProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 7);
  this->faceProg_.setAttributeBuffer(1, GL_FLOAT, sizeof(GL_FLOAT) * 3, 3, sizeof(GL_FLOAT) * 7);
  this->faceProg_.setAttributeBuffer(2, GL_FLOAT, sizeof(GL_FLOAT) * 6, 1, sizeof(GL_FLOAT) * 7);

  this->faceVAO_->release();
  this->faceVBO_->release();
  this->faceProg_.release();

  if (this->edgeVBO_) {
    this->edgeVBO_->destroy();
    delete this->edgeVBO_;
    this->edgeVBO_ = NULL;
  }
  this->edgeVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->edgeVBO_->create();
  this->edgeVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->edgeVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->edgeVBO_->allocate(this->edgeData_.data(),
    sizeof(GL_FLOAT) * this->edgeData_.size());

  if (this->edgeVAO_) {
    this->edgeVAO_->destroy();
    delete this->edgeVAO_;
    this->edgeVAO_ = NULL;
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
  if (m_mesher && m_mesher->interfacesComputed())
  {
    std::vector<GLfloat> ViolationData;
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator 
      edgesIter = m_mesh->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while (edgesIter != m_mesh->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      edge->evaluated = false;
      edgesIter++;
    }

    // now grab each cut only once
    edgesIter = m_mesh->halfEdges.begin();
    while (edgesIter != m_mesh->halfEdges.end())
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
      this->cutVBO_ = NULL;
    }
    this->cutVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->cutVBO_->create();
    this->cutVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->cutVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->cutVBO_->allocate(this->cutData_.data(),
      sizeof(GL_FLOAT) * this->cutData_.size());

    if (this->cutVAO_) {
      this->cutVAO_->destroy();
      delete this->cutVAO_;
      this->cutVAO_ = NULL;
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
      this->violVBO_ = NULL;
    }
    this->violVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->violVBO_->create();
    this->violVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->violVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->violVBO_->allocate(this->violData_.data(),
      sizeof(GL_FLOAT) * this->violData_.size());

    if (this->violVAO_) {
      this->violVAO_->destroy();
      delete this->violVAO_;
      this->violVAO_ = NULL;
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
  this->faceData_.clear();
  this->edgeData_.clear();
  for (size_t f = 0; f < m_mesh->faces.size(); f++)
  {
    int t1 = m_mesh->faces[f]->tets[0];
    int t2 = m_mesh->faces[f]->tets[1];


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

    if (num_adj_tets == 2 && m_mesh->tets[t1]->mat_label != m_mesh->tets[t2]->mat_label)
      surface = true;

    int m1 = -1;
    int m2 = -2;
    if (t1 >= 0){
      m1 = m_mesh->tets[t1]->mat_label;
      if (m1 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m1])
        force = true;
    }
    if (t2 >= 0){
      m2 = m_mesh->tets[t2]->mat_label;
      if (m2 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m2])
        force = true;
    }
    if (m1 == m2)
      force = false;
    if (m_bClipping)
    {
      cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
      float d = m_4fvClippingPlane[3];

      // does plane cut through face?
      for (int v = 0; v < 3; v++)
      {
        // is vertex on proper side of the plane?
        cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
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
        if (m_mesh->faces[f]->tets[0] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
        if (m_mesh->faces[f]->tets[1] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

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
      if (m_mesh->faces[f]->tets[0] >= 0){

        tet1 = m_mesh->tets[m_mesh->faces[f]->tets[0]];
        if (m_bColorByQuality){
          float t = tet1->minAngle() / 90.0f;
          color1[0] = (1 - t)*bad_color[0] + t*good_color[0];
          color1[1] = (1 - t)*bad_color[1] + t*good_color[1];
          color1[2] = (1 - t)*bad_color[2] + t*good_color[2];
        } else {
          color1 = color_for_label[(int)tet1->mat_label];
        }
      }
      if (m_mesh->faces[f]->tets[1] >= 0){

        tet2 = m_mesh->tets[m_mesh->faces[f]->tets[1]];
        if (m_bColorByQuality){
          float t = tet2->minAngle() / 90.0f;
          color2[0] = (1 - t)*bad_color[0] + t*good_color[0];
          color2[1] = (1 - t)*bad_color[1] + t*good_color[1];
          color2[2] = (1 - t)*bad_color[2] + t*good_color[2];
        } else {
          color2 = color_for_label[(int)tet2->mat_label];
        }
      }
      if (m_mesh->faces[f]->tets[0] < 0)
        color1 = color2;
      if (m_mesh->faces[f]->tets[1] < 0)
        color2 = color1;

      cleaver::vec3 normal = m_mesh->faces[f]->normal;

      // set vertex positions and colors
      for (int v = 0; v < 3; v++)
      {
        cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
        cleaver::Vertex *vertex2 = m_mesh->verts[m_mesh->faces[f]->verts[(v+1)%3]];

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

        uint32_t r = static_cast<uint32_t>(0.5*(color1[0] + color2[0]) * 255);
        uint32_t g = static_cast<uint32_t>(0.5*(color1[1] + color2[1]) * 255);
        uint32_t b = static_cast<uint32_t>(0.5*(color1[2] + color2[2]) * 255);
        uint32_t a = 255;
        uint32_t col = (r << 24) | (g << 16) | (b << 8) & a;
        float val;
        memcpy(&val, &col, 4);
        faceData_.push_back(val);
      }

    }
  }
  m_colorUpdate = false;

  if (this->faceVBO_) {
    this->faceVBO_->destroy();
    delete this->faceVBO_;
    this->faceVBO_ = NULL;
  }
  this->faceVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  this->faceVBO_->create();
  this->faceVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
  if (!this->faceVBO_->bind()) {
    qWarning() << "Could not bind shader program to context";
  }
  this->faceVBO_->allocate(this->faceData_.data(),
    sizeof(GL_FLOAT) * this->faceData_.size());

  if (this->faceVAO_) {
    this->faceVAO_->destroy();
    delete this->faceVAO_;
    this->faceVAO_ = NULL;
  }
  this->faceVAO_ = new QOpenGLVertexArrayObject(this);
  this->faceVAO_->create();
  this->faceVAO_->bind();
  this->faceProg_.enableAttributeArray(0);
  this->faceProg_.enableAttributeArray(1);
  this->faceProg_.enableAttributeArray(2);
  this->faceProg_.setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(GL_FLOAT) * 7);
  this->faceProg_.setAttributeBuffer(1, GL_FLOAT, sizeof(GL_FLOAT) * 3, 3, sizeof(GL_FLOAT) * 7);
  this->faceProg_.setAttributeBuffer(2, GL_FLOAT, sizeof(GL_FLOAT) * 6, 1, sizeof(GL_FLOAT) * 7);

  this->faceVAO_->release();
  this->faceVBO_->release();
  this->faceProg_.release();

  this->cutData_.clear();
  // Now update Cuts List
  if (m_mesher && m_mesher->interfacesComputed())
  {
    std::map<std::pair<int, int>,
      cleaver::HalfEdge*>::iterator edgesIter = m_mesh->halfEdges.begin();

    // reset evaluation flag, so we can use to avoid duplicates
    while (edgesIter != m_mesh->halfEdges.end())
    {
      cleaver::HalfEdge *edge = (*edgesIter).second;
      edge->evaluated = false;
      edgesIter++;
    }

    // now grab each cut only once
    edgesIter = m_mesh->halfEdges.begin();
    while (edgesIter != m_mesh->halfEdges.end())
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
      this->cutVBO_ = NULL;
    }
    this->cutVBO_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->cutVBO_->create();
    this->cutVBO_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!this->cutVBO_->bind()) {
      qWarning() << "Could not bind shader program to context";
    }
    this->cutVBO_->allocate(this->cutData_.data(),
      sizeof(GL_FLOAT) * this->cutData_.size());

    if (this->cutVAO_) {
      this->cutVAO_->destroy();
      delete this->cutVAO_;
      this->cutVAO_ = NULL;
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


// column major multiplication
void matrixMultiply(float A[16], float B[16], float C[16])
{
  for (int j = 0; j < 4; j++)
  {
    for (int i = 0; i < 4; i++)
    {
      // compute result C[i][j];
      C[i + 4 * j] = 0;

      for (int k = 0; k < 4; k++)
        C[i + 4 * j] += A[i + 4 * k] * B[k + 4 * j];
    }
  }
}

cleaver::vec3 matrixVector(float A[16], const cleaver::vec3 &x, float &depth)
{
  float r[4] = { 0 };
  float xx[4];
  xx[0] = x.x;
  xx[1] = x.y;
  xx[2] = x.z;
  xx[3] = 1;

  for (int i = 0; i < 4; i++){
    for (int k = 0; k < 4; k++)
    {
      r[i] += A[i + 4 * k] * xx[k];
    }
  }

  depth = r[3];

  return cleaver::vec3((r[0] / r[3]), r[1] / r[3], r[2] / r[3]);
}

struct triangle{ float x[3], y[3]; float d[3]; QColor color; };
bool triangle_sort(triangle t1, triangle t2){

  float d1 = std::max(std::max(t1.d[0], t1.d[1]), t1.d[2]);
  float d2 = std::max(std::max(t2.d[0], t2.d[1]), t2.d[2]);

  return d1 > d2;
}

void MeshWindow::dumpSVGImage(const std::string &filename)
{
  int w = this->width();
  int h = this->height();

  GLdouble aspectRatio = (GLdouble)w / (float)h;
  GLdouble zNear = 0.1;
  GLdouble zFar = 500.0;
  GLdouble yFovInDegrees = 45;
  GLdouble top = zNear * tan(yFovInDegrees * M_PI / 360.0);
  GLdouble bottom = -top;
  GLdouble right = top*aspectRatio;
  GLdouble left = -right;

  float projectionMatrix[16];
  GLdouble A = (right + left) / (right - left);
  GLdouble B = (top + bottom) / (top - bottom);
  GLdouble C = -(zFar + zNear) / (zFar - zNear);
  GLdouble D = -(2 * zFar*zNear) / (zFar - zNear);
  projectionMatrix[0] = 2 * zNear / (right - left);
  projectionMatrix[1] = 0;
  projectionMatrix[2] = 0;
  projectionMatrix[3] = 0;
  projectionMatrix[4] = 0;
  projectionMatrix[5] = 2 * zNear / (top - bottom);
  projectionMatrix[6] = 0;
  projectionMatrix[7] = 0;
  projectionMatrix[8] = A;
  projectionMatrix[9] = B;
  projectionMatrix[10] = C;
  projectionMatrix[11] = -1;
  projectionMatrix[12] = 0;
  projectionMatrix[13] = 0;
  projectionMatrix[14] = D;
  projectionMatrix[15] = 0;

  std::cout << "Computed Projection Matrix:" << std::endl;
  printMatrix(projectionMatrix);

  float *viewMatrix = this->cameraMatrix_.data();
  std::cout << "Computed ModelView Matrix:" << std::endl;
  printMatrix(viewMatrix);

  float viewProjectionMatrix[16];
  matrixMultiply(projectionMatrix, viewMatrix, viewProjectionMatrix);

  std::stringstream ss;

  ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
  ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" <<
    w << "px\" height =\"" << h << "px\" viewBox = \"0 0 " << w <<
    " " << h << "\">" << std::endl;

  std::vector<triangle> triangle_list;

  for (size_t f = 0; f < m_mesh->faces.size(); f++)
  {
    int t1 = m_mesh->faces[f]->tets[0];
    int t2 = m_mesh->faces[f]->tets[1];


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

    if (num_adj_tets == 2 && m_mesh->tets[t1]->mat_label != m_mesh->tets[t2]->mat_label)
      surface = true;

    int m1 = -1;
    int m2 = -2;
    if (t1 >= 0){
      m1 = m_mesh->tets[t1]->mat_label;
      if (m_bMaterialFaceLock[m1])
        force = true;
    }
    if (t2 >= 0){
      m2 = m_mesh->tets[t2]->mat_label;
      if (m_bMaterialFaceLock[m2])
        force = true;
    }
    if (m1 == m2)
      force = false;

    if (m_bClipping)
    {
      cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
      float d = m_4fvClippingPlane[3];

      // does plane cut through face?
      for (int v = 0; v < 3; v++)
      {
        // is vertex on proper side of the plane?
        cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
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
        if (m_mesh->faces[f]->tets[0] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

          for (int v = 0; v < 4; v++){
            cleaver::Vertex *vertex = tet->verts[v];
            cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

            if (n.dot(p) - d > 1E-4){
              clipborder = true;
              break;
            }
          }
        }
        if (m_mesh->faces[f]->tets[1] > 0)
        {
          cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

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

    if ((((!clipped && exterior) || 
      clipborder) && !m_bSurfacesOnly) || 
      (m_bSurfacesOnly && !clipped && surface) || force)
    {
      cleaver::Tet *tet1 = m_mesh->tets[m_mesh->faces[f]->tets[0]];
      cleaver::Tet *tet2 = m_mesh->tets[m_mesh->faces[f]->tets[1]];

      float default_color[3] = { 0.9f, 0.9f, 0.9f };
      float *color1 = default_color, *color2 = default_color;
      if (m_mesh->faces[f]->tets[0] >= 0)
        color1 = color_for_label[(int)tet1->mat_label];
      if (m_mesh->faces[f]->tets[1] >= 0)
        color2 = color_for_label[(int)tet2->mat_label];
      if (m_mesh->faces[f]->tets[0] < 0)
        color1 = color2;
      if (m_mesh->faces[f]->tets[1] < 0)
        color2 = color1;

      cleaver::vec3 normal = m_mesh->faces[f]->normal;


      //ss << "<polygon points=\" ";

      //Cleaver::Vertex *v1 = m_mesh->verts[m_mesh->faces[f].verts[0]];
      //Cleaver::Vertex *v2 = m_mesh->verts[m_mesh->faces[f].verts[1]];
      //Cleaver::Vertex *v3 = m_mesh->verts[m_mesh->faces[f].verts[2]];

      /*
      float avg_length = (1.0/3.0)*(length(v1->pos() - v2->pos()) +
      length(v2->pos() - v3->pos()) +
      length(v3->pos() - v1->pos()));
      float stroke_width = avg_length / 18.0f;
      */

      triangle tri;

      // set vertex positions and colors
      for (int v = 0; v < 3; v++)
      {
        cleaver::Vertex *v1 = m_mesh->verts[m_mesh->faces[f]->verts[(v + 0) % 3]];

        float depth1;

        cleaver::vec3 p1 = matrixVector(viewProjectionMatrix, v1->pos(), depth1);

        float x1 = w * (p1.x + 1) / 2;
        float y1 = h - h * (p1.y + 1) / 2;

        tri.x[v] = x1;
        tri.y[v] = y1;
        tri.d[v] = depth1;
      }

      tri.color = QColor((int)(0.5*(color1[0] + color2[0]) * 255), 
        (int)(0.5*(color1[1] + color2[1]) * 255), (int)(0.5*(color1[2] 
        + color2[2]) * 255), 255);

      triangle_list.push_back(tri);
    }
  }

  sort(triangle_list.begin(), triangle_list.end(), triangle_sort);
  for (unsigned int i = 0; i < triangle_list.size(); i++)
  {
    triangle t = triangle_list[i];
    ss << "<polygon points=\" ";

    float avg_length = 0;


    for (int v = 0; v < 3; v++){
      ss << t.x[v] << "," << t.y[v] << " ";
      float dx = t.x[v] - t.x[(v + 1) % 3];
      float dy = t.y[v] - t.y[(v + 1) % 3];
      avg_length += (1.0 / 3.0)*(sqrt(dx*dx + dy*dy));
    }
    //float stroke_width = avg_length / 30.0f;
    float stroke_width = 0.5f;

    ss << "\" style=\"fill:" << t.color.name().toStdString() << 
      ";stroke-linejoin:round;stroke-opacity:0.4;stroke:black;stroke-width:"
      << stroke_width << "\"/>\n" << std::endl;
  }


  // need to sort these triangles

  ss << "</svg>" << std::endl;


  std::string svgString = ss.str();
  //std::cout << svgString << std::endl;
  std::cout << "saving SVG file: " << filename << ".svg" << std::endl;

  std::ofstream file(std::string(filename + ".svg").c_str());

  file << svgString;

  file.close();
}
