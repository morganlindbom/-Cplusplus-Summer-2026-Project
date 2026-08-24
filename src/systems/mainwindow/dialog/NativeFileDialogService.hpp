// NativeFileDialogService.hpp
#pragma once

#include "systems/mainwindow/dialog/IFileDialogService.hpp"

namespace pvd
{
class NativeFileDialogService final : public IFileDialogService
{
  public:
    /**Constructs the production file-dialog implementation.*/
    NativeFileDialogService() = default;

    /**Shows the platform-native Open dialog.*/
    QString getOpenFileName(QWidget* parent, const QString& title, const QString& directory,
                            const QString& filter) override;

    /**Shows the platform-native directory dialog.*/
    QString getExistingDirectory(QWidget* parent, const QString& title, const QString& directory = {}) override;
};
} // namespace pvd
