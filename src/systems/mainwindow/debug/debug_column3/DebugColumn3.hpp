// DebugColumn3.hpp
#pragma once

#include <QStringList>
#include <QWidget>
#include <QHash>
#include <QQueue>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTimer;
class QComboBox;

namespace pvd
{
class DebugColumn3 final : public QWidget
{
    Q_OBJECT

  public:
    explicit DebugColumn3(const QString& db, QWidget* parent = nullptr);
    ~DebugColumn3() override;

    void configure(const QString& openocdExe, const QString& gdbExe, const QString& interfaceCfg,
                   const QString& targetCfg, const QString& elfPath, int speed,
                   const QString& resetMethod = "run reset", bool autoStart = false);
    void startSession();
    void startReadbackSession();
    void stopServer();
    void setAvailableCores(bool core1Enabled);

  private:
    enum class DebugCore
    {
        Core0,
        Core1
    };

    void appendLog(const QString& text);
    void setStatus(const QString& text);
    void interpretOpenOcdOutput(const QString& output);
    void interpretGdbOutput(const QString& output);
    [[nodiscard]] bool validateConfiguration(QString* error) const;
    void startOpenOcd();
    void probeOpenOcdReadiness();
    void startGdb();
    void sendGdbStartupCommands();
    void resolveCoreThreadMapping();
    void selectDebugCore(DebugCore core);
    void updateSelectedCoreFromThread(int threadId);
    void resetCoreSelection();
    void setCoreSelectorEnabled(bool enabled);
    [[nodiscard]] int threadForCore(DebugCore core) const;
    [[nodiscard]] QString coreName(DebugCore core) const;
    [[nodiscard]] QString coreTargetName(DebugCore core) const;
    void setSessionButtons(bool running);
    void sendGdbCommand(const QString& command, bool echo = true);
    void haltTarget();
    void startReadbackSynchronization();
    void startReadbackEvidence();
    void sendNextReadbackCommand();
    void finishReadback(bool success, const QString& reason = {});
    void consumeReadbackOutput(const QString& output);
    void consumeReadbackMiOutput(const QString& output);
    void sendReadbackMiCommand(const QString& command);
    void advanceReadbackMiStartup(const QString& resultClass, const QString& record);
    void writeReadbackResult(bool success, const QString& reason);
    [[nodiscard]] QString resetMonitorCommand() const;
    void stopGdb();
    void stopOpenOcd();
    void markReadbackCleanupComplete();
    [[nodiscard]] bool sendOpenOcdCommands(const QStringList& commands, int connectTimeoutMs = 500);

    QString openocdExe_;
    QString gdbExe_;
    QString interface_;
    QString target_;
    QString elfPath_;
    QString resetMethod_ = "run reset";
    int speed_ = 5000;
    int readinessAttempts_ = 0;
    bool stopping_ = false;
    bool gdbStartupCommandsSent_ = false;
    bool openocdGdbServerReady_ = false;
    bool core1Enabled_ = false;
    bool readbackOnly_ = false;
    bool coreThreadMappingResolved_ = false;
    DebugCore selectedCore_ = DebugCore::Core0;
    QHash<int, DebugCore> coreThreadMapping_;
    QString gdbOutputBuffer_;

    QLabel* status_ = nullptr;
    QLabel* coreLabel_ = nullptr;
    QComboBox* coreSelector_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* readbackStartButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QLineEdit* command_ = nullptr;
    QPushButton* sendCommand_ = nullptr;
    QProcess* openocdProcess_ = nullptr;
    QProcess* gdbProcess_ = nullptr;
    QTimer* readinessTimer_ = nullptr;
    QTimer* readbackDeadlineTimer_ = nullptr;
    QLabel* readbackStatus_ = nullptr;
    QLabel* readbackResultPath_ = nullptr;
    QQueue<QString> readbackCommands_;
    QString readbackRunId_;
    QString readbackRequestPath_;
    QString readbackAckPath_;
    QString readbackLogPath_;
    QString readbackResultPathValue_;
    QString readbackResponseBuffer_;
    QString readbackPrompt_;
    QString readbackSyncToken_;
    QString readbackStage_;
    QString readbackTranscript_;
    QString readbackLastCommand_;
    QString readbackLastResponse_;
    int readbackCommandId_ = 0;
    bool readbackEvidenceActive_ = false;
    bool readbackEvidenceCompleted_ = false;
    bool readbackRequestAccepted_ = false;
    bool readbackOpenOcdReady_ = false;
    bool readbackGdbConnected_ = false;
    bool readbackTargetHalted_ = false;
    bool readbackEvidenceReadStarted_ = false;
    bool readbackSyncActive_ = false;
    bool readbackSyncPromptSeen_ = false;
    bool readbackSyncTokenSeen_ = false;
    bool readbackMiMode_ = false;
    int readbackMiToken_ = 100;
    int readbackMiPendingToken_ = 0;
    int readbackMiFirstEvidenceToken_ = 0;
    QString readbackMiPendingCommand_;
    QString readbackMiStartupPhase_;
    QString readbackMiBuffer_;
    int readbackMiAsyncRecordCount_ = 0;
    int readbackMiForeignTokenCount_ = 0;
    int readbackMiParseFailureCount_ = 0;
};
} // namespace pvd
