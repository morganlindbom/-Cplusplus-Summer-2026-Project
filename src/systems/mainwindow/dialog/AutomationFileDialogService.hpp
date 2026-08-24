// AutomationFileDialogService.hpp
#pragma once

#include "systems/mainwindow/dialog/IFileDialogService.hpp"

namespace pvd
{
class AutomationFileDialogService final : public IFileDialogService
{
  public:
    /**Constructs the deterministic Qt-owned file-dialog implementation.*/
    AutomationFileDialogService() = default;

    /**Shows a non-native Qt Open dialog for GUI certification.*/
    QString getOpenFileName(QWidget* parent, const QString& title, const QString& directory,
                            const QString& filter) override;

    /**Shows a non-native Qt directory dialog for GUI certification.*/
    QString getExistingDirectory(QWidget* parent, const QString& title, const QString& directory = {}) override;
};
} // namespace pvd
