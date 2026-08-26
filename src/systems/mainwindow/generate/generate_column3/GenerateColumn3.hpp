// GenerateColumn3.hpp
#pragma once
#include <QWidget>
class QLabel;
namespace pvd {
enum class GenerateWorkflowState { Idle, Running, Completed, Failed };
class GenerateColumn3 final : public QWidget {
    Q_OBJECT
public:
    explicit GenerateColumn3(const QString& db, QWidget* parent=nullptr);
    void setWorkflowState(GenerateWorkflowState state);
    GenerateWorkflowState workflowState() const { return workflowState_; }
    void setStatus(const QString& text, bool ok=true);
signals:
    void validateRequested();
    void generateRequested();
private:
    QLabel* status_=nullptr;
    GenerateWorkflowState workflowState_=GenerateWorkflowState::Idle;
};
}
