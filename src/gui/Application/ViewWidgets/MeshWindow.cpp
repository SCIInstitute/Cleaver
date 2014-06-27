#include "MeshWindow.h"
#include "TargetCamera.h"
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
float DefaultCutsColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f};
float color_for_label[][4] = {{ 0.0f, 174/255.0f, 239/255.0f, 1.0f},   // label 0
                              { 0.0f, 166/255.0f,  81/255.0f, 1.0f},   // label 1
                              { 1.0f, 242/255.0f, 0.0f, 1.0f},         // label 2
                              { 237/255.0f, 28/255.0f, 36/255.0f, 1.0f},   // label 3
                              { 1.0f, 0.0f, 1.0f, 1.0f},          // label 4
                              { 0.5f, 1.0f, 0.5f, 1.0f},          // label 5
                              { 1.0f, 0.5f, 0.5f, 1.0f}};         // label 6

const char* starModeString[4] = {"No-Star Mode","Vertex-Star Mode", "Edge-Star Mode", "Face-Star Mode"};


extern std::vector<cleaver::vec3> badEdges;

MeshWindow::MeshWindow(QObject *parent) :
  QGLWidget(QGLFormat(QGL::SampleBuffers), qobject_cast<QWidget *>(parent))
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setMouseTracking(true);
    this->setFocusPolicy(Qt::ClickFocus);
    //this->setFocusPolicy(Qt::StrongFocus);

    m_mesh = NULL;
    m_volume = NULL;
    m_mesher = NULL;
    init = false;

    initializeOptions();
    initializeCamera();    
}

MeshWindow::~MeshWindow()
{
    delete m_camera;
    delete m_Axiscamera;
}

void MeshWindow::setDefaultOptions()
{
    initializeOptions();
}

void MeshWindow::resetView()
{
    m_camera->reset();
    m_Axiscamera->reset();
    this->updateGL();
}


void MeshWindow::saveView()
{
   std::cout << "Save View to file" << std::endl;

   //m_camera->writeToFile("filename");
   float *viewmatrix = m_camera->viewMatrix();
   for(int i=0; i < 16; i++){
      m_savedViewMatrix.data()[i] = viewmatrix[i];
      // std::cout << "saving[" << i << "] = " << viewmatrix[i] << std::endl;
   }

   std::ofstream file("camera.dat", std::ios::out | std::ios::binary);
   file.write((char*)m_savedViewMatrix.data(), 16*sizeof(qreal));

   file.close();
}

void MeshWindow::loadView()
{
    std::cout << "Loading View from file" << std::endl;

    qreal matrix[16];
    std::ifstream file("camera.dat", std::ios::in | std::ios::binary);

    file.read((char*)matrix, 16*sizeof(qreal));

//    for(int i=0; i < 16; i++){
//        std::cout << "loading[" << i << "] = " << matrix[i] << std::endl;
//    }
    m_savedViewMatrix = QMatrix4x4(matrix).transposed();

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

    memcpy(m_4fvBBoxColor, DefaultBBoxColor, 4*sizeof(float));
    memcpy(m_4fvCutsColor, DefaultCutsColor, 4*sizeof(float));

    //m_cameraType = Target;
    m_cameraType = Trackball;

    // for adjacency visualization and debugging
    m_starmode = NoStar;
    m_currentVertex = 0;
    m_currentEdge = 0;
    m_currentFace = 0;
    m_shrinkscale = 0.0;
    //m_vertexData = NULL;

}

void MeshWindow::initializeCamera()
{
    if(m_cameraType == Target) {
        m_camera = new TargetCamera();
        m_Axiscamera = new TargetCamera();
    }
    else if(m_cameraType == Trackball) {
        m_camera = new TrackballCamera();
        m_Axiscamera = new TargetCamera();
    }
}

void MeshWindow::initializeShaders()
{
    const char *vertex_shader_source = (const char *)default_vert;
    const char *fragment_shader_source = (const char *)default_frag;


    // Create Shader And Program Objects
    program = glCreateProgram();
    vertex_shader = glCreateShader(GL_VERTEX_SHADER_ARB);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER_ARB);

    // Load Shader Sources
    glShaderSource(vertex_shader, 1, &vertex_shader_source, (const GLint *) &default_vert_len);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, (const GLint *) &default_frag_len);

    // Compile The Shaders
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    // Attach The Shader Objects To The Program Object
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    // Link The Program Object
    glLinkProgram(program);

    GLsizei bufferSize = 255;
    GLsizei length = 0;
    GLchar *infoLogBuffer = new GLchar[bufferSize];
    glGetProgramInfoLog(program, bufferSize, &length, infoLogBuffer);

    std::cout << infoLogBuffer << std::endl;
}

void MeshWindow::initializeGL()
{
    #if defined(WIN32)
    glewInit();
    #endif

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glShadeModel(GL_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    setup_vbos();

    initializeShaders();
    initializeSSAO();

    init = true;
}

void MeshWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_width = w;
    m_height = h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLdouble aspectRatio = (GLdouble) w / (float)h;
    GLdouble zNear = 0.1;
    GLdouble zFar  = 2000.0;
    GLdouble yFovInDegrees = 45;

    GLdouble top    = zNear * tan(yFovInDegrees * M_PI / 360.0);
    GLdouble bottom = -top;
    GLdouble right = top*aspectRatio;
    GLdouble left  = -right;

    if(m_cameraType == Trackball) {
        ((TrackballCamera*)m_camera)->setBallSize(w, h);
        ((TrackballCamera*)m_Axiscamera)->setBallSize(w, h);
    }

    glFrustum(left, right, bottom, top, zNear, zFar);

    //--------------------------------------------
    // Achieves same as calling glFrustrum()
    //--------------------------------------------
    /*
    float projectionMatrix[16];
    GLdouble A = (right + left) / (right - left);
    GLdouble B = (top + bottom) / (top - bottom);
    GLdouble C = -(zFar + zNear)   / (zFar - zNear);
    GLdouble D = -(2*zFar*zNear)   / (zFar - zNear);
    projectionMatrix[0] = 2*zNear/(right - left);
    projectionMatrix[1] = 0;
    projectionMatrix[2] = 0;
    projectionMatrix[3] = 0;
    projectionMatrix[4] = 0;
    projectionMatrix[5] = 2*zNear/(top-bottom);
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
    glMultMatrixf(projectionMatrix);
    */
    //--------------------------------------------

}

void createFrustumMatrix(double *matrix, double left, double right, double bottom, double top, double zNear, double zFar)
{
    double temp, temp2, temp3, temp4;
    temp = 2.0 * zNear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zFar - zNear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zFar - zNear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zFar) / temp4;
    matrix[15] = 0.0;
}

void printMatrix(GLdouble *matrix)
{
    std::cout << matrix[0]  << " " << matrix[1]  << " " << matrix[2]  << " " << matrix[3] << std::endl;
    std::cout << matrix[4]  << " " << matrix[5]  << " " << matrix[6]  << " " << matrix[7] << std::endl;
    std::cout << matrix[8]  << " " << matrix[9]  << " " << matrix[10] << " " << matrix[11] << std::endl;
    std::cout << matrix[12] << " " << matrix[13] << " " << matrix[14] << " " << matrix[15] << std::endl;
}

