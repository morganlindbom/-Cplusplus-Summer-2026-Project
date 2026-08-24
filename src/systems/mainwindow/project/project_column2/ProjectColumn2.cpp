#include "systems/mainwindow/project/project_column2/ProjectColumn2.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QSignalBlocker>
namespace pvd
{
ProjectColumn2::ProjectColumn2(const QString& db, QWidget* parent) : QWidget(parent)
{
    auto* l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Project / State Settings"));
    auto* form = new QFormLayout();
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    product_ = new QComboBox(this);
    product_->setObjectName("project_product");
    product_->addItems({"Raspberry Pi Pico 2 W", "Raspberry Pi Pico 2"});
    language_ = new QComboBox(this);
    language_->setObjectName("project_language");
    language_->addItems({"C++", "C"});
    state_ = new QComboBox(this);
    state_->setObjectName("project_state");
    state_->addItems({"Development", "Testing", "Release"});
    form->addRow("Product", product_);
    form->addRow("Language", language_);
    form->addRow("State", state_);
    l->addLayout(form);
    l->addWidget(makePanelTitle("State-specific options", this));
    debugSessionTools_ = new QCheckBox("Debug session tools", this);
    debugSessionTools_->setObjectName("project_debug_session_tools");
    runtimeDiagnostics_ = new QCheckBox("Runtime diagnostics", this);
    runtimeDiagnostics_->setObjectName("project_runtime_diagnostics");
    verboseBuildEvidence_ = new QCheckBox("Verbose build evidence", this);
    verboseBuildEvidence_->setObjectName("project_verbose_build_evidence");
    debugSessionTools_->setChecked(true);
    runtimeDiagnostics_->setChecked(true);
    verboseBuildEvidence_->setChecked(true);
    l->addWidget(debugSessionTools_);
    l->addWidget(runtimeDiagnostics_);
    l->addWidget(verboseBuildEvidence_);
    l->addStretch();
    connect(product_, &QComboBox::currentTextChanged, this, &ProjectColumn2::productChanged);
    connect(language_, &QComboBox::currentTextChanged, this, &ProjectColumn2::languageChanged);
    connect(state_, &QComboBox::currentTextChanged, this, &ProjectColumn2::stateChanged);
    connect(debugSessionTools_, &QCheckBox::toggled, this, &ProjectColumn2::debugSessionToolsChanged);
    connect(runtimeDiagnostics_, &QCheckBox::toggled, this, &ProjectColumn2::runtimeDiagnosticsChanged);
    connect(verboseBuildEvidence_, &QCheckBox::toggled, this, &ProjectColumn2::verboseBuildEvidenceChanged);
}
QString ProjectColumn2::language() const
{
    return language_->currentText();
}
QString ProjectColumn2::stateName() const
{
    return state_->currentText();
}
QString ProjectColumn2::product() const
{
    return product_->currentText();
}
bool ProjectColumn2::debugSessionTools() const
{
    return debugSessionTools_->isChecked();
}
bool ProjectColumn2::runtimeDiagnostics() const
{
    return runtimeDiagnostics_->isChecked();
}
bool ProjectColumn2::verboseBuildEvidence() const
{
    return verboseBuildEvidence_->isChecked();
}
void ProjectColumn2::setProjectState(const QString& product, const QString& language, const QString& state,
                                     bool debugSessionTools, bool runtimeDiagnostics, bool verboseBuildEvidence)
{
    const QSignalBlocker productBlocker(product_);
    const QSignalBlocker languageBlocker(language_);
    const QSignalBlocker stateBlocker(state_);
    const QSignalBlocker debugBlocker(debugSessionTools_);
    const QSignalBlocker runtimeBlocker(runtimeDiagnostics_);
    const QSignalBlocker verboseBlocker(verboseBuildEvidence_);
    product_->setCurrentText(product);
    language_->setCurrentText(language);
    state_->setCurrentText(state);
    debugSessionTools_->setChecked(debugSessionTools);
    runtimeDiagnostics_->setChecked(runtimeDiagnostics);
    verboseBuildEvidence_->setChecked(verboseBuildEvidence);
}
} // namespace pvd
