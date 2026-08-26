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
    status_ = new QLabel("Status: Idle", this);
    status_->setObjectName("generate_status");
    status_->setAccessibleName("Generate status");
    status_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    status_->setWordWrap(true);
    status_->setMinimumHeight(28);
    status_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    status_->setStyleSheet("font-weight:600; color:#333333;");
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
void GenerateColumn3::setWorkflowState(GenerateWorkflowState state)
{
    workflowState_ = state;
    switch (state)
    {
    case GenerateWorkflowState::Idle: setStatus("Status: Idle", true); break;
    case GenerateWorkflowState::Running: setStatus(QString::fromUtf8("Status: Generating…"), true); break;
    case GenerateWorkflowState::Completed: setStatus("Status: Generate completed", true); break;
    case GenerateWorkflowState::Failed: setStatus("Status: Generate failed", false); break;
    }
}
void GenerateColumn3::setStatus(const QString& text, bool ok)
{ /**Displays validation/generation feedback.*/
    status_->setText(text);
    status_->setStyleSheet(ok ? "color:#126b2f;" : "color:#a32626;");
}
} // namespace pvd