void printMatrix(float *matrix)
{
    std::cout << matrix[0]  << " " << matrix[1]  << " " << matrix[2]  << " " << matrix[3] << std::endl;
    std::cout << matrix[4]  << " " << matrix[5]  << " " << matrix[6]  << " " << matrix[7] << std::endl;
    std::cout << matrix[8]  << " " << matrix[9]  << " " << matrix[10] << " " << matrix[11] << std::endl;
    std::cout << matrix[12] << " " << matrix[13] << " " << matrix[14] << " " << matrix[15] << std::endl;
}

static inline void qMultMatrix(const QMatrix4x4 &mat)
{
    if (sizeof(qreal) == sizeof(GLfloat))
        glMultMatrixf((GLfloat*)mat.constData());
#ifndef QT_OPENGL_ES
    else if (sizeof(qreal) == sizeof(GLdouble))
        glMultMatrixd((GLdouble*)mat.constData());
#endif
    else
    {
        GLfloat fmat[16];
        qreal const *r = mat.constData();
        for (int i = 0; i < 16; ++i)
            fmat[i] = r[i];
        glMultMatrixf(fmat);
    }
}

void MeshWindow::paintGL()
{
    #ifdef WIN32
    if(!init)
        initializeGL();
    #endif

    makeCurrent();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    if(m_bOpenGLError)
//        return;
    float glmat[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, glmat);
    for(size_t i = 0; i < 16; i++) {
        if(isnan(glmat[i])) {
            std::cout << "Recovering from a NaN matrix error..." << std::endl;
            if (m_volume) {
                m_camera->reset();
            } else {
                m_Axiscamera->reset();
            }
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
    
    glMultMatrixf(m_camera->viewMatrix());
    static float rot = 0.1f;
    rot += 0.5f;

    if(m_cameraType == Trackball)
    {
        if(m_volume){
            glTranslatef(m_volume->bounds().center().x,
                         m_volume->bounds().center().y,
                         m_volume->bounds().center().z);
        }
        //else
        //  glTranslatef(m_mesher->bounds().);


        QMatrix4x4 matrix;
        QMatrix4x4 postmatrix(0.261093, -0.435186, -0.861652, 0,
                                   -0.0523809, 0.884911, -0.462805, 0,
                                    0.963891,  0.165969,  0.208249, 0,
                                    0,         0,         0,  1);

        matrix.rotate(((TrackballCamera*)m_camera)->rot());
        qMultMatrix(matrix);
        qMultMatrix(postmatrix.transposed());

        if(m_volume){
            glTranslatef(-m_volume->bounds().center().x,
                         -m_volume->bounds().center().y,
                         -m_volume->bounds().center().z);
        }

    }

    if(m_bLoadedView){
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMultMatrixd(m_savedViewMatrix.data());
    }

    /*
    GLfloat projectionMatrix[16];
    GLfloat modelViewMatrix[16];
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)projectionMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix);

    std::cout << "Actual Projection Matrix: " << std::endl;
    printMatrix(projectionMatrix);

    std::cout << "Actual ModelView Matrix: " << std::endl;
    printMatrix(modelViewMatrix);
    */

    if(m_mesh)
    {
        if(m_bShowFaces)
            drawFaces();

        if(m_bShowCuts)
            drawCuts();    
    }

    if(m_volume)
    {
        if(m_bShowBBox){
            glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
            
            glDisable(GL_LIGHTING);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(2.0);

            drawBox(m_volume->bounds());

            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }
    }

    if(m_mesh){
        if(m_bShowEdges)
            drawEdges();
        if(m_bShowCuts)
            drawCuts();

        glLineWidth(3.0f);
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);
        for(size_t i=0; i < badEdges.size()/2; i+=2)
        {
            glVertex3f(badEdges[i].x, badEdges[i].y, badEdges[i].z);
            glVertex3f(badEdges[i+1].x, badEdges[i+1].y, badEdges[i+1].z);
        }
        glEnd();
    }

    switch(m_starmode)
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


    if(m_bShowViolationPolytopes)
        drawViolationPolytopesForVertices();

    if(m_bShowCuts)
        drawCuts();

    if(m_bClipping && m_bShowClippingPlane)
        drawClippingPlane();

    // done MeshWindow::paintGL()
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

    /*
    glBegin(GL_LINES);
    for(int t=0; t < tetlist.size(); t++)
    {
        for(int i=0; i < 4; i++){
            for(int j=i+1; j < 4; j++){
                Cleaver::vec3 v1 = tetlist[t]->verts[i]->pos();
                Cleaver::vec3 v2 = tetlist[t]->verts[j]->pos();

                if(tetlist[t]->verts[i] == vertex)
                    glColor3f(0.5f, 0.8f, 0.5f);
                else
                    glColor3f(0.0f, 0.0f, 0.0f);

                glVertex3f(v1.x, v1.y, v1.z);

                if(tetlist[t]->verts[j] == vertex)
                    glColor3f(0.5f, 0.8f, 0.5f);
                else
                    glColor3f(0.0f, 0.0f, 0.0f);

                glVertex3f(v2.x, v2.y, v2.z);
            }
        }

    }
    glEnd();
    */


    //--- Draw Faces Around Vertex ---
    std::vector<cleaver::HalfFace*> facelist = m_mesh->facesAroundVertex(vertex);

    glColor3f(0.7f,0.7f,0.7f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glBegin(GL_TRIANGLES);
    for(unsigned int f=0; f < facelist.size(); f++)
    {

        for(int v=0; v < 3; v++){
            cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
            glColor3f(v/2.0f,1.0f - (v/2.0f), 1.0f);
            glVertex3f(p.x, p.y, p.z);
        }
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);


    glColor3f(0.0f,0.0f,0.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBegin(GL_TRIANGLES);
    for(unsigned int f=0; f < facelist.size(); f++)
    {

        for(int v=0; v < 3; v++){
            cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
            glVertex3f(p.x, p.y, p.z);
        }
    }
    glEnd();


    // check for uniqueness
    /*
    for(int f=0; f < facelist.size(); f++)
    {
        Cleaver::Vertex *v1 = facelist[f]->halfEdges[0]->vertex;
        Cleaver::Vertex *v2 = facelist[f]->halfEdges[1]->vertex;
        Cleaver::Vertex *v3 = facelist[f]->halfEdges[2]->vertex;

        std::cout << "face[" << f << "] ( " << facelist[f] << " ) = "
                  << v1->tm_v_index << ", "
                  << v2->tm_v_index << ", "
                  << v3->tm_v_index << std::endl;
    }
    */


    //--- Draw Edges Around Vertex ----
    /*
    std::vector<Cleaver::HalfEdge*> edgelist = m_mesh->edgesAroundVertex(vertex);
    Cleaver::vec3 v1 = vertex->pos();

    glBegin(GL_LINES);
    for(int e=0; e < edgelist.size(); e++)
    {
        Cleaver::vec3 v2 = edgelist[e]->vertex->pos();

        glColor3f(0.0, 0.5f, 0.0f);
        glVertex3f(v1.x, v1.y, v1.z);
        glColor3f(0.0, 0.0f, 0.0f);
        glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
    */

}

void MeshWindow::drawEdgeStar(int edge)
{
    // get pointer to the current edge
    int idx = 0;
    std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator iter = m_mesh->halfEdges.begin();
    while(idx != edge){
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

    glColor3f(0.7f,0.7f,0.7f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glBegin(GL_TRIANGLES);
    for(unsigned int f=0; f < facelist.size(); f++)
    {

        for(int v=0; v < 3; v++){
            cleaver::vec3 p = facelist[f]->halfEdges[v]->vertex->pos();
            glColor3f(v/2.0f,1.0f - (v/2.0f), 1.0f);
            glVertex3f(p.x, p.y, p.z);
        }
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);


    glColor3f(0.0f,0.0f,0.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBegin(GL_TRIANGLES);
    for(unsigned int f=0; f < facelist.size(); f++)
    {

        for(int v=0; v < 3; v++){
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
    for(unsigned int t=0; t < tetlist.size(); t++){
        for(int f=0; f < 4; f++){
            cleaver::vec3 p1 = tetlist[t]->verts[(f+0)%4]->pos();
            cleaver::vec3 p2 = tetlist[t]->verts[(f+1)%4]->pos();
            cleaver::vec3 p3 = tetlist[t]->verts[(f+2)%4]->pos();

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
    glColor3f(0.0f,0.0f,0.0f);
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
    for(unsigned int t=0; t < tetlist.size(); t++)
    {
        for(int i=0; i < 4; i++){
            for(int j=i+1; j < 4; j++){
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
    if(m_mesher->alphasComputed())
    {
        for(size_t v=0; v < m_mesh->verts.size(); v++)
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
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE );

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
    std::vector<cleaver::Tet*>      adjTets  = m_mesh->tetsAroundVertex(vertex);

    glDisable(GL_LIGHTING);
    glBegin(GL_TRIANGLES);
    // phase 1 - simply connect edge violations
    for(size_t t=0; t < adjTets.size(); t++)
    {
        // each tet t has 3 edges incident to vertex v
        int count = 0;
        std::vector<cleaver::HalfEdge*> edges = m_mesh->edgesAroundTet(adjTets[t]);
        for(int e=0; e < 6; e++){
            cleaver::HalfEdge *edge = edges[e];
            if(edge->incidentToVertex(vertex))
            {
                count++;
                float t = edge->alphaForVertex(vertex);
                if(edge->vertex == vertex){
                    cleaver::vec3 v2 = edge->vertex->pos();
                    cleaver::vec3 v1 = edge->mate->vertex->pos();
                    cleaver::vec3 pos = (t)*v1 + (1-t)*v2;
                    glVertex3f(pos.x, pos.y, pos.z);
                }
                else{
                    cleaver::vec3 v1 = edge->vertex->pos();
                    cleaver::vec3 v2 = edge->mate->vertex->pos();
                    cleaver::vec3 pos = (t)*v1 + (1-t)*v2;
                    glVertex3f(pos.x, pos.y, pos.z);
                }
            }
        }
        if(count != 3){
            std::cout << "PROBLEM!" << std::endl;
            exit(9);
        }
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void MeshWindow::drawFaces()
{
    glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LIGHTING);

    // bind VBOs for vertex array and color array
    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[VERTEX_OBJECT]);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[COLOR_OBJECT]);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[NORMAL_OBJECT]);
    glNormalPointer(GL_FLOAT, 0, 0);

    glEnableClientState(GL_VERTEX_ARRAY);      // activate normal array
    glEnableClientState(GL_COLOR_ARRAY);       // activate vertex array
    glEnableClientState(GL_NORMAL_ARRAY);      // activate color  array

    // shader pogram
    glUseProgram(program);

    // glVertexAttrib
    //glEnableVertexAttribArray(4);
    //glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, m_vertexData);

    // Draw The Triangles
    glDrawArrays(GL_TRIANGLES, 0, m_meshVertexCount);

    glUseProgram(0);

    glDisableClientState(GL_NORMAL_ARRAY);      // deactivate normal array
    glDisableClientState(GL_COLOR_ARRAY);       // deactivate vertex array
    glDisableClientState(GL_VERTEX_ARRAY);      // deactivate color  array

    // bind with 0, so, switch back to normal pointer operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_LIGHTING);
}

void MeshWindow::initializeSSAO()
{
//    // Create a framebuffer object
//    GLuint myFBO = 0;
//    glGenFramebuffers(1,  &myFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, myFBO);


//    // Create Texture which will contain the image
//    GLuint renderTexture;
//    glGenTextures(1, &renderTexture);
//    glBindTexture(GL_TEXTURE_2D, renderTexture);

//    // Give an empty image to OpenGL
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);



//    // We need depth testing, so create and attach a depth renderbuffer
//    GLuint myRB;
//    glGenRenderbuffers(1, &myRB);
//    glBindRenderbuffer(GL_RENDERBUFFER, myRB);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, myRB);

//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);


//    // Swithch back to normal framebuffer rendering
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MeshWindow::drawFacesWithSSAO()
{
    GLuint framebufferID = 0;
    GLuint colorTextureID = 0;


    // (1) render geometry with Normal+Depth shader into a Texture   (FBO render pass)

    //----------------------------------------------------------------------------------------------------------
    glViewport(0, 0, 512, 512);                                    // set The Current Viewport to the fbo size
    glBindTexture(GL_TEXTURE_2D, 0);                                // unlink textures because if we dont it all is gonna fail
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebufferID);        // switch to rendering on our FBO
    //----------------------------------------------------------------------------------------------------------

    glClearColor(1.0f, 0.0f, 0.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);            // Clear Screen And Depth Buffer on the fbo to red
    glLoadIdentity();                                              // Reset The Modelview Matrix

    /*
    gluLookAt(m_camera->e().x, m_camera->e().y, m_camera->e().z,
              m_camera->t().x, m_camera->t().y, m_camera->t().z,
              m_camera->u().x, m_camera->u().y, m_camera->u().z);
    */

    drawFaces();

    //----------------------------------------------------------------------------------------------------------
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);                    // switch to rendering on the framebuffer
    //----------------------------------------------------------------------------------------------------------


    // (2) render screen aligned quad with SSAO shader

    glEnable(GL_TEXTURE_2D);                                        // enable texturing
    glClearColor (1.0f, 1.0f, 1.0f, 0.0f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);            // Clear Screen And Depth Buffer on the framebuffer

    glBindTexture(GL_TEXTURE_2D, colorTextureID);                   // bind our FBO texture
    glViewport (0, 0, 512, 512);                                    // set The Current Viewport

    glLoadIdentity ();                                              // Reset The Modelview Matrix

    // draw screen aligned quad

    glDisable(GL_TEXTURE_2D);
}

void MeshWindow::drawEdges()
{
    glColor4f(0.0f, 0.0f, 0.0f, 0.2f);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5);

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    // bind VBOs for vertex array and color array
    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[VERTEX_OBJECT]);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glEnableClientState(GL_VERTEX_ARRAY);

    /*
    if(m_mesher->samplingDone() && !m_bShowFaces){
        glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[COLOR_OBJECT]);
        glColorPointer(3, GL_INT, 0, 0);
        glEnableClientState(GL_COLOR_ARRAY);
    }
    */

    // Draw The Triangles
    glDrawArrays(GL_TRIANGLES, 0, m_meshVertexCount);

    /*
    if(m_mesher->samplingDone() && !m_bShowFaces){
        glDisableClientState(GL_COLOR_ARRAY);       // deactivate color array
    }
    */
    glDisableClientState(GL_VERTEX_ARRAY);      // deactivate vertex array


    // bind with 0, so, switch back to normal pointer operation
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);


    glEnable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

void MeshWindow::drawCuts()
{
    glDisable(GL_LIGHTING);
    glColor4fv(m_4fvCutsColor);
    glPointSize(4.0);

    // bind VBOs for vertex array and color array
    glBindBuffer(GL_ARRAY_BUFFER, m_cutVBO);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    // Draw The Points
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_POINTS, 0, m_cutVertexCount);
    glDisableClientState(GL_VERTEX_ARRAY);

    // bind with 0, so, switch back to normal pointer operation
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);


    // now draw violating vectors
    glBindBuffer(GL_ARRAY_BUFFER, m_violVBO);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    // Draw The Lines
    glColor4f(1.0f, 0.5f, 0.5f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_LINES, 0, 2*m_violVertexCount);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);


}

void MeshWindow::drawClippingPlane()
{
    cleaver::vec3 u,v;
    cleaver::vec3 n = cleaver::vec3(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);

    u = n.cross(cleaver::vec3::unitX);
    double dotval = n.dot(cleaver::vec3::unitX);

    if(fabs(n.dot(cleaver::vec3::unitY)) <= dotval){
        u = n.cross(cleaver::vec3::unitY);
        dotval = fabs(n.dot(cleaver::vec3::unitY));
    }
    if(fabs(n.dot(cleaver::vec3::unitZ)) <= dotval)
        u = n.cross(cleaver::vec3::unitZ);

    v = n.cross(u);

    // point P0 center of plane should be center of bounding box
    int w = 0.6f*m_dataBounds.size.x;
    int h = 0.6f*m_dataBounds.size.y;

    cleaver::vec3 shift = cleaver::vec3(m_dataBounds.size.x*n.x,
                                        m_dataBounds.size.y*n.y,
                                        m_dataBounds.size.z*n.z);

    cleaver::vec3 p0 = m_dataBounds.center() - 0.5f*shift + m_4fvClippingPlane[3]*n;
    cleaver::vec3 p1 = p0 + ( 1.0)*w*u + ( 1.0)*h*v;
    cleaver::vec3 p2 = p0 + (-1.0)*w*u + ( 1.0)*h*v;
    cleaver::vec3 p3 = p0 + (-1.0)*w*u + (-1.0)*h*v;
    cleaver::vec3 p4 = p0 + ( 1.0)*w*u + (-1.0)*h*v;

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
    float x = box.origin.x;
    float y = box.origin.y;
    float z = box.origin.z;
    float w = box.size.x;
    float h = box.size.y;
    float d = box.size.z;

    glPushMatrix();
    glTranslatef(x,y,z);

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
    float tmp2[16];
    glPushMatrix();
    
    glTranslatef(-4.2*ratio, +4.0, -15);
    if (!m_volume) {
    float * tmp = m_Axiscamera->viewMatrix();
        for(int i = 0; i < 16; i++)
            if (i < 12 || i == 15)
                tmp2[i] = tmp[i];
            else
                tmp2[i] = 0.f;
        glMultMatrixf(tmp2);
    } else {
        QMatrix4x4 mat;
        QQuaternion q =((TrackballCamera*)m_camera)->rot();
        q.setX(-q.x());
        q.setZ(-q.z());
        mat.rotate(q);
        mat.rotate(20,QVector3D(1,0,0));
        mat.rotate(-20,QVector3D(0,1,0));
        glMultMatrixd(mat.constData());
    }
    
    
    
    glBegin(GL_LINES);
    
    glColor4f(1.f, 0.f, 0.f,1.f);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    
    glVertex3f(1.2, 0.2, 0);
    glVertex3f(1.4, -0.2, 0);
    
    glVertex3f(1.4, 0.2, 0);
    glVertex3f(1.2, -0.2, 0);
    
    glColor4f(0.f, 1.f, 0.f,1.f);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);
    
    glVertex3f(0, 1.2, 0);
    glVertex3f(0, 1.4, 0);
    
    glVertex3f(0, 1.4, 0);
    glVertex3f(0.1, 1.6, 0);
    
    glVertex3f(0, 1.4, 0);
    glVertex3f(-0.1, 1.6, 0);
    
    glColor4f(0.f, 0.f, 1.f,1.f);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    
    glVertex3f(0, 0.2, 1.4);
    glVertex3f(0, 0.2, 1.2);
    
    glVertex3f(0, 0.2, 1.2);
    glVertex3f(0, -0.2, 1.4);
    
    glVertex3f(0, -0.2, 1.4);
    glVertex3f(0, -0.2, 1.2);
    
    glEnd();
    glPopMatrix();
}

void MeshWindow::drawOTCell(cleaver::OTCell *cell)
{
    if(cell){
        float *color = color_for_label[cell->level%8];
        glColor3fv(color);
        drawBox(cell->bounds);
        for(int i=0; i < 8; i++)
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

void MeshWindow::mouseMoveEvent(QMouseEvent *event)
{
    Qt::MouseButtons buttonstate = event->buttons();

    if(buttonstate == Qt::RightButton){
        m_camera->pan((m_prev_x - event->x()), (event->y() - m_prev_y));
    }
    else if(buttonstate == Qt::LeftButton){
        if(m_cameraType == Target) {
            m_camera->rotate((m_prev_x - event->x()), (event->y() - m_prev_y));
            m_Axiscamera->rotate((m_prev_x - event->x()), (event->y() - m_prev_y));
        }
        else if(m_cameraType == Trackball) {
            ((TrackballCamera*)m_camera)->
            rotateBetween(QVector2D(m_prev_x, m_prev_y), QVector2D(event->x(), event->y()));
            ((TrackballCamera*)m_Axiscamera)->
            rotateBetween(QVector2D(m_prev_x, m_prev_y), QVector2D(event->x(), event->y()));
        }
    }

    m_prev_x = event->x();
    m_prev_y = event->y();

    if(buttonstate == Qt::RightButton)
    {
        float c_x = this->width() / 2;
        float c_y = this->height() / 2;

        QPoint loc = event->pos();
        float p_x = loc.x() - c_x;       // x pixel coordinate matching eye x coordinate
        float p_y = -(loc.y() - c_y);    // y pixel coordinate matching eye y coordinate

        if(m_volume){
            float aspect = m_volume->height()/(float)this->height();
            float t_x = p_x * aspect * m_camera->s().x + m_camera->e().x;
            float t_y = p_y * aspect * m_camera->s().y + m_camera->e().y;

            t_x += m_volume->width()/2.0 - 0.5;
            t_y += m_volume->height()/2.0 - 0.5;

            loc.setX(t_x);
            loc.setY(t_y);
        }
    }

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
    switch(event->key())
    {
        case Qt::Key_Space:
            std::cout << "reseting camera" << std::endl;
            resetView();
            break;
        case Qt::Key_Control:
            ctrl_down = true;
            break;
        case Qt::Key_M:
            if(m_starmode == FaceStar)
                m_starmode = NoStar;
            else
                m_starmode = (StarMode)(m_starmode + 1);
            std::cout << starModeString[m_starmode] << std::endl;
            this->updateGL();
            break;
        case Qt::Key_J:
            m_currentVertex--;
            if(m_currentVertex < 0)
                m_currentVertex = m_mesh->verts.size()-1;
            std::cout << "vertex index: " << m_currentVertex << std::endl;

            m_currentEdge--;
            if(m_currentEdge < 0)
                m_currentEdge = m_mesh->halfEdges.size() - 1;
            std::cout << "edge index: " << m_currentEdge << std::endl;

            m_currentFace--;
            if(m_currentFace < 0)
                m_currentFace = (4*m_mesh->tets.size() - 1);
            std::cout << "face index: " << m_currentFace << std::endl;

            this->updateGL();
            break;
        case Qt::Key_K:
            m_currentVertex++;
            if(m_currentVertex >= (int)m_mesh->verts.size())
                m_currentVertex = 0;
            std::cout << "vertex index: " << m_currentVertex << std::endl;

            m_currentEdge++;
            if(m_currentEdge >= (int)m_mesh->halfEdges.size())
                m_currentEdge = 0;
            std::cout << "edge index: " << m_currentEdge << std::endl;

            m_currentFace++;
            if(m_currentFace >= (int)(4*m_mesh->tets.size()))
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
    switch(event->key())
    {
    case Qt::Key_Control:
         ctrl_down = false;
         break;
    }
}

void MeshWindow::wheelEvent(QWheelEvent *event)
{
    if(ctrl_down){
        m_shrinkscale -= 0.0001f*event->delta();
        m_shrinkscale = std::max(m_shrinkscale, 0.0f);
        m_shrinkscale = std::min(m_shrinkscale, 1.0f);
        //std::cout << "shrinkscale = " << m_shrinkscale << std::endl;
        update_vbos();
    }else{
        m_camera->zoom(event->delta());
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
    
    if(mesh->imported) {

        m_bMaterialFaceLock.clear();
        m_bMaterialCellLock.clear();

        for(int m = 0; m < mesh->material_count; m++)
        {
            m_bMaterialFaceLock.push_back(false);
            m_bMaterialCellLock.push_back(false);
        }
    }
    
//    std::cout << "NUM MATERIALS IN MESH: " << m_bMaterialFaceLock.size() << std::endl;
   // MainWindow::instance()->focus((QMdiSubWindow*)this->parentWidget());
    if(m_cameraType == Target && m_volume == NULL){
        ((TargetCamera*)m_camera)->setTargetBounds(mesh->bounds);
        ((TargetCamera*)m_Axiscamera)->setTargetBounds(mesh->bounds);
    }

    m_camera->reset();
    m_Axiscamera->reset();

    if(init){
        update_vbos();
        updateGL();
    }
}

void MeshWindow::setVolume(cleaver::Volume *volume)
{
    m_volume = volume;
    m_dataBounds = cleaver::BoundingBox::merge(m_dataBounds, volume->bounds());

    if(!m_mesher)
        m_mesher = new cleaver::CleaverMesher(volume);
    else{
        m_mesher->setVolume(volume);
    }

    m_bMaterialFaceLock.clear();
    m_bMaterialCellLock.clear();
    for(int m=0; m < volume->numberOfMaterials(); m++){
        m_bMaterialFaceLock.push_back(false);
        m_bMaterialCellLock.push_back(false);
    }
//    std::cout << "NUM MATERIALS IN VOLUME: " << m_bMaterialFaceLock.size() << std::endl;

    if(m_cameraType == Target) {
        ((TargetCamera*)m_camera)->setTargetBounds(m_volume->bounds());
        ((TargetCamera*)m_Axiscamera)->setTargetBounds(m_volume->bounds());
    }
    else if(m_cameraType == Trackball) {
        ((TrackballCamera*)m_camera)->setTargetBounds(m_volume->bounds());
        ((TrackballCamera*)m_Axiscamera)->setTargetBounds(m_volume->bounds());
    }
    m_camera->reset();
    m_Axiscamera->reset();
    updateGL();
}

void MeshWindow::setup_vbos()
{
    // generate a new VBO and get the associated ID
    m_meshVertexCount = 0;
    m_cutVertexCount = 0;
    m_meshVBO[0] = m_meshVBO[1] = m_meshVBO[2] = 0;
    glGenBuffers(3, m_meshVBO);
    glGenBuffers(1, &m_cutVBO);
    glGenBuffers(1, &m_violVBO);

    if(glGetError() != GL_NO_ERROR) {
        std::cerr << "Failed to set up OpenGL VBOS: " << glGetError() <<
            ". Attempting OpenGL Rendering..." << std::endl;
        m_bOpenGLError = true;
    } else m_bOpenGLError = false;

    update_vbos();
}

void MeshWindow::update_vbos()
{
    // 1. Generate a new buffer object with glGenBuffersARB().
    // 2. Bind the buffer object with glBindBufferARB().
    // 3. Copy vertex data to the buffer object with glBufferDataARB().

    // if no mesh, set empty buffer
    if(!m_mesh)
    {
        m_meshVertexCount = 0;
        m_cutVertexCount = 0;
        std::vector<GLfloat> PositionData;
        std::vector<GLubyte> ColorData;
        std::vector<GLfloat> NormalData;

        m_meshVertexCount = 0;
        GLsizeiptr PositionSize = 0;
        GLsizeiptr NormalSize   = 0;
        GLsizeiptr ColorSize    = 0;


        // upload data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[VERTEX_OBJECT]);
        glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[NORMAL_OBJECT]);
        glBufferData(GL_ARRAY_BUFFER, NormalSize, NormalData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[COLOR_OBJECT]);
        glBufferData(GL_ARRAY_BUFFER, ColorSize, ColorData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);


        return;
    }
    else{
        if((m_mesher && mesher()->stencilsDone()) || m_mesh->imported)
            build_output_vbos();
        else
            build_bkgrnd_vbos();
    }

    // now update data to send to shader
    /*
    if(m_vertexData)
        delete []m_vertexData;
    m_vertexData = new float[m_meshVertexCount];
    for(int i=0; i < m_meshVertexCount; i++)
        m_vertexData[m_meshVertexCount] = (float)i;
    */    

}

void MeshWindow::updateMesh()
{
    update_vbos();
}

cleaver::vec3 computeIncenter(const cleaver::vec3 &v1, const cleaver::vec3 &v2, const cleaver::vec3 &v3)
{
    cleaver::vec3 result = cleaver::vec3::zero;

    float a = length(v2 - v3);
    float b = length(v3 - v1);
    float c = length(v1 - v2);
    float perimeter = a+b+c;

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
    float total_area = a+b+c+d;

    result.x = (a*v1.x + b*v2.x + c*v3.x + d*v4.x) / total_area;
    result.y = (a*v1.y + b*v2.y + c*v3.y + d*v4.y) / total_area;
    result.z = (a*v1.z + b*v2.z + c*v3.z + d*v4.z) / total_area;

    return result;
}

void MeshWindow::build_bkgrnd_vbos()
{
    m_meshVertexCount = 0;
    m_cutVertexCount = 0;
    std::vector<GLfloat> PositionData;
    std::vector<GLfloat> NormalData;
    std::vector<GLubyte> ColorData;


    for(size_t f=0; f < m_mesh->faces.size(); f++)
    {
        int t1 = m_mesh->faces[f]->tets[0];
        int t2 = m_mesh->faces[f]->tets[1];


        bool clipped = false;
        bool exterior = false;
        bool clipborder = false;
        int num_adj_tets = 0;
        num_adj_tets += t1 >= 0 ? 1 : 0;
        num_adj_tets += t2 >= 0 ? 1 : 0;

        if(num_adj_tets == 1)
            exterior = true;

        if(m_bClipping)
        {
            cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
            float d = m_4fvClippingPlane[3];

            // does plane cut through face?
            for(int v=0; v < 3; v++)
            {
                // is vertex on proper side of the plane?
                cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
                cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                if(n.dot(p) - d > 1E-4){
                    clipped = true;
                    break;
                }
            }

            // determine if face is on border of nonclipped faces
            if(!clipped)
            {
                // look at both adjacent tets
                if(m_mesh->faces[f]->tets[0] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
                            clipborder = true;
                            break;
                        }
                    }
                }
                if(m_mesh->faces[f]->tets[1] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
                            clipborder = true;
                            break;
                        }
                    }
                }
            }
        }

        cleaver::vec3 normal = m_mesh->faces[f]->normal;

        if((!clipped && exterior) || clipborder)        
        {
            for(int v=0; v < 3; v++)
            {
                cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];

                PositionData.push_back(vertex->pos().x);
                PositionData.push_back(vertex->pos().y);
                PositionData.push_back(vertex->pos().z);

                NormalData.push_back(normal.x);
                NormalData.push_back(normal.y);
                NormalData.push_back(normal.z);

                if(m_mesher && m_mesher->samplingDone())
                {
                    float *color = color_for_label[vertex->label];
                    ColorData.push_back((int)(color[0]*255));
                    ColorData.push_back((int)(color[1]*255));
                    ColorData.push_back((int)(color[2]*255));
                    ColorData.push_back((int)(1.0f*255));
                }
                else{
                    int color_label = -1;
                    if(m_mesh->faces[f]->tets[0] >= 0)
                        color_label = m_mesh->tets[m_mesh->faces[f]->tets[0]]->mat_label;
                    else if(m_mesh->faces[f]->tets[1] >= 0)
                        color_label = m_mesh->tets[m_mesh->faces[f]->tets[1]]->mat_label;

                    if(color_label < 0){
                        ColorData.push_back((int)(0.9f*255));
                        ColorData.push_back((int)(0.9f*255));
                        ColorData.push_back((int)(0.9f*255));
                        ColorData.push_back((int)(1.0f*255));
                    }
                    else{
                        float *color = color_for_label[color_label%4];
                        ColorData.push_back((int)(color[0]*255));
                        ColorData.push_back((int)(color[1]*255));
                        ColorData.push_back((int)(color[2]*255));
                        ColorData.push_back((int)(1.0f*255));
                    }
                }
            }

        }
    }


    m_meshVertexCount = PositionData.size() / 3;
    GLsizeiptr PositionSize = PositionData.size() * sizeof(GLfloat);
    GLsizeiptr ColorSize    =    ColorData.size() * sizeof(GLubyte);
    GLsizeiptr NormalSize   =   NormalData.size() * sizeof(GLfloat);

    // upload data to VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[VERTEX_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[NORMAL_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, NormalSize, NormalData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[COLOR_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, ColorSize, ColorData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

//    std::cout << "vbo vertex count: " << m_meshVertexCount << std::endl;
//    std::cout << "GL_ARRAY_BUFFER Sizes: " << std::endl;
//    std::cout << "\tPositionSize = " << PositionSize << std::endl;
//    std::cout << "\tNormalSize   = " << NormalSize << std::endl;
//    std::cout << "\tColorSize    = " << ColorSize << std::endl;


    GLenum errorCode = glGetError();
    if(errorCode != GL_NO_ERROR)
    {
        std::cerr << "Failed to upload background mesh data to VBOS. " <<
            "Attempting OpenGL Rendering..." << std::endl;
        switch(errorCode)
        {
            case GL_OUT_OF_MEMORY:
                std::cerr << "\tOut of memory." << std::endl;
                break;
            case GL_INVALID_OPERATION:
                std::cerr << "\tInvalid Operation." << std::endl;
                break;
            case GL_INVALID_ENUM:
                std::cerr << "\tInvalid Enum." << std::endl;
                break;
            default:
                std::cerr << "\tUnspecified Error: " << errorCode << std::endl;
                break;
        }
        m_bOpenGLError = true;
    } else m_bOpenGLError = false;


    // Now update Cuts List
    if(m_mesher && m_mesher->interfacesComputed())
    {
        std::vector<GLfloat> ViolationData;
        PositionData.clear();
        std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator edgesIter = m_mesh->halfEdges.begin();

        // reset evaluation flag, so we can use to avoid duplicates
        while(edgesIter != m_mesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*edgesIter).second;
            edge->evaluated = false;
            edgesIter++;
        }

        // now grab each cut only once
        edgesIter = m_mesh->halfEdges.begin();
        while(edgesIter != m_mesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*edgesIter).second;
            if(edge->cut && edge->cut->order() == 1 && !edge->evaluated)
            {
                PositionData.push_back(edge->cut->pos().x);
                PositionData.push_back(edge->cut->pos().y);
                PositionData.push_back(edge->cut->pos().z);

                if(edge->cut->violating){
                    ViolationData.push_back(edge->cut->pos().x);
                    ViolationData.push_back(edge->cut->pos().y);
                    ViolationData.push_back(edge->cut->pos().z);

                    ViolationData.push_back(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().x);
                    ViolationData.push_back(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().y);
                    ViolationData.push_back(((cleaver::Vertex*)edge->cut->closestGeometry)->pos().z);
                }

                edge->evaluated = true;
                edge->mate->evaluated = true;
            }

            edgesIter++;
        }




        m_cutVertexCount = PositionData.size() / 3;
        PositionSize = PositionData.size() * sizeof(GLfloat);

        // upload data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_cutVBO);
        glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);

        m_violVertexCount = ViolationData.size() / 6;
        GLsizeiptr ViolationSize = ViolationData.size() * sizeof(GLfloat);

        glBindBuffer(GL_ARRAY_BUFFER, m_violVBO);
        glBufferData(GL_ARRAY_BUFFER, ViolationSize, ViolationData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
    }
}

