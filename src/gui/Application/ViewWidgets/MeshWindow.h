#ifndef MESHWINDOW_H
#define MESHWINDOW_H

// enable vertex buffer includes
#define GL_GLEXT_PROTOTYPES

#if defined(WIN32)
#include <GL/glew.h>
#endif

#include <QGLWidget>
#include <Cleaver/CleaverMesher.h>
#include <Cleaver/TetMesh.h>
#include <Cleaver/Volume.h>
#include "Camera.h"
#include <QMatrix4x4>

enum StarMode { NoStar, VertexStar, EdgeStar, FaceStar };
enum CameraType { Target, Trackball };


class MeshWindow : public QGLWidget
{
    Q_OBJECT
public:
    explicit MeshWindow(QObject *parent = 0);
    ~MeshWindow();

    bool connectToTimer(QTimer *timer);
    void setDefaultOptions();
    void resetView();
    void saveView();
    void loadView();
    void updateMesh();

    //-- mutators --
    void setMesh(cleaver::TetMesh *mesh);
    void setVolume(cleaver::Volume *volume);
    void setAxisVisible(bool value){ m_bShowAxis = value; updateGL();}
    void setBBoxVisible(bool value){ m_bShowBBox = value; }
    void setFacesVisible(bool value){ m_bShowFaces = value; }
    void setEdgesVisible(bool value){ m_bShowEdges = value; }
    void setCutsVisible(bool value){ m_bShowCuts = value; }
    void setSyncedClipping(bool value){ m_bSyncedClipping = value;  }
    void setSurfacesOnly(bool value){ m_bSurfacesOnly = value; }
    void setColorByQuality(bool value){ m_bColorByQuality = value;
        m_colorUpdate = true;}
    void setClippingPlaneVisible(bool value){ m_bShowClippingPlane = value; }
    void setClipping(bool value){ m_bClipping = value; update_vbos(); }
    void setClippingPlane(float plane[4]){ memcpy(m_4fvClippingPlane, plane, 4*sizeof(float)); if(!m_bShowClippingPlane || m_bSyncedClipping) update_vbos();}
    void setCamera(Camera *camera){ m_camera = camera; }
    void setCameraType(CameraType type){ m_cameraType = type; initializeCamera(); }
    void setMaterialFaceLock(int m, bool value){ m_bMaterialFaceLock[m] = value; }
    void setMaterialCellLock(int m, bool value){ m_bMaterialCellLock[m] = value; }


    //void setClippingPlane(float plane[4]){ memcpy(m_4fvClippingPlane, plane, 4*sizeof(float)); update_vbos();}
    //void setClipping(bool value){ m_bClipping = value; update_vbos(); }

    //-- acccessors
    bool axisVisible(){ return m_bShowAxis; }
    bool bboxVisible(){ return m_bShowBBox; }
    bool facesVisible(){ return m_bShowFaces; }
    bool edgesVisible(){ return m_bShowEdges; }
    bool cutsVisible(){ return m_bShowCuts; }

    bool getMaterialFaceLock(int m) const { return m_bMaterialFaceLock[m]; }
    bool getMaterialCellLock(int m) const { return m_bMaterialCellLock[m]; }

    cleaver::CleaverMesher* mesher(){ return m_mesher; }
    cleaver::TetMesh* mesh(){ return m_mesh; }
    cleaver::Volume* volume(){ return m_volume; }
    cleaver::BoundingBox dataBounds(){ return m_dataBounds; }

    
signals:
    
public slots:

private:

    cleaver::CleaverMesher *m_mesher;
    cleaver::TetMesh *m_mesh;
    cleaver::Volume  *m_volume;
    cleaver::BoundingBox m_dataBounds;
    Camera *m_camera;
    Camera *m_Axiscamera;
    CameraType m_cameraType;
    int m_width, m_height;


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

    QMatrix4x4 m_savedViewMatrix;

    GLuint m_cutVBO;
    GLuint m_violVBO;
    GLuint m_meshVBO[3];

    GLuint m_meshVertexCount;
    GLuint m_cutVertexCount;
    GLuint m_violVertexCount;

    void drawOTCell(cleaver::OTCell *node);
    void drawTree();
    void drawFaces();
    void drawEdges();
    void drawCuts();
    void drawClippingPlane();
    void drawBox(const cleaver::BoundingBox &box);

    // draw violation regions around vertices
    void drawViolationPolytopesForVertices();
    void drawViolationPolytopeForVertex(int v);
    //void drawSafetyPolytopes();

    // experimental
    void initializeSSAO();
    void drawFacesWithSSAO();

    //void printModelViewProjection();
    void dumpSVGImage(const std::string &filename);


    // worker functions
    void setup_vbos();
    void update_vbos();
    void build_bkgrnd_vbos();
    void build_output_vbos();

    // star adjacency visualization calls
    void drawVertexStar(int v);
    void drawEdgeStar(int e);
    void drawFaceStar(int f);

    GLenum program;
    GLenum vertex_shader;
    GLenum fragment_shader;    
    bool init;

protected:

    void initializeOptions();
    void initializeCamera();
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
