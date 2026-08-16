// FunctionCatalog.hpp
#pragma once
#include <QString>
#include <QVector>
namespace pvd {
struct FunctionOption { QString id; QString name; QString category; QString description; QString databasePath; };
class FunctionCatalog final
{
public:
    explicit FunctionCatalog(QString rootDirectory);
    QVector<FunctionOption> functionsForGpio(int gpio) const;
    QVector<FunctionOption> specialFunctions(const QString& componentId) const;
    QString functionDatabase(const QString& functionId) const;
private:
    QString root_;
};
}
