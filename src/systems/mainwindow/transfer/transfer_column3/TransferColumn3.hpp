// TransferColumn3.hpp
#pragma once

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;

namespace pvd
{
class TransferColumn3 final : public QWidget
{
    Q_OBJECT

  public:
    explicit TransferColumn3(const QString& db, QWidget* parent = nullptr);

    [[nodiscard]] static bool postProgramStartStatesValid(const QString& core0State,
                                                           const QString& core1State);

    void configure(const QString& artifact, const QString& method, const QString& generatedDirectory,
                   const QString& buildDirectory, const QString& expectedTarget, bool latestBuildSuccessful);

  private:
    [[nodiscard]] bool artifactIsCurrent(QString* reason) const;
    [[nodiscard]] QString currentBootselDrive() const;
    void setStatus(const QString& text);
    void startTransfer();
    void finishOpenOcdTransfer(int exitCode, QProcess::ExitStatus exitStatus);

    QString artifact_;
    QString method_;
    QString generatedDirectory_;
    QString buildDirectory_;
    QString expectedTarget_;
    QLabel* status_ = nullptr;
    QLineEdit* drive_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QProcess* process_ = nullptr;
    bool openOcdTransferActive_ = false;
    QString transferOutput_;
};
} // namespace pvd
