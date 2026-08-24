// System.hpp
#pragma once
#include "systems/project/ProjectDirtyState.hpp"
#include "systems/state/ApplicationState.hpp"
#include <QObject>
#include <memory>
namespace pvd
{
struct FunctionOption;
class SystemDatabase;
class FunctionCatalog;
class MainWindow;
class Workflow;
class Viewer;
class ProjectColumn2;
class ProjectColumn3;
class FunctionSelectionColumn2;
class FunctionSelectionColumn3;
class SettingsColumn2;
class SettingsColumn3;
class CodeColumn2;
class CodeColumn3;
class PioColumn2;
class PioColumn3;
class GenerateColumn2;
class GenerateColumn3;
class BuildColumn2;
class BuildColumn3;
class TransferColumn2;
class TransferColumn3;
class DebugColumn2;
class DebugColumn3;
class System final : public QObject
{
    Q_OBJECT
  public:
    explicit System(QObject* parent = nullptr);
    ~System() override;
    void run();

  private:
    QString db(const QString& componentId) const;
    void createComponents();
    void connectComponents();
    void refreshProjectViews();
    void applyDebugProbeSettings();
    void handleFunctionChange(const QString& componentId, const FunctionOption& option, int gpio, int physicalPin);
    void markProjectDirty();
    bool saveProjectState(const QString& successMessage = {});
    bool confirmDiscardUnsavedChanges(const QString& operation);
    QString runtimeSystemsRoot() const;
    ApplicationState state_;
    std::unique_ptr<SystemDatabase> systemDb_;
    std::unique_ptr<FunctionCatalog> functionCatalog_;
    MainWindow* window_ = nullptr;
    Workflow* workflow_ = nullptr;
    Viewer* viewer_ = nullptr;
    ProjectColumn2* project2_ = nullptr;
    ProjectColumn3* project3_ = nullptr;
    FunctionSelectionColumn2* function2_ = nullptr;
    FunctionSelectionColumn3* function3_ = nullptr;
    SettingsColumn2* settings2_ = nullptr;
    SettingsColumn3* settings3_ = nullptr;
    CodeColumn2* code2_ = nullptr;
    CodeColumn3* code3_ = nullptr;
    PioColumn2* pio2_ = nullptr;
    PioColumn3* pio3_ = nullptr;
    GenerateColumn2* generate2_ = nullptr;
    GenerateColumn3* generate3_ = nullptr;
    BuildColumn2* build2_ = nullptr;
    BuildColumn3* build3_ = nullptr;
    TransferColumn2* transfer2_ = nullptr;
    TransferColumn3* transfer3_ = nullptr;
    DebugColumn2* debug2_ = nullptr;
    DebugColumn3* debug3_ = nullptr;
    bool latestBuildSuccessful_ = false;
    QString lastBuiltProjectPath_;
    ProjectDirtyState projectDirty_;
};
} // namespace pvd
