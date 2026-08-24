// ProjectColumn3.hpp
#pragma once
#include <QWidget>
class QLineEdit;
class QLabel;
namespace pvd
{
class IFileDialogService;
class ProjectColumn3 final : public QWidget
{
  Q_OBJECT public :
      /**Constructs the project panel with its dialog interaction dependency.*/
      explicit ProjectColumn3(const QString& db, IFileDialogService* fileDialogService, QWidget* parent = nullptr);
    void setProject(const QString& name, const QString& path);
    void setStatus(const QString& text, bool ok);
    void setDirty(bool dirty);
  signals:
    void projectNameChanged(const QString& name);
    void createRequested(const QString& name, const QString& path);
    void openRequested(const QString& path);
    void saveRequested(const QString& name, const QString& path);

  private:
    QLineEdit* name_ = nullptr;
    QLineEdit* path_ = nullptr;
    QLabel* status_ = nullptr;
    IFileDialogService* fileDialogService_ = nullptr;
};
} // namespace pvd
