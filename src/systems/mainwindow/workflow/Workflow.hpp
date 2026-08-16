// Workflow.hpp
#pragma once
#include <QWidget>
class QListWidget;
namespace pvd {
class Workflow final : public QWidget
{
    Q_OBJECT
public:
    explicit Workflow(const QString& databasePath, QWidget* parent=nullptr);
    QString currentWorkflow() const;
    void selectWorkflow(const QString& workflowId);
signals:
    void workflowSelected(const QString& workflowId);
private:
    QListWidget* list_ = nullptr;
};
}
