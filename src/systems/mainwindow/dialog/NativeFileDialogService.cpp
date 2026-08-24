// NativeFileDialogService.cpp
#include "systems/mainwindow/dialog/NativeFileDialogService.hpp"
#include <QFileDialog>

namespace pvd
{
QString NativeFileDialogService::getOpenFileName(QWidget* parent, const QString& title, const QString& directory,
                                                 const QString& filter)
{
    /**Preserves the existing native Open Project dialog semantics.*/
    return QFileDialog::getOpenFileName(parent, title, directory, filter);
}

QString NativeFileDialogService::getExistingDirectory(QWidget* parent, const QString& title, const QString& directory)
{
    /**Preserves the existing native directory-selection semantics.*/
    return QFileDialog::getExistingDirectory(parent, title, directory);
}
} // namespace pvd