void MeshWindow::build_output_vbos()
{
    m_meshVertexCount = 0;
    m_cutVertexCount = 0;
    std::vector<GLfloat> PositionData;
    std::vector<GLfloat> NormalData;
    std::vector<GLubyte> ColorData;    
//    for(size_t i = 0; i < m_bMaterialFaceLock.size(); i++)
//        std::cout << "Material " << i << ": " << m_bMaterialFaceLock[i] << std::endl;
//    std::cout << "Num Materials: " << m_bMaterialFaceLock.size() << std::endl;
    for(size_t f=0; f < m_mesh->faces.size(); f++)
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

        if(num_adj_tets == 1)
            exterior = true;

        if(num_adj_tets == 2 && m_mesh->tets[t1]->mat_label != m_mesh->tets[t2]->mat_label)
            surface = true;

        int m1 = -1;
        int m2 = -2;
        if(t1 >= 0){
            m1 = m_mesh->tets[t1]->mat_label;
            if(m1 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m1])
                force = true;
        }
        if(t2 >= 0){
            m2 = m_mesh->tets[t2]->mat_label;
            if(m2 < (int)m_bMaterialFaceLock.size() && m_bMaterialFaceLock[m2])
                force = true;
        }
        if(m1 == m2)
            force = false;

        if(m_bClipping)
        {
            cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
            float d = m_4fvClippingPlane[3];

            // does plane cut through face?
            for(int v=0; v < 3; v++)
            {
                // is vertex on proper side of the plane?
                cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
                cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                if(n.dot(p) - d > 1E-4){
                    clipped = true;
                    break;
                }
            }

            // determine if face is on border of nonclipped faces
            if(!clipped)
            {
                // look at both adjacent tets
                if(m_mesh->faces[f]->tets[0] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
                            clipborder = true;
                            break;
                        }
                    }
                }
                if(m_mesh->faces[f]->tets[1] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
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
        if(draw)
        {
            cleaver::Tet *tet1 = 0; 
            cleaver::Tet *tet2 = 0;

            float default_color[3] = {0.9f, 0.9f, 0.9f};
            float good_color[3]    = {0.3f, 1.0f, 0.0f};
            float  bad_color[3]    = {1.0f, 0.3f, 0.0f};

            float *color1 = default_color, *color2 = default_color;
            if(m_mesh->faces[f]->tets[0] >= 0){
                
                tet1 = m_mesh->tets[m_mesh->faces[f]->tets[0]];
                if(m_bColorByQuality){
                    float t   = tet1->minAngle() / 90.0f;
                    color1[0] = (1 - t)*bad_color[0] + t*good_color[0];
                    color1[1] = (1 - t)*bad_color[1] + t*good_color[1];
                    color1[2] = (1 - t)*bad_color[2] + t*good_color[2];
                } else {
                    color1 = color_for_label[(int)tet1->mat_label];
                }
			}
            if(m_mesh->faces[f]->tets[1] >= 0){
                
                tet2 = m_mesh->tets[m_mesh->faces[f]->tets[1]];
                if(m_bColorByQuality){
                    float t   = tet2->minAngle() / 90.0f;
                    color2[0] = (1 - t)*bad_color[0] + t*good_color[0];
                    color2[1] = (1 - t)*bad_color[1] + t*good_color[1];
                    color2[2] = (1 - t)*bad_color[2] + t*good_color[2];
                } else {
                    color2 = color_for_label[(int)tet2->mat_label];
                }
			}
            if(m_mesh->faces[f]->tets[0] < 0)
                color1 = color2;
            if(m_mesh->faces[f]->tets[1] < 0)
                color2 = color1;

            cleaver::vec3 normal = m_mesh->faces[f]->normal;

            // set vertex positions and colors
            for(int v=0; v < 3; v++)
            {
                cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
                
                PositionData.push_back(vertex->pos().x);
                PositionData.push_back(vertex->pos().y);
                PositionData.push_back(vertex->pos().z);

                NormalData.push_back(normal.x);
                NormalData.push_back(normal.y);
                NormalData.push_back(normal.z);

                ColorData.push_back((int)(0.5*(color1[0]+color2[0])*255));
                ColorData.push_back((int)(0.5*(color1[1]+color2[1])*255));
                ColorData.push_back((int)(0.5*(color1[2]+color2[2])*255));
                ColorData.push_back((int)(1.0f*255));
            }

        }
    }
    m_colorUpdate = false;


    m_meshVertexCount = PositionData.size() / 3;
    GLsizeiptr PositionSize = PositionData.size() * sizeof(GLfloat);
    GLsizeiptr ColorSize    =    ColorData.size() * sizeof(GLubyte);
    GLsizeiptr NormalSize   =   NormalData.size() * sizeof(GLfloat);

    // upload data to VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[VERTEX_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[COLOR_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, ColorSize, ColorData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_meshVBO[NORMAL_OBJECT]);
    glBufferData(GL_ARRAY_BUFFER, NormalSize, NormalData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Now update Cuts List
    if(m_mesher && m_mesher->interfacesComputed())
    {
        PositionData.clear();
        std::map<std::pair<int, int>, cleaver::HalfEdge*>::iterator edgesIter = m_mesh->halfEdges.begin();

        // reset evaluation flag, so we can use to avoid duplicates
        while(edgesIter != m_mesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*edgesIter).second;
            edge->evaluated = false;
            edgesIter++;
        }

        // now grab each cut only once
        edgesIter = m_mesh->halfEdges.begin();
        while(edgesIter != m_mesh->halfEdges.end())
        {
            cleaver::HalfEdge *edge = (*edgesIter).second;
            if(edge->cut && edge->cut->order() == 1 && !edge->evaluated)
            {
                PositionData.push_back(edge->cut->pos().x);
                PositionData.push_back(edge->cut->pos().y);
                PositionData.push_back(edge->cut->pos().z);
                edge->evaluated = true;
                edge->mate->evaluated = true;
            }

            edgesIter++;
        }

        m_cutVertexCount = PositionData.size() / 3;
        PositionSize = PositionData.size() * sizeof(GLfloat);

        // upload data to VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_cutVBO);
        glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData.data(), GL_STATIC_DRAW);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}


