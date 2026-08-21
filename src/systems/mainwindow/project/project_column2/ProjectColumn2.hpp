// ProjectColumn2.hpp
#pragma once
#include <QWidget>
class QComboBox;
class QCheckBox;
namespace pvd
{
    class ProjectColumn2 final : public QWidget
    {
    Q_OBJECT public : explicit ProjectColumn2(const QString &db, QWidget *parent = nullptr);
        QString language() const;
        QString stateName() const;
        QString product() const;
        bool debugSessionTools() const;
        bool runtimeDiagnostics() const;
        bool verboseBuildEvidence() const;
        void setProjectState(const QString& product, const QString& language, const QString& state,
                             bool debugSessionTools, bool runtimeDiagnostics, bool verboseBuildEvidence);
    signals:
        void productChanged(const QString &);
        void languageChanged(const QString &);
        void stateChanged(const QString &);
        void debugSessionToolsChanged(bool);
        void runtimeDiagnosticsChanged(bool);
        void verboseBuildEvidenceChanged(bool);

    private:
        QComboBox *product_ = nullptr;
        QComboBox *language_ = nullptr;
        QComboBox *state_ = nullptr;
        QCheckBox *debugSessionTools_ = nullptr;
        QCheckBox *runtimeDiagnostics_ = nullptr;
        QCheckBox *verboseBuildEvidence_ = nullptr;
    };
}
