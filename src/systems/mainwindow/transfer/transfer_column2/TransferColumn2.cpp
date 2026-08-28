// TransferColumn2.cpp
#include "systems/mainwindow/transfer/transfer_column2/TransferColumn2.hpp"

#include "systems/database/SqliteUtil.hpp"
#include "systems/mainwindow/PanelUtil.hpp"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>

namespace pvd
{
TransferColumn2::TransferColumn2(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds the transfer-target selection panel with stable automation identifiers.

    Artifact selection remains derived from the active project build directory so a
    transfer cannot silently point at an unrelated firmware file.
    */
    auto* layout = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Transfer Target"));
    auto* form = new QFormLayout();
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);

    artifact_ = new QLineEdit(this);
    artifact_->setObjectName("transfer_artifact");
    artifact_->setReadOnly(true);

    method_ = new QComboBox(this);
    method_->setObjectName("transfer_method");
    method_->addItems({"OpenOCD probe", "picotool load", "Copy UF2 to drive"});

    form->addRow("Firmware artifact", artifact_);
    form->addRow("Method", method_);
    layout->addLayout(form);
    layout->addStretch();
}

void TransferColumn2::setBuildPath(const QString& path, const QString& targetName)
{
    /**Selects the current target UF2 only when it exists in the active build directory.

    An absent artifact clears the field so TransferColumn3's freshness gate reports a
    missing build instead of reusing a stale path from a previous project.
    */
    const QString candidate = QDir(path).filePath(targetName + ".uf2");
    artifact_->setText(QFileInfo::exists(candidate) ? candidate : QString{});
}

QString TransferColumn2::artifact() const
{
    /**Returns the current build-derived UF2 artifact path.

    The value is read-only in the UI to preserve the active-project safety invariant.
    */
    return artifact_->text();
}

QString TransferColumn2::method() const
{
    /**Returns the selected transfer transport.

    Transport execution and safety validation are owned by TransferColumn3.
    */
    return method_->currentText();
}
} // namespace pvd
