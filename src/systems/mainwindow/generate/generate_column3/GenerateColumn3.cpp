// GenerateColumn3.cpp
#include "systems/mainwindow/generate/generate_column3/GenerateColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QLabel>
#include <QPushButton>
namespace pvd
{
GenerateColumn3::GenerateColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{ /**Builds project generation status and controls.*/
    auto* l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Project Generation"));
    status_ = new QLabel("Ready to generate", this);
    status_->setObjectName("generate_status");
    status_->setWordWrap(true);
    l->addWidget(status_);
    auto* v = new QPushButton("Validate Project", this);
    v->setObjectName("validate_project");
    auto* g = new QPushButton("Generate Project", this);
    g->setObjectName("generate_project");
    l->addWidget(v);
    l->addWidget(g);
    l->addStretch();
    connect(v, &QPushButton::clicked, this, &GenerateColumn3::validateRequested);
    connect(g, &QPushButton::clicked, this, &GenerateColumn3::generateRequested);
}
void GenerateColumn3::setStatus(const QString& text, bool ok)
{ /**Displays validation/generation feedback.*/
    status_->setText(text);
    status_->setStyleSheet(ok ? "color:#126b2f;" : "color:#a32626;");
}
} // namespace pvd
