// System.cpp
#include "systems/System.hpp"
#include "systems/SystemDatabase.hpp"
#include "systems/components/FunctionCatalog.hpp"
#include "systems/components/PicoPinMap.hpp"
#include "systems/generation/ProjectGenerator.hpp"
#include "systems/project/ProjectStore.hpp"
#include "systems/mainwindow/MainWindow.hpp"
#include "systems/mainwindow/workflow/Workflow.hpp"
#include "systems/mainwindow/viewer/Viewer.hpp"
#include "systems/mainwindow/project/project_column2/ProjectColumn2.hpp"
#include "systems/mainwindow/project/project_column3/ProjectColumn3.hpp"
#include "systems/mainwindow/dialog/IFileDialogService.hpp"
#include "systems/mainwindow/function_selection/function_selection_column2/FunctionSelectionColumn2.hpp"
#include "systems/mainwindow/function_selection/function_selection_column3/FunctionSelectionColumn3.hpp"
#include "systems/mainwindow/settings/settings_column2/SettingsColumn2.hpp"
#include "systems/mainwindow/settings/settings_column3/SettingsColumn3.hpp"
#include "systems/mainwindow/code/code_column2/CodeColumn2.hpp"
#include "systems/mainwindow/code/code_column3/CodeColumn3.hpp"
#include "systems/mainwindow/pio/pio_column2/PioColumn2.hpp"
#include "systems/mainwindow/pio/pio_column3/PioColumn3.hpp"
#include "systems/mainwindow/generate/generate_column2/GenerateColumn2.hpp"
#include "systems/mainwindow/generate/generate_column3/GenerateColumn3.hpp"
#include "systems/mainwindow/build/build_column2/BuildColumn2.hpp"
#include "systems/mainwindow/build/build_column3/BuildColumn3.hpp"
#include "systems/mainwindow/transfer/transfer_column2/TransferColumn2.hpp"
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"
#include "systems/mainwindow/debug/debug_column2/DebugColumn2.hpp"
#include "systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>

