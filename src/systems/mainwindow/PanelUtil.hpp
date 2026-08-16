// PanelUtil.hpp
#pragma once
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace pvd {
inline QLabel* makePanelTitle(const QString& text, QWidget* parent)
{
    /**Creates one consistent bold panel heading.*/
    auto* label = new QLabel(text, parent);
    QFont font = label->font(); font.setBold(true); label->setFont(font);
    return label;
}
inline QVBoxLayout* makePanelLayout(QWidget* parent, const QString& title)
{
    /**Creates the standard compact layout used by MainWindow components.*/
    auto* layout = new QVBoxLayout(parent); layout->setContentsMargins(8,8,8,8); layout->setSpacing(6);
    layout->addWidget(makePanelTitle(title,parent));
    return layout;
}
}
