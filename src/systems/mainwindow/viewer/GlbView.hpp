#pragma once
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QPoint>
#include <QVector3D>
#include <QQuaternion>
#include <functional>
#include <QString>
class QOpenGLShaderProgram;
namespace pvd {
class GlbView final : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    explicit GlbView(QWidget* parent=nullptr);
    bool load(const QString&, QString* error=nullptr);
    void resetView(); void topView();
    void setSelectedComponent(const QString& id);
    std::function<void(const QString&)> componentClicked;
protected:
    void initializeGL() override; void resizeGL(int,int) override; void paintGL() override;
    void mousePressEvent(QMouseEvent*) override; void mouseMoveEvent(QMouseEvent*) override; void mouseReleaseEvent(QMouseEvent*) override; void wheelEvent(QWheelEvent*) override;
private:
    enum class DragMode { None, Orbit, Pan };
    enum class ProjectionMode { Perspective, Orthographic };
    void cameraBasis(QVector3D& position, QVector3D& right, QVector3D& up, QVector3D& forward) const;
    QString componentAt(const QPoint& position) const;
    struct Vertex { float x,y,z,nx,ny,nz,r,g,b,a,highlight; };
    QVector<Vertex> vertices_; QVector<QString> triangleComponentIds_; QOpenGLShaderProgram* program_=nullptr; QOpenGLVertexArrayObject vao_; unsigned int vbo_=0;
    QQuaternion cameraRotation_;
    ProjectionMode projectionMode_=ProjectionMode::Perspective;
    float cameraDistance_=3.5f,orthographicScale_=1.05f,sceneRadius_=0.7f; QVector3D cameraTarget_{0,0,0};
    QPoint lastMouse_; DragMode dragMode_=DragMode::None; bool gpuReady_=false;
    QString selectedComponent_;
};
}
