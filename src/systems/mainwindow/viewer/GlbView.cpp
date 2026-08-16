#include "systems/mainwindow/viewer/GlbView.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLShaderProgram>
#include <QSurfaceFormat>
#include <QMatrix4x4>
#include <QVector4D>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <functional>
#include <cmath>

namespace pvd
{
    GlbView::GlbView(QWidget *p) : QOpenGLWidget(p)
    {
        QSurfaceFormat f;
        f.setDepthBufferSize(24);
        f.setSamples(4);
        setFormat(f);
        setMinimumSize(260, 260);
        resetView();
    }
    void GlbView::resetView()
    {
        projectionMode_ = ProjectionMode::Perspective;
        const QVector3D direction = QVector3D(.60f, .85f, 1.00f).normalized();
        const QVector3D forward = -direction;
        const QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
        const QVector3D up = QVector3D::crossProduct(right, forward).normalized();
        QMatrix3x3 basis;
        basis(0, 0) = right.x(); basis(1, 0) = right.y(); basis(2, 0) = right.z();
        basis(0, 1) = up.x(); basis(1, 1) = up.y(); basis(2, 1) = up.z();
        basis(0, 2) = -forward.x(); basis(1, 2) = -forward.y(); basis(2, 2) = -forward.z();
        cameraRotation_ = QQuaternion::fromRotationMatrix(basis).normalized();
        cameraTarget_ = {0, 0, 0};
        cameraDistance_ = 3.5f;
        update();
    }
    void GlbView::topView()
    {
        projectionMode_ = ProjectionMode::Orthographic;
        cameraRotation_ = QQuaternion::fromAxisAndAngle(1, 0, 0, -90).normalized();
        cameraTarget_ = {0, 0, 0};
        orthographicScale_ = 1.05f;
        update();
    }
    void GlbView::setSelectedComponent(const QString& id)
    {
        selectedComponent_ = id;
        for (int triangle = 0; triangle < triangleComponentIds_.size(); ++triangle)
        {
            const float mark = !id.isEmpty() && triangleComponentIds_[triangle] == id ? 1.f : 0.f;
            for (int vertex = 0; vertex < 3; ++vertex)
                vertices_[triangle * 3 + vertex].highlight = mark;
        }
        if (gpuReady_)
        {
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, vertices_.size() * int(sizeof(Vertex)), vertices_.constData(), GL_STATIC_DRAW);
            doneCurrent();
        }
        update();
    }

    void GlbView::cameraBasis(QVector3D& position, QVector3D& right, QVector3D& up, QVector3D& forward) const
    {
        forward = cameraRotation_.rotatedVector({0, 0, -1}).normalized();
        up = cameraRotation_.rotatedVector({0, 1, 0}).normalized();
        right = cameraRotation_.rotatedVector({1, 0, 0}).normalized();
        position = cameraTarget_ - forward * cameraDistance_;
    }

    QString GlbView::componentAt(const QPoint& mousePosition) const
    {
        if (vertices_.isEmpty())
            return {};
        QVector3D camera, right, up, forward;
        cameraBasis(camera, right, up, forward);
        const float aspect = float(width()) / std::max(1, height());
        QMatrix4x4 projection, view;
        if (projectionMode_ == ProjectionMode::Orthographic)
            projection.ortho(-aspect * orthographicScale_, aspect * orthographicScale_, -orthographicScale_, orthographicScale_, .0005f, 100.f);
        else
            projection.perspective(45.f, aspect, std::max(.0005f, cameraDistance_ * .002f), std::max(100.f, cameraDistance_ + sceneRadius_ * 20.f));
        view.lookAt(camera, cameraTarget_, up);
        auto screenPoint = [&](const Vertex& vertex)
        {
            const QVector4D clip = projection * view * QVector4D(vertex.x, vertex.y, vertex.z, 1.f);
            if (std::abs(clip.w()) < .000001f)
                return QPointF(-1e9, -1e9);
            return QPointF((clip.x() / clip.w() * .5f + .5f) * width(), (1.f - (clip.y() / clip.w() * .5f + .5f)) * height());
        };
        const QPointF p(mousePosition);
        QString hit;
        float nearestDepth = 1e30f;
        for (int triangle = 0; triangle < triangleComponentIds_.size(); ++triangle)
        {
            if (triangleComponentIds_[triangle].isEmpty())
                continue;
            const int base = triangle * 3;
            const QPointF a = screenPoint(vertices_[base]), b = screenPoint(vertices_[base + 1]), c = screenPoint(vertices_[base + 2]);
            const float denominator = (b.y() - c.y()) * (a.x() - c.x()) + (c.x() - b.x()) * (a.y() - c.y());
            if (std::abs(denominator) < .000001f)
                continue;
            const float u = ((b.y() - c.y()) * (p.x() - c.x()) + (c.x() - b.x()) * (p.y() - c.y())) / denominator;
            const float v = ((c.y() - a.y()) * (p.x() - c.x()) + (a.x() - c.x()) * (p.y() - c.y())) / denominator;
            if (u < 0.f || v < 0.f || u + v > 1.f)
                continue;
            const QVector3D centroid((vertices_[base].x + vertices_[base + 1].x + vertices_[base + 2].x) / 3.f, (vertices_[base].y + vertices_[base + 1].y + vertices_[base + 2].y) / 3.f, (vertices_[base].z + vertices_[base + 1].z + vertices_[base + 2].z) / 3.f);
            const float depth = QVector3D::dotProduct(centroid - camera, forward);
            if (depth > 0.f && depth < nearestDepth)
            {
                nearestDepth = depth;
                hit = triangleComponentIds_[triangle];
            }
        }
        return hit;
    }
    bool GlbView::load(const QString &path, QString *error)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
        {
            if (error)
                *error = "Cannot open GLB";
            return false;
        }
        QByteArray b = f.readAll();
        if (b.size() < 28 || b.left(4) != "glTF")
        {
            if (error)
                *error = "Invalid GLB";
            return false;
        }
        quint32 js = *reinterpret_cast<const quint32 *>(b.constData() + 12);
        QJsonObject root = QJsonDocument::fromJson(b.mid(20, js)).object();
        QByteArray bin = b.mid(28 + js);
        QJsonArray acc = root["accessors"].toArray(), views = root["bufferViews"].toArray(), meshes = root["meshes"].toArray(), nodes = root["nodes"].toArray(), mats = root["materials"].toArray();
        vertices_.clear();
        triangleComponentIds_.clear();
        auto ident = []()
        {QJsonArray a;for(int i=0;i<16;i++)a.append(i%5==0?1.:0.);return a; };
        auto mul = [](const QJsonArray &a, const QJsonArray &b)
        {QJsonArray c;for(int i=0;i<16;i++)c.append(0.);for(int col=0;col<4;col++)for(int row=0;row<4;row++){double x=0;for(int k=0;k<4;k++)x+=a[k*4+row].toDouble()*b[col*4+k].toDouble();c[col*4+row]=x;}return c; };
        QVector<int> parent(nodes.size(), -1);
        for (int i = 0; i < nodes.size(); i++)
            for (auto c : nodes[i].toObject()["children"].toArray())
                parent[c.toInt()] = i;
        QVector<QJsonArray> world(nodes.size());
        QVector<bool> ready(nodes.size(), false);
        std::function<QJsonArray(int)> wm = [&](int i)
        {if(ready[i])return world[i];auto n=nodes[i].toObject();auto local=n.contains("matrix")?n["matrix"].toArray():ident();world[i]=parent[i]<0?local:mul(wm(parent[i]),local);ready[i]=true;return world[i]; };
        auto pos = [&](int ai, int j)
        {auto a=acc[ai].toObject(),v=views[a["bufferView"].toInt()].toObject();int off=v["byteOffset"].toInt()+a["byteOffset"].toInt()+j*v["byteStride"].toInt(12);float q[3];std::memcpy(q,bin.constData()+off,12);return QVector3D(q[0],q[1],q[2]); };
        auto tr = [](const QJsonArray &m, const QVector3D &p)
        { return QVector3D(m[0].toDouble() * p.x() + m[4].toDouble() * p.y() + m[8].toDouble() * p.z() + m[12].toDouble(), m[1].toDouble() * p.x() + m[5].toDouble() * p.y() + m[9].toDouble() * p.z() + m[13].toDouble(), m[2].toDouble() * p.x() + m[6].toDouble() * p.y() + m[10].toDouble() * p.z() + m[14].toDouble()); };
        auto componentId = [&](int nodeIndex)
        {
            for (int current = nodeIndex; current >= 0; current = parent[current])
            {
                const QString name = nodes[current].toObject().value("name").toString();
                if (name == "RP2350A") return QString("rp2350a");
                if (name == "Wireless_RF_Shield") return QString("wireless");
                if (name == "SWD_Debug_Header") return QString("debug_probe");
                if (name == "Micro_USB_Connector") return QString("usb_connector");
                if (name == "BOOTSEL_Button") return QString("bootsel");
                if (name == "Onboard_LED") return QString("onboard_led");
                if (name.startsWith("Pin_")) return QString("pin_%1").arg(name.mid(4).toInt());
            }
            return QString();
        };
        auto normalTransform = [](const QJsonArray &m, const QVector3D &n)
        {
            QMatrix4x4 q;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    q(row, col) = float(m[col * 4 + row].toDouble());
            const QMatrix3x3 nm = q.normalMatrix();
            return QVector3D(nm(0, 0) * n.x() + nm(0, 1) * n.y() + nm(0, 2) * n.z(), nm(1, 0) * n.x() + nm(1, 1) * n.y() + nm(1, 2) * n.z(), nm(2, 0) * n.x() + nm(2, 1) * n.y() + nm(2, 2) * n.z()).normalized();
        };
        QVector<QVector3D> all;
        for (int ni = 0; ni < nodes.size(); ni++)
        {
            auto node = nodes[ni].toObject();
            if (!node.contains("mesh"))
                continue;
            auto matrix = wm(ni);
            const QString component = componentId(ni);
            auto mesh = meshes[node["mesh"].toInt()].toObject();
            for (auto pv : mesh["primitives"].toArray())
            {
                auto prim = pv.toObject(), attrs = prim["attributes"].toObject();
                if (!attrs.contains("POSITION"))
                    continue;
                int ai = attrs["POSITION"].toInt(), count = acc[ai].toObject()["count"].toInt();
                QVector<QVector3D> p;
                QVector<QVector3D> normals;
                const bool hasNormals = attrs.contains("NORMAL");
                const int normalAccessor = hasNormals ? attrs["NORMAL"].toInt() : -1;
                for (int j = 0; j < count; j++)
                {
                    p.push_back(tr(matrix, pos(ai, j)));
                    if (hasNormals)
                        normals.push_back(normalTransform(matrix, pos(normalAccessor, j)));
                    all.push_back(p.back());
                }
                QColor col("#64828a");
                if (prim.contains("material"))
                {
                    const int materialIndex = prim["material"].toInt();
                    auto a = mats[materialIndex].toObject()["pbrMetallicRoughness"].toObject()["baseColorFactor"].toArray();
                    if (a.size() >= 3)
                        col = QColor::fromRgbF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a.size() > 3 ? a[3].toDouble() : 1.);
                    if (materialIndex == 24)
                        col = QColor::fromRgbF(.82, .008, .035, 1.0);
                }
                QVector<quint32> ix;
                if (prim.contains("indices"))
                {
                    auto a = acc[prim["indices"].toInt()].toObject(), v = views[a["bufferView"].toInt()].toObject();
                    int type = a["componentType"].toInt(), step = type == 5125 ? 4 : 2, off = v["byteOffset"].toInt() + a["byteOffset"].toInt();
                    for (int j = 0; j < a["count"].toInt(); j++)
                    {
                        const char *q = bin.constData() + off + j * step;
                        ix.push_back(type == 5125 ? *reinterpret_cast<const quint32 *>(q) : *reinterpret_cast<const quint16 *>(q));
                    }
                }
                else
                    for (int j = 0; j < count; j++)
                        ix.push_back(j);
                for (int j = 0; j + 2 < ix.size(); j += 3)
                {
                    auto n = QVector3D::crossProduct(p[ix[j + 1]] - p[ix[j]], p[ix[j + 2]] - p[ix[j]]).normalized();
                    for (int k = 0; k < 3; k++)
                    {
                        auto &q = p[ix[j + k]];
                        const QVector3D vertexNormal = hasNormals ? normals[ix[j + k]] : n;
                        vertices_.push_back({q.x(), q.y(), q.z(), vertexNormal.x(), vertexNormal.y(), vertexNormal.z(), float(col.redF()), float(col.greenF()), float(col.blueF()), float(col.alphaF()), 0.f});
                    }
                    triangleComponentIds_.push_back(component);
                }
            }
        }
        if (all.isEmpty())
        {
            if (error)
                *error = "GLB contains no geometry";
            return false;
        }
        QVector3D lo = all[0], hi = lo;
        for (auto &q : all)
        {
            lo.setX(std::min(lo.x(), q.x()));
            lo.setY(std::min(lo.y(), q.y()));
            lo.setZ(std::min(lo.z(), q.z()));
            hi.setX(std::max(hi.x(), q.x()));
            hi.setY(std::max(hi.y(), q.y()));
            hi.setZ(std::max(hi.z(), q.z()));
        }
        float s = 1.4f / std::max({hi.x() - lo.x(), hi.y() - lo.y(), hi.z() - lo.z()}), cx = (lo.x() + hi.x()) * .5f, cy = (lo.y() + hi.y()) * .5f, cz = (lo.z() + hi.z()) * .5f;
        sceneRadius_ = (hi - QVector3D(cx, cy, cz)).length() * s;
        for (auto &v : vertices_)
        {
            v.x = (v.x - cx) * s;
            v.y = (v.y - cy) * s;
            v.z = (v.z - cz) * s;
        }
        if (gpuReady_)
        {
            makeCurrent();
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, vertices_.size() * int(sizeof(Vertex)), vertices_.constData(), GL_STATIC_DRAW);
            doneCurrent();
        }
        update();
        return true;
    }
    void GlbView::initializeGL()
    {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_MULTISAMPLE);
        glDisable(GL_CULL_FACE);
        glClearColor(.78f, .88f, .95f, 1.f);
        program_ = new QOpenGLShaderProgram(this);
        const bool vertexOk=program_->addShaderFromSourceCode(QOpenGLShader::Vertex, "attribute vec3 aPosition;attribute vec3 aNormal;attribute vec4 aColor;attribute float aHighlight;uniform mat4 uMvp;varying vec3 vNormal;varying vec4 vColor;varying float vHighlight;void main(){gl_Position=uMvp*vec4(aPosition,1.0);vNormal=normalize(aNormal);vColor=aColor;vHighlight=aHighlight;}");
        const bool fragmentOk=program_->addShaderFromSourceCode(QOpenGLShader::Fragment, "varying vec3 vNormal;varying vec4 vColor;varying float vHighlight;void main(){vec3 n=normalize(vNormal);vec3 key=normalize(vec3(-.25,1.0,-.35));vec3 fill=normalize(vec3(.7,.65,.5));float kd=.44+1.0*max(dot(n,key),0.0)+.28*max(dot(n,fill),0.0);float spec=.12*pow(max(dot(n,normalize(key+vec3(0.0,1.0,0.0))),0.0),24.0);vec3 base=mix(vColor.rgb,vec3(1.0,.72,.05),vHighlight*.78);vec3 color=min(base*kd+vec3(spec)+vec3(vHighlight*.18),vec3(1.0));gl_FragColor=vec4(color,vColor.a);}");
        const bool linked=program_->link();
        if(!vertexOk||!fragmentOk||!linked) qWarning()<<"Viewer shader error:"<<program_->log();
        vao_.create();
        vao_.bind();
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * int(sizeof(Vertex)), vertices_.constData(), GL_STATIC_DRAW);
        int p = program_->attributeLocation("aPosition"), n = program_->attributeLocation("aNormal"), c = program_->attributeLocation("aColor"), h = program_->attributeLocation("aHighlight");
        program_->enableAttributeArray(p);
        program_->setAttributeBuffer(p, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
        program_->enableAttributeArray(n);
        program_->setAttributeBuffer(n, GL_FLOAT, offsetof(Vertex, nx), 3, sizeof(Vertex));
        program_->enableAttributeArray(c);
        program_->setAttributeBuffer(c, GL_FLOAT, offsetof(Vertex, r), 4, sizeof(Vertex));
        program_->enableAttributeArray(h);
        program_->setAttributeBuffer(h, GL_FLOAT, offsetof(Vertex, highlight), 1, sizeof(Vertex));
        vao_.release();
        gpuReady_ = true;
    }
    void GlbView::resizeGL(int w, int h) { glViewport(0, 0, w, h); }
    void GlbView::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!program_ || vertices_.isEmpty())
            return;
        float aspect = float(width()) / std::max(1, height());
        QVector3D cam, right, up, forward;
        cameraBasis(cam, right, up, forward);
        QMatrix4x4 proj, view;
        if (projectionMode_ == ProjectionMode::Orthographic)
        {
            proj.ortho(-aspect * orthographicScale_, aspect * orthographicScale_, -orthographicScale_, orthographicScale_, .0005f, 100.f);
        }
        else
        {
            const float nearPlane = std::max(.0005f, cameraDistance_ * .002f);
            const float farPlane = std::max(100.f, cameraDistance_ + sceneRadius_ * 20.f);
            proj.perspective(45.f, aspect, nearPlane, farPlane);
        }
        view.lookAt(cam, cameraTarget_, up);
        program_->bind();
        program_->setUniformValue("uMvp", proj * view);
        vao_.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
        vao_.release();
        program_->release();
    }
    void GlbView::mousePressEvent(QMouseEvent *e)
    {
        lastMouse_ = e->pos();
        if (e->button() == Qt::LeftButton)
        {
            dragMode_ = DragMode::Orbit;
            const QString hit = componentAt(e->pos());
            if (!hit.isEmpty() && componentClicked)
                componentClicked(hit);
        }
        else if (e->button() == Qt::RightButton)
        {
            dragMode_ = DragMode::Pan;
            setCursor(Qt::ClosedHandCursor);
        }
        e->accept();
    }
    void GlbView::mouseMoveEvent(QMouseEvent *e)
    {
        auto d = e->pos() - lastMouse_;
        lastMouse_ = e->pos();
        if (dragMode_ == DragMode::Orbit && (e->buttons() & Qt::LeftButton))
        {
            const QVector3D forward = cameraRotation_.rotatedVector({0, 0, -1}).normalized();
            const QVector3D up = cameraRotation_.rotatedVector({0, 1, 0}).normalized();
            const QVector3D right = QVector3D::crossProduct(forward, up).normalized();
            const QQuaternion yawRotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), -d.x() * .45f);
            const QQuaternion pitchRotation = QQuaternion::fromAxisAndAngle(right, -d.y() * .45f);
            cameraRotation_ = (yawRotation * pitchRotation * cameraRotation_).normalized();
            update();
        }
        else if (dragMode_ == DragMode::Pan && (e->buttons() & Qt::RightButton))
        {
            QVector3D position, right, up, forward;
            cameraBasis(position, right, up, forward);
            const float worldPerPixel = projectionMode_ == ProjectionMode::Orthographic ? (2.f * orthographicScale_ / std::max(1, height())) : (2.f * cameraDistance_ * std::tan(22.5f * 3.14159265f / 180.f) / std::max(1, height()));
            cameraTarget_ += (-float(d.x()) * worldPerPixel * right) + (float(d.y()) * worldPerPixel * up);
            update();
        }
        e->accept();
    }
    void GlbView::mouseReleaseEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::RightButton)
            unsetCursor();
        dragMode_ = DragMode::None;
        e->accept();
    }
    void GlbView::wheelEvent(QWheelEvent *e)
    {
        const float factor = e->angleDelta().y() > 0 ? .88f : 1.14f;
        if (projectionMode_ == ProjectionMode::Orthographic)
            orthographicScale_ = std::clamp(orthographicScale_ * factor, .02f, 4.f);
        else
            cameraDistance_ = std::clamp(cameraDistance_ * factor, std::max(.0005f, sceneRadius_ * .03f), 20.f);
        update();
    }
}
