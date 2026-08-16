// DebugColumn2.hpp
#pragma once
#include <QWidget>
class QLineEdit; class QComboBox;
namespace pvd { class DebugColumn2 final:public QWidget{Q_OBJECT public:explicit DebugColumn2(const QString& db,QWidget* parent=nullptr);QString openocd()const;QString interfaceCfg()const;QString targetCfg()const;int speed()const;QString resetMethod()const;void applySetting(const QString& key,const QString& value);private:QLineEdit* openocd_=nullptr;QLineEdit* interface_=nullptr;QLineEdit* target_=nullptr;QComboBox* speed_=nullptr;QString resetMethod_="run reset";}; }
