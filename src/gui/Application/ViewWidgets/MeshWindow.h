#ifndef MESHWINDOW_H
#define MESHWINDOW_H

#include <QGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <cleaver/CleaverMesher.h>
#include <cleaver/TetMesh.h>
#include <cleaver/Volume.h>
#include "Camera.h"
#include <QMatrix4x4>

enum StarMode { NoStar, VertexStar, EdgeStar, FaceStar };

class MeshWindow : public QGLWidget
{
  Q_OBJECT
public:
  explicit MeshWindow(const QGLFormat& format, QObject *parent = 0);
  ~MeshWindow();

  bool connectToTimer(QTimer *timer);
  void setDefaultOptions();
  void resetView();
  void saveView();
  void loadView();
  void updateMesh();
  QVector3D screenToBall(const QVector2D &s);
  double angleBetween(const QVector3D &v1, const QVector3D &v2);

  QSize sizeHint() const;

  //-- mutators --
  void setMesh(cleaver::TetMesh *mesh);
  void setVolume(cleaver::Volume *volume);
  void setMesher(cleaver::CleaverMesher *mesher);
  void setAxisVisible(bool value) { m_bShowAxis = value; }
  void setBBoxVisible(bool value) { m_bShowBBox = value; }
  void setFacesVisible(bool value) { m_bShowFaces = value; }
  void setEdgesVisible(bool value) { m_bShowEdges = value; }
  void setCutsVisible(bool value) { m_bShowCuts = value; }
  void setSyncedClipping(bool value) { m_bSyncedClipping = value; }
  void setSurfacesOnly(bool value) { m_bSurfacesOnly = value; }
  void setColorByQuality(bool value) {
    m_bColorByQuality = value;
    m_colorUpdate = true;
  }
  void setClippingPlaneVisible(bool value) { m_bShowClippingPlane = value; }
  void setClipping(bool value) { m_bClipping = value; update_vbos(); }
  void setClippingPlane(float plane[4]) {
    memcpy(m_4fvClippingPlane, plane, 4 * sizeof(float));
    if (!m_bShowClippingPlane || m_bSyncedClipping)
      update_vbos();
  }

  void setMaterialFaceLock(int m, bool value) { m_bMaterialFaceLock[m] = value; }
  void setMaterialCellLock(int m, bool value) { m_bMaterialCellLock[m] = value; }

  //-- acccessors
  bool axisVisible() { return m_bShowAxis; }
  bool bboxVisible() { return m_bShowBBox; }
  bool facesVisible() { return m_bShowFaces; }
  bool edgesVisible() { return m_bShowEdges; }
  bool cutsVisible() { return m_bShowCuts; }

  bool getMaterialFaceLock(int m) const { return m_bMaterialFaceLock[m]; }
  bool getMaterialCellLock(int m) const { return m_bMaterialCellLock[m]; }

  cleaver::BoundingBox dataBounds() { return m_dataBounds; }


signals:
  void closed(MeshWindow* win);

  public slots:

private:

  cleaver::Volume * volume_;
  cleaver::TetMesh * mesh_;
  cleaver::CleaverMesher * mesher_;
  cleaver::BoundingBox m_dataBounds;
  QMatrix4x4 rotateMatrix_, cameraMatrix_, perspMat_;

  StarMode m_starmode;
  int m_currentVertex;
  int m_currentEdge;
  int m_currentFace;

  int m_prev_x, m_prev_y;

  // render options
  bool m_bShowAxis;
  bool m_bShowBBox;
  bool m_bShowFaces;
  bool m_bShowEdges;
  bool m_bShowCuts;
  bool m_bShowViolationPolytopes;
  bool m_bClipping;
  bool m_bShowClippingPlane;
  bool m_bSyncedClipping;
  bool m_bSurfacesOnly;
  bool m_bColorByQuality;
  bool m_bImportedBeta;
  bool m_colorUpdate;

  bool m_bOpenGLError;

  bool m_bLoadedView;

  std::vector<bool> m_bMaterialFaceLock;
  std::vector<bool> m_bMaterialCellLock;

  float m_shrinkscale;
  float m_4fvBBoxColor[4];
  float m_4fvCutsColor[4];
  float m_4fvClippingPlane[4];

  void drawOTCell(cleaver::OTCell *node);
  void drawTree();
  void drawFaces();
  void drawEdges();
  void drawCuts();
  void drawAxis();
  void drawClippingPlane();
  void drawBox(const cleaver::BoundingBox &box);

  // draw violation regions around vertices
  void drawViolationPolytopesForVertices();
  void drawViolationPolytopeForVertex(int v);


  // worker functions
  void setup_vbos();
  void update_vbos();
  void build_bkgrnd_vbos();
  void build_output_vbos();

  // star adjacency visualization calls
  void drawVertexStar(int v);
  void drawEdgeStar(int e);
  void drawFaceStar(int f);

  QOpenGLShaderProgram faceProg_, edgeProg_, axisProg_;
  QOpenGLBuffer * faceVBO_, *edgeVBO_, *cutVBO_, *violVBO_, bboxVBO_, axisVBO_;
  QOpenGLVertexArrayObject * faceVAO_, *edgeVAO_, *cutVAO_, *violVAO_, axisVAO_, bboxVAO_;
  std::vector<float> faceData_, edgeData_, cutData_, violData_, bboxData_;
  bool init;

protected:

  void initializeOptions();
  void initializeShaders();
  void initializeGL();
  void paintGL();
  void resizeGL(int width, int height);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);
  void wheelEvent(QWheelEvent *event);
  void closeEvent(QCloseEvent *event);
};

#endif // MESHWINDOW_H
