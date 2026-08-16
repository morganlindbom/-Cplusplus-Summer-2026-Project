// Viewer.hpp
#pragma once
#include <QHash>
#include <QWidget>
class QLabel;
namespace pvd {
class GlbView;
class Viewer final : public QWidget
{
    Q_OBJECT
public:
    explicit Viewer(const QString& databasePath, QWidget* parent=nullptr);
    void selectComponent(const QString& componentId);
    void setSimulationText(const QString& text);
signals:
    void componentSelected(const QString& componentId);
private:
    void buildBoard();
    GlbView* view_=nullptr;
    QLabel* status_=nullptr;
    QHash<QString,QWidget*> items_;
    QString selected_;
};
}