namespace pvd
{
System::System(bool certificationDialogs, QObject* parent) : QObject(parent)
{
    /**Creates the application orchestrator and resolves the governing database.*/
    const QString systems = runtimeSystemsRoot();
    systemDb_ = std::make_unique<SystemDatabase>(QDir(systems).filePath("system.sqlite"));
    functionCatalog_ = std::make_unique<FunctionCatalog>(QDir(systems).filePath("components/pin_functions"));
    fileDialogService_ = makeFileDialogService(certificationDialogs);
    createComponents();
    connectComponents();
}
System::~System()
{
    /**Stops external debugger processes before Qt destroys the raw UI pointers.

    DebugColumn3 owns OpenOCD and GDB child processes, so the orchestrator closes
    them explicitly while the debug widget is still fully alive.
    */
    if (debug3_)
        debug3_->stopServer();
}
QString System::runtimeSystemsRoot() const
{
    /**Returns the CMake-copied runtime systems directory.*/
    return QDir(QStringLiteral(PVD_RUNTIME_ROOT)).filePath("src/systems");
}
QString System::db(const QString& componentId) const
{
    /**Resolves a UI component database exclusively through system.sqlite.*/
    return systemDb_->componentDatabase(componentId);
}
void System::createComponents()
{
    /**Creates global and workflow-specific objects while keeping each column independent.*/
    workflow_ = new Workflow(db("workflow"));
    viewer_ = new Viewer(db("viewer"));
    window_ = new MainWindow(workflow_, viewer_);
    window_->setCloseGuard([this]() { return confirmDiscardUnsavedChanges("Close Project"); });
    project2_ = new ProjectColumn2(db("project_column2"));
    project3_ = new ProjectColumn3(db("project_column3"), fileDialogService_.get());
    function2_ = new FunctionSelectionColumn2(db("function_selection_column2"));
    function3_ = new FunctionSelectionColumn3(db("function_selection_column3"), functionCatalog_.get());
    settings2_ = new SettingsColumn2(db("settings_column2"));
    settings3_ = new SettingsColumn3(db("settings_column3"), functionCatalog_.get());
    code2_ = new CodeColumn2(db("code_column2"));
    code3_ = new CodeColumn3(db("code_column3"));
    pio2_ = new PioColumn2(db("pio_column2"));
    pio3_ = new PioColumn3(db("pio_column3"));
    generate2_ = new GenerateColumn2(db("generate_column2"));
    generate3_ = new GenerateColumn3(db("generate_column3"));
    build2_ = new BuildColumn2(db("build_column2"));
    build3_ = new BuildColumn3(db("build_column3"));
    transfer2_ = new TransferColumn2(db("transfer_column2"));
    transfer3_ = new TransferColumn3(db("transfer_column3"));
    debug2_ = new DebugColumn2(db("debug_column2"));
    debug3_ = new DebugColumn3(db("debug_column3"));
    window_->registerWorkflow("project", project2_, project3_);
    window_->registerWorkflow("function_selection", function2_, function3_);
    window_->registerWorkflow("settings", settings2_, settings3_);
    window_->registerWorkflow("code", code2_, code3_);
    window_->registerWorkflow("pio", pio2_, pio3_);
    window_->registerWorkflow("generate", generate2_, generate3_);
    window_->registerWorkflow("build", build2_, build3_);
    window_->registerWorkflow("transfer", transfer2_, transfer3_);
    window_->registerWorkflow("debug", debug2_, debug3_);
    window_->activateWorkflow("project");
    pio3_->selectProgram("Mogge");
}
void System::connectComponents()
{
    /**Owns all cross-component signal routing in the application.*/
    connect(viewer_, &Viewer::componentSelected, this,
            [this](const QString& id) { window_->activateWorkflow("function_selection"); });
    connect(workflow_, &Workflow::workflowSelected, window_, &MainWindow::activateWorkflow);
    connect(function2_, &FunctionSelectionColumn2::componentSelected, this,
            [this](const QString& id)
            {
                function3_->setComponent(id);
                viewer_->selectComponent(id);
                if (state_.selections.contains(id))
                    function3_->setSelectedFunction(state_.selections[id].functionId);
            });
    connect(viewer_, &Viewer::componentSelected, this, [this](const QString& id) { function2_->selectComponent(id); });
    connect(function3_, &FunctionSelectionColumn3::functionChanged, this, &System::handleFunctionChange);
    connect(settings2_, &SettingsColumn2::selectionRequested, this,
            [this](const QString& id)
            {
                if (state_.selections.contains(id))
                    settings3_->showSelection(state_.selections[id]);
            });
    connect(settings3_, &SettingsColumn3::settingChanged, this,
            [this](const QString& id, const QString& key, const QString& value)
            {
                if (state_.selections.contains(id))
                {
                    state_.selections[id].settings[key] = value;
                    markProjectDirty();
                    latestBuildSuccessful_ = false;
                    if (key == "direction" || key == "operation_mode")
                    {
                        // Rebuild SettingsColumn3 only after the authoritative project state
                        // has been updated. The widget may have queued a transient rebuild
                        // from its pre-model snapshot; this queued refresh is model-owned.
                        const QString componentId = id;
                        QMetaObject::invokeMethod(
                            settings3_,
                            [this, componentId]()
                            {
                                if (state_.selections.contains(componentId) &&
                                    settings3_->currentComponentId() == componentId)
                                    settings3_->showSelection(state_.selections.value(componentId));
                            },
                            Qt::QueuedConnection);
                    }
                    if (id == "rp2350a" && (key == "core1_enabled" || key == "enabled"))
                    {
                        const auto& rp = state_.selections[id];
                        settings3_->setCore1Enabled(rp.settings.value("enabled", "true") == "true" &&
                                                    rp.settings.value("core1_enabled", "false") == "true");
                        settings3_->showSelection(rp);
                    }
                    if (id == "debug_probe")
                        debug2_->applySetting(key, value);
                }
            });
    connect(pio3_, &PioColumn3::programsChanged, this,
            [this]()
            {
                latestBuildSuccessful_ = false;
                pio2_->setPrograms(pio3_->programs().keys());
            });
    connect(project2_, &ProjectColumn2::languageChanged, this,
            [this](const QString& value)
            {
                state_.language = value;
                markProjectDirty();
                latestBuildSuccessful_ = false;
            });
    connect(project2_, &ProjectColumn2::stateChanged, this,
            [this](const QString& value)
            {
                state_.state = value;
                latestBuildSuccessful_ = false;
                markProjectDirty();
            });
    connect(project2_, &ProjectColumn2::productChanged, this,
            [this](const QString& value)
            {
                state_.product = value;
                markProjectDirty();
            });
    connect(project2_, &ProjectColumn2::debugSessionToolsChanged, this,
            [this](bool value)
            {
                state_.debugSessionTools = value;
                latestBuildSuccessful_ = false;
                markProjectDirty();
            });
    connect(project2_, &ProjectColumn2::runtimeDiagnosticsChanged, this,
            [this](bool value)
            {
                state_.runtimeDiagnostics = value;
                markProjectDirty();
            });
    connect(project2_, &ProjectColumn2::verboseBuildEvidenceChanged, this,
            [this](bool value)
            {
                state_.verboseBuildEvidence = value;
                markProjectDirty();
            });
    connect(project3_, &ProjectColumn3::createRequested, this,
            [this](const QString& name, const QString& path)
            {
                if (!confirmDiscardUnsavedChanges("Create Project"))
                    return;
                state_.projectName = name.trimmed().isEmpty() ? "PICO2W" : name;
                state_.projectPath = path;
                latestBuildSuccessful_ = false;
                lastBuiltProjectPath_.clear();
                markProjectDirty();
                if (saveProjectState("Project saved in: " + path))
                    refreshProjectViews();
            });
    connect(project3_, &ProjectColumn3::projectNameChanged, this,
            [this](const QString& name)
            {
                if (!state_.projectPath.isEmpty() && name != state_.projectName)
                {
                    state_.projectName = name;
                    markProjectDirty();
                }
            });
    connect(project3_, &ProjectColumn3::openRequested, this,
            [this](const QString& path)
            {
                if (!confirmDiscardUnsavedChanges("Open Project"))
                    return;
                latestBuildSuccessful_ = false;
                lastBuiltProjectPath_.clear();
                QString error;
                if (!ProjectStore::load(path, &state_, &error))
                {
                    QMessageBox::warning(window_, "Open Project", error);
                    project3_->setStatus("Open failed: " + error, false);
                    return;
                }
                projectDirty_.markLoaded();
                project3_->setDirty(false);
                refreshProjectViews();
            });
    connect(project3_, &ProjectColumn3::saveRequested, this,
            [this](const QString& name, const QString& path)
            {
                if (state_.projectName != name || QDir(state_.projectPath).absolutePath() != QDir(path).absolutePath())
                {
                    latestBuildSuccessful_ = false;
                    lastBuiltProjectPath_.clear();
                }
                state_.projectName = name;
                state_.projectPath = path;
                markProjectDirty();
                if (saveProjectState("Project saved in: " + path))
                    refreshProjectViews();
            });
    connect(generate3_, &GenerateColumn3::validateRequested, this,
            [this]()
            {
                QString report;
                const bool ok = ProjectGenerator::validate(state_, &report);
                generate3_->setStatus(report, ok);
            });
    connect(generate3_, &GenerateColumn3::generateRequested, this,
            [this]()
            {
                latestBuildSuccessful_ = false;
                QString error;
                if (ProjectGenerator::generate(&state_, pio3_->programs(), &error))
                {
                    pio3_->reloadGeneratedFiles(state_.generatedFiles);
                    generate3_->setStatus(
                        QString("Ready for Build — Generation completed: %1 files").arg(state_.generatedFiles.size()),
                        true);
                    refreshProjectViews();
                }
                else
                    generate3_->setStatus(error, false);
            });
    connect(code2_, &CodeColumn2::fileSelected, code3_, &CodeColumn3::loadFile);
    connect(code3_, &CodeColumn3::buildRequested, this,
            [this]()
            {
                const QString source = QDir(state_.projectPath).filePath("generated"),
                              build = QDir(state_.projectPath).filePath("build");
                build2_->setProjectPath(state_.projectPath);
                build3_->setPaths(source, build);
                const bool debugBuild = state_.state != "Release";
                build3_->setBuildOptions(debugBuild, debugBuild && state_.debugSessionTools,
                                         state_.verboseBuildEvidence);
                build3_->build();
            });
    connect(build3_, &BuildColumn3::buildStarted, this, [this]() { latestBuildSuccessful_ = false; });
    connect(build3_, &BuildColumn3::buildCompleted, this,
            [this](bool ok, const QString& output)
            {
                latestBuildSuccessful_ = ok;
                lastBuiltProjectPath_ = ok ? QDir(state_.projectPath).canonicalPath() : QString{};
                code3_->setBuildResult(ok, output);
            });
    connect(pio2_, &PioColumn2::programSelected, pio3_, &PioColumn3::selectProgram);
    connect(workflow_, &Workflow::workflowSelected, this,
            [this](const QString& id)
            {
                if (id == "build")
                {
                    const QString source = QDir(state_.projectPath).filePath("generated"),
                                  build = QDir(state_.projectPath).filePath("build");
                    build2_->setProjectPath(state_.projectPath);
                    build3_->setPaths(source, build);
                    const bool debugBuild = state_.state != "Release";
                    build3_->setBuildOptions(debugBuild, debugBuild && state_.debugSessionTools,
                                             state_.verboseBuildEvidence);
                }
                else if (id == "transfer")
                {
                    const QString generated = QDir(state_.projectPath).filePath("generated"),
                                  build = QDir(state_.projectPath).filePath("build");
                    QString target = state_.projectName;
                    target.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");
                    if (target.isEmpty())
                        target = "PICO2W";
                    transfer2_->setBuildPath(build, target);
                    const bool currentBuild =
                        latestBuildSuccessful_ && lastBuiltProjectPath_ == QDir(state_.projectPath).canonicalPath();
                    transfer3_->configure(transfer2_->artifact(), transfer2_->method(), generated, build, target,
                                          currentBuild);
                }
                else if (id == "debug")
                {
                    applyDebugProbeSettings();
                    const auto rp = state_.selections.value("rp2350a");
                    debug3_->setAvailableCores(rp.functionId == "rp2350a.configure" &&
                                               rp.settings.value("enabled", "true") == "true" &&
                                               rp.settings.value("core1_enabled", "false") == "true");
                    QString target = state_.projectName;
                    target.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");
                    if (target.isEmpty())
                        target = "PICO2W";
                    const QString elf = QDir(QDir(state_.projectPath).filePath("build")).filePath(target + ".elf");
                    debug3_->configure(debug2_->openocd(), debug2_->gdb(), debug2_->interfaceCfg(),
                                       debug2_->targetCfg(), elf, debug2_->speed(), debug2_->resetMethod(), false);
                }
            });
}
void System::handleFunctionChange(const QString& componentId, const FunctionOption& option, int gpio, int physicalPin)
{
    /**Converts a UI function choice into project state and coordinates dependent components.*/
    latestBuildSuccessful_ = false;
    if (option.id == "disabled")
    {
        state_.selections.remove(componentId);
    }
    else
    {
        FunctionSelection s = state_.selections.value(componentId);
        s.componentId = componentId;
        s.displayName = componentDisplayName(componentId);
        s.physicalPin = physicalPin;
        s.gpio = gpio;
        s.functionId = option.id;
        s.functionName = option.name;
        if (option.id == "sio")
        {
            s.settings.insert("direction", s.settings.value("direction", "Input"));
            s.settings.insert("pull", s.settings.value("pull", "None"));
        }
        state_.selections[componentId] = s;
    }
    markProjectDirty();
    settings2_->refresh(state_);
    viewer_->setSimulationText(option.id == "disabled" ? "Disabled: " + componentDisplayName(componentId)
                                                       : componentDisplayName(componentId) + " — " + option.name);
}
void System::refreshProjectViews()
{
    /**Refreshes all presentation objects that derive from active project state.*/
    project3_->setProject(state_.projectName, state_.projectPath);
    const auto rp = state_.selections.value("rp2350a");
    settings3_->setCore1Enabled(rp.functionId == "rp2350a.configure" &&
                                rp.settings.value("enabled", "true") == "true" &&
                                rp.settings.value("core1_enabled", "false") == "true");
    debug3_->setAvailableCores(rp.functionId == "rp2350a.configure" && rp.settings.value("enabled", "true") == "true" &&
                               rp.settings.value("core1_enabled", "false") == "true");
    project2_->setProjectState(state_.product, state_.language, state_.state, state_.debugSessionTools,
                               state_.runtimeDiagnostics, state_.verboseBuildEvidence);
    settings2_->refresh(state_);
    generate2_->setFiles(state_.generatedFiles);
    QStringList codeFiles;
    for (const auto& file : state_.generatedFiles)
    {
        const QString suffix = QFileInfo(file).suffix().toLower();
        // The PIO editor owns .pio source files; all other generated
        // project files belong in the general code/file editor.
        if (suffix != "pio")
            codeFiles << file;
    }
    code2_->setFiles(codeFiles);
    pio2_->setPrograms(pio3_->programs().keys());
    build2_->setProjectPath(state_.projectPath);
    const bool debugBuild = state_.state != "Release";
    build3_->setBuildOptions(debugBuild, debugBuild && state_.debugSessionTools, state_.verboseBuildEvidence);
    applyDebugProbeSettings();
    project3_->setDirty(projectDirty_.isDirty());
}
void System::applyDebugProbeSettings()
{
    /**Applies the persisted debug-probe selection to the live debug configuration.

    The project state remains authoritative while DebugColumn2 owns tool discovery
    and presentation-level defaults.
    */
    const auto selection = state_.selections.value("debug_probe");
    for (auto it = selection.settings.cbegin(); it != selection.settings.cend(); ++it)
        debug2_->applySetting(it.key(), it.value());
}
void System::markProjectDirty()
{
    projectDirty_.markDirty();
    if (project3_)
        project3_->setDirty(true);
}
bool System::saveProjectState(const QString& successMessage)
{
    QString error;
    if (!ProjectStore::save(state_, &error))
    {
        projectDirty_.markDirty();
        if (project3_)
            project3_->setStatus("Save failed: " + error, false);
        if (window_)
            QMessageBox::warning(window_, "Save Project", error);
        return false;
    }
    projectDirty_.markSaved();
    if (project3_)
        project3_->setStatus(successMessage.isEmpty() ? "Project saved." : successMessage, true);
    return true;
}
bool System::confirmDiscardUnsavedChanges(const QString& operation)
{
    if (!projectDirty_.isDirty())
        return true;
    const auto answer =
        QMessageBox::warning(window_, operation, "The project has unsaved changes.",
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save)
        return saveProjectState();
    return answer == QMessageBox::Discard;
}
void System::run()
{
    /**Shows the reconstructed application shell after all routing is established.*/
    refreshProjectViews();
    window_->show();
}
} // namespace pvd
