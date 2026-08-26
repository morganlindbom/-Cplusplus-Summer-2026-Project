// BuildColumn3.hpp
#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QProcess;

namespace pvd
{
class BuildColumn3 final : public QWidget
{
    Q_OBJECT

  public:
    explicit BuildColumn3(const QString& db, QWidget* parent = nullptr);

    void setPaths(const QString& source, const QString& build);
    void setBuildOptions(bool debugBuild, bool deoptimizedDebug, bool verboseEvidence);
    void build();

  signals:
    void buildStarted();
    void buildCompleted(bool ok, const QString& output);

  private:
    enum class BuildWorkflowState
    {
        Idle,
        ConfigureRunning,
        ConfigureCompleted,
        ConfigureFailed,
        BuildRunning,
        BuildCompleted,
        BuildFailed
    };

    void configureProject();
    void run(const QString& program, const QStringList& args, bool isBuild);
    void setWorkflowState(BuildWorkflowState state);

    QString source_;
    QString build_;
    QLabel* status_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QProcess* process_ = nullptr;
    bool currentProcessIsBuild_ = false;
    bool buildCompletionReported_ = false;
    bool debugBuild_ = true;
    bool deoptimizedDebug_ = true;
    bool verboseEvidence_ = true;
};
} // namespace pvd