// column major multiplication
void matrixMultiply(float A[16], float B[16], float C[16])
{
    for(int j=0; j < 4; j++)
    {
        for(int i=0; i < 4; i++)
        {
            // compute result C[i][j];
            C[i+4*j] = 0;

            for(int k=0; k < 4; k++)
                C[i+4*j] += A[i+4*k]*B[k+4*j];
        }
    }
}

cleaver::vec3 matrixVector(float A[16], const cleaver::vec3 &x, float &depth)
{
    float r[4] = {0};
    float xx[4];
    xx[0] = x.x;
    xx[1] = x.y;
    xx[2] = x.z;
    xx[3] = 1;

    for(int i=0; i < 4; i++){
        for(int k=0; k < 4; k++)
        {
            r[i] += A[i+4*k]*xx[k];
        }
    }

    depth = r[3];

    return cleaver::vec3((r[0] / r[3]), r[1] / r[3], r[2] / r[3]);
}

/*
void MeshWindow::printModelViewProjection()
{

    int w = this->width();
    int h = this->height();

    GLdouble aspectRatio = (GLdouble) w / (float)h;
    GLdouble zNear = 0.1;
    GLdouble zFar  = 500.0;
    GLdouble yFovInDegrees = 45;
    GLdouble top    = zNear * tan(yFovInDegrees * M_PI / 360.0);
    GLdouble bottom = -top;
    GLdouble right = top*aspectRatio;
    GLdouble left  = -right;

    GLdouble projectionMatrix[16];
    GLdouble A = (right + left) / (right - left);
    GLdouble B = (top + bottom) / (top - bottom);
    GLdouble C = -(zFar + zNear)   / (zFar - zNear);
    GLdouble D = -(2*zFar*zNear)   / (zFar - zNear);
    projectionMatrix[0] = 2*zNear/(right - left);
    projectionMatrix[1] = 0;
    projectionMatrix[2] = 0;
    projectionMatrix[3] = 0;
    projectionMatrix[4] = 0;
    projectionMatrix[5] = 2*zNear/(top-bottom);
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

    float *viewMatrix = m_camera->viewMatrix();

    float viewProjectionMatrix[16];
    matrixMultiply(viewMatrix, projectionMatrix, viewProjectionMatrix);

    std::cout  << "Projection Matrix: "
}
*/

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

    GLdouble aspectRatio = (GLdouble) w / (float)h;
    GLdouble zNear = 0.1;
    GLdouble zFar  = 500.0;
    GLdouble yFovInDegrees = 45;
    GLdouble top    = zNear * tan(yFovInDegrees * M_PI / 360.0);
    GLdouble bottom = -top;
    GLdouble right = top*aspectRatio;
    GLdouble left  = -right;

    float projectionMatrix[16];
    GLdouble A = (right + left) / (right - left);
    GLdouble B = (top + bottom) / (top - bottom);
    GLdouble C = -(zFar + zNear)   / (zFar - zNear);
    GLdouble D = -(2*zFar*zNear)   / (zFar - zNear);
    projectionMatrix[0] = 2*zNear/(right - left);
    projectionMatrix[1] = 0;
    projectionMatrix[2] = 0;
    projectionMatrix[3] = 0;
    projectionMatrix[4] = 0;
    projectionMatrix[5] = 2*zNear/(top-bottom);
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

    float *viewMatrix = m_camera->viewMatrix();
    std::cout << "Computed ModelView Matrix:" << std::endl;
    printMatrix(viewMatrix);

    float viewProjectionMatrix[16];
    matrixMultiply(projectionMatrix, viewMatrix, viewProjectionMatrix);

    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << w << "px\" height =\"" << h << "px\" viewBox = \"0 0 " << w << " " << h << "\">" << std::endl;

    std::vector<triangle> triangle_list;

    for(size_t f=0; f < m_mesh->faces.size(); f++)
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

        if(num_adj_tets == 1)
            exterior = true;

        if(num_adj_tets == 2 && m_mesh->tets[t1]->mat_label != m_mesh->tets[t2]->mat_label)
            surface = true;

        int m1 = -1;
        int m2 = -2;
        if(t1 >= 0){
            m1 = m_mesh->tets[t1]->mat_label;
            if(m_bMaterialFaceLock[m1])
                force = true;
        }
        if(t2 >= 0){
            m2 = m_mesh->tets[t2]->mat_label;
            if(m_bMaterialFaceLock[m2])
                force = true;
        }
        if(m1 == m2)
            force = false;

        if(m_bClipping)
        {
            cleaver::vec3 n(m_4fvClippingPlane[0], m_4fvClippingPlane[1], m_4fvClippingPlane[2]);
            float d = m_4fvClippingPlane[3];

            // does plane cut through face?
            for(int v=0; v < 3; v++)
            {
                // is vertex on proper side of the plane?
                cleaver::Vertex *vertex = m_mesh->verts[m_mesh->faces[f]->verts[v]];
                cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                if(n.dot(p) - d > 1E-4){
                    clipped = true;
                    break;
                }
            }

            // determine if face is on border of nonclipped faces
            if(!clipped)
            {
                // look at both adjacent tets
                if(m_mesh->faces[f]->tets[0] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[0]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
                            clipborder = true;
                            break;
                        }
                    }
                }
                if(m_mesh->faces[f]->tets[1] > 0)
                {
                    cleaver::Tet *tet = m_mesh->tets[m_mesh->faces[f]->tets[1]];

                    for(int v=0; v < 4; v++){
                        cleaver::Vertex *vertex = tet->verts[v];
                        cleaver::vec3 p(vertex->pos().x, vertex->pos().y, vertex->pos().z);

                        if(n.dot(p) - d > 1E-4){
                            clipborder = true;
                            break;
                        }
                    }
                }
            }
        }

        if((((!clipped && exterior) || clipborder) && !m_bSurfacesOnly) || (m_bSurfacesOnly && !clipped && surface) || force)
        {
            cleaver::Tet *tet1 = m_mesh->tets[m_mesh->faces[f]->tets[0]];
            cleaver::Tet *tet2 = m_mesh->tets[m_mesh->faces[f]->tets[1]];

            float default_color[3] = {0.9f, 0.9f, 0.9f};
            float *color1 = default_color, *color2 = default_color;
            if(m_mesh->faces[f]->tets[0] >= 0)
                color1 = color_for_label[(int)tet1->mat_label];
            if(m_mesh->faces[f]->tets[1] >= 0)
                color2 = color_for_label[(int)tet2->mat_label];
            if(m_mesh->faces[f]->tets[0] < 0)
                color1 = color2;
            if(m_mesh->faces[f]->tets[1] < 0)
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
            for(int v=0; v < 3; v++)
            {
                cleaver::Vertex *v1 = m_mesh->verts[m_mesh->faces[f]->verts[(v+0)%3]];
                cleaver::Vertex *v2 = m_mesh->verts[m_mesh->faces[f]->verts[(v+1)%3]];

                float depth1;
                float depth2;

                cleaver::vec3 p1 = matrixVector(viewProjectionMatrix, v1->pos(), depth1);
                cleaver::vec3 p2 = matrixVector(viewProjectionMatrix, v2->pos(), depth2);

                float x1 = w * (p1.x + 1) / 2;
                float y1 = h -  h * (p1.y + 1) / 2;
                //float x2 = w * (p2.x + 1) / 2;
                //float y2 = h - h * (p2.y + 1) / 2;


                tri.x[v] = x1;
                tri.y[v] = y1;
                tri.d[v] = depth1;


                //ss << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\" style=\"stroke: black; stoke-width:2.0\"/>" << std::endl;

                //ss << x1 << "," << y1 << " ";

            }

            //std::string color = QColor((int)(0.5*(color1[0]+color2[0])*255), (int)(0.5*(color1[1]+color2[1])*255), (int)(0.5*(color1[2]+color2[2])*255), 255).name();
            tri.color = QColor((int)(0.5*(color1[0]+color2[0])*255), (int)(0.5*(color1[1]+color2[1])*255), (int)(0.5*(color1[2]+color2[2])*255), 255);

            triangle_list.push_back(tri);


            //ss << "\" style=\"fill:" << color << ";stroke-linejoin:round;stroke:black;stroke-width:" << stroke_width << "\"/>\n" << std::endl;

        }
    }

    sort(triangle_list.begin(), triangle_list.end(), triangle_sort);
    for(unsigned int i=0; i < triangle_list.size(); i++)
    {
        triangle t = triangle_list[i];
        ss << "<polygon points=\" ";

        float avg_length = 0;


        for(int v=0; v < 3; v++){
            ss << t.x[v] << "," << t.y[v] << " ";
            float dx = t.x[v] - t.x[(v+1)%3];
            float dy = t.y[v] - t.y[(v+1)%3];
            avg_length += (1.0/3.0)*(sqrt(dx*dx + dy*dy));
        }
        //float stroke_width = avg_length / 30.0f;
        float stroke_width = 0.5f;

        ss << "\" style=\"fill:" << t.color.name().toStdString() << ";stroke-linejoin:round;stroke-opacity:0.4;stroke:black;stroke-width:" <<  stroke_width << "\"/>\n" << std::endl;
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
