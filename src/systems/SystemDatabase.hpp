// SystemDatabase.hpp
#pragma once
#include <QString>
#include <QStringList>

namespace pvd {
class SystemDatabase final
{
public:
    explicit SystemDatabase(QString path);
    QString componentDatabase(const QString& componentId) const;
    QStringList workflowOrder() const;
private:
    QString path_;
};
}
