// MainWindow.hpp
#pragma once
#include <functional>
#include <QHash>
#include <QMainWindow>
class QStackedWidget;
class QCloseEvent;
namespace pvd
{
class Workflow;
class Viewer;
class MainWindow final : public QMainWindow
{
    Q_OBJECT
  public:
    explicit MainWindow(Workflow* workflow, Viewer* viewer, QWidget* parent = nullptr);
    void registerWorkflow(const QString& id, QWidget* column2, QWidget* column3);
    void activateWorkflow(const QString& id);
    void setCloseGuard(std::function<bool()> guard);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    Workflow* workflow_ = nullptr;
    Viewer* viewer_ = nullptr;
    QStackedWidget* column2Stack_ = nullptr;
    QStackedWidget* column3Stack_ = nullptr;
    QHash<QString, int> indices_;
    std::function<bool()> closeGuard_;
};
} // namespace pvd
