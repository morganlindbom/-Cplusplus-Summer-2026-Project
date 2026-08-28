// TransferColumn2.hpp
#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;

namespace pvd
{
class TransferColumn2 final : public QWidget
{
    Q_OBJECT

  public:
    explicit TransferColumn2(const QString& db, QWidget* parent = nullptr);

    void setBuildPath(const QString& path, const QString& targetName);
    [[nodiscard]] QString artifact() const;
    [[nodiscard]] QString method() const;

  private:
    QLineEdit* artifact_ = nullptr;
    QComboBox* method_ = nullptr;
};
} // namespace pvd
