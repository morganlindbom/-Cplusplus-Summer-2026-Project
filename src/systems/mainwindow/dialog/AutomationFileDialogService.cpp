// AutomationFileDialogService.cpp
#include "systems/mainwindow/dialog/AutomationFileDialogService.hpp"
#include "systems/mainwindow/dialog/NativeFileDialogService.hpp"
#include <QFileDialog>
#include <QLineEdit>

namespace pvd
{
std::unique_ptr<IFileDialogService> makeFileDialogService(bool certificationDialogs)
{
    /**Selects deterministic dialogs only when the explicit certification mode is enabled.*/
    if (certificationDialogs)
        return std::make_unique<AutomationFileDialogService>();
    return std::make_unique<NativeFileDialogService>();
}

namespace
{
void identifyDialog(QFileDialog& dialog)
{
    /**Adds stable identity to the certification-only Qt dialog and its filename field.*/
    dialog.setObjectName("pvd_automation_file_dialog");
    dialog.setAccessibleName("PVD automation file dialog");
    if (auto* field = dialog.findChild<QLineEdit*>("fileNameEdit"))
    {
        field->setAccessibleName("PVD file name");
        field->setAccessibleDescription("Selected project file name");
    }
}

QString execute(QFileDialog& dialog)
{
    /**Executes one real Qt dialog and returns only its user-selected path.*/
    identifyDialog(dialog);
    return dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty() ? dialog.selectedFiles().first()
                                                                                   : QString{};
}
} // namespace

QString AutomationFileDialogService::getOpenFileName(QWidget* parent, const QString& title, const QString& directory,
                                                     const QString& filter)
{
    /**Creates a deterministic non-native Open dialog without bypassing the GUI workflow.*/
    QFileDialog dialog(parent, title, directory, filter);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    return execute(dialog);
}

QString AutomationFileDialogService::getExistingDirectory(QWidget* parent, const QString& title,
                                                          const QString& directory)
{
    /**Creates a deterministic non-native directory dialog without bypassing the GUI workflow.*/
    QFileDialog dialog(parent, title, directory);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    return execute(dialog);
}
} // namespace pvd
