// DebugColumn2.hpp
#pragma once

#include <QString>
#include <QWidget>

class QComboBox;
class QLineEdit;

namespace pvd
{
class DebugColumn2 final : public QWidget
{
    Q_OBJECT

  public:
    explicit DebugColumn2(const QString& db, QWidget* parent = nullptr);

    [[nodiscard]] QString openocd() const;
    [[nodiscard]] QString gdb() const;
    [[nodiscard]] QString interfaceCfg() const;
    [[nodiscard]] QString targetCfg() const;
    [[nodiscard]] int speed() const;
    [[nodiscard]] QString resetMethod() const;

    void applySetting(const QString& key, const QString& value);

  private:
    QLineEdit* openocd_ = nullptr;
    QLineEdit* gdb_ = nullptr;
    QLineEdit* interface_ = nullptr;
    QLineEdit* target_ = nullptr;
    QComboBox* speed_ = nullptr;
    QString resetMethod_ = "run reset";
};
} // namespace pvd
