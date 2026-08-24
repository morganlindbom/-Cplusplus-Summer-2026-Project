// IFileDialogService.hpp
#pragma once

#include <QString>
#include <memory>

class QWidget;

namespace pvd
{
class IFileDialogService
{
  public:
    virtual ~IFileDialogService() = default;

    /**Shows an Open dialog and returns the selected file, or an empty string on cancel.*/
    virtual QString getOpenFileName(QWidget* parent, const QString& title, const QString& directory,
                                    const QString& filter) = 0;

    /**Shows a directory dialog and returns the selected directory, or an empty string on cancel.*/
    virtual QString getExistingDirectory(QWidget* parent, const QString& title, const QString& directory = {}) = 0;
};
} // namespace pvd

namespace pvd
{
/**Creates the dialog service selected by the explicit application mode.*/
std::unique_ptr<IFileDialogService> makeFileDialogService(bool certificationDialogs);
} // namespace pvd
