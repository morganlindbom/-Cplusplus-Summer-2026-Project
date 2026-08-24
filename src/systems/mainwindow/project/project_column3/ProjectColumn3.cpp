// ProjectColumn3.cpp
#include "systems/mainwindow/project/project_column3/ProjectColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFileDialog>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
namespace pvd
{
ProjectColumn3::ProjectColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds Project Management and Project Information areas.*/
    auto* l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Project Management"));
    auto* form = new QFormLayout();
    name_ = new QLineEdit("PICO2W", this);
    name_->setObjectName("project_name");
    path_ = new QLineEdit(this);
    path_->setObjectName("project_path");
    auto* browse = new QPushButton("Browse...", this);
    auto* row = new QWidget(this);
    auto* rh = new QHBoxLayout(row);
    rh->setContentsMargins(0, 0, 0, 0);
    rh->addWidget(path_, 1);
    rh->addWidget(browse);
    form->addRow("Project Name", name_);
    form->addRow("Project Path", row);
    l->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* create = new QPushButton("Create New Project", this);
    create->setObjectName("project_create");
    auto* open = new QPushButton("Open Existing Project", this);
    open->setObjectName("project_open");
    auto* save = new QPushButton("Save Project", this);
    save->setObjectName("project_save");
    buttons->addWidget(create);
    buttons->addWidget(open);
    buttons->addWidget(save);
    l->addLayout(buttons);

    l->addWidget(makePanelTitle("Update State Snapshot", this));
    auto* snapshot = new QPushButton("Create / Update State Snapshot", this);
    l->addWidget(snapshot);
    status_ = new QLabel("No project opened.", this);
    status_->setObjectName("project_dirty_status");
    status_->setWordWrap(true);
    l->addWidget(status_);
    l->addWidget(makePanelTitle("Project Information", this));
    auto* info = new QLabel("Create Project generates a complete project workspace. Function choices and settings are "
                            "persisted in a project-named SQLite database and remain isolated to the selected project.",
                            this);
    info->setWordWrap(true);
    l->addWidget(info);
    l->addStretch();

    connect(name_, &QLineEdit::textChanged, this, &ProjectColumn3::projectNameChanged);

    connect(browse, &QPushButton::clicked, this,
            [this]()
            {
                const auto p = QFileDialog::getExistingDirectory(this, "Project directory", path_->text());
                if (!p.isEmpty())
                    path_->setText(p);
            });
    connect(create, &QPushButton::clicked, this,
            [this]()
            {
                const QString projectName = name_->text().trimmed();
                if (projectName.isEmpty())
                {
                    status_->setText("Project name is required.");
                    return;
                }
                const QString projectPath = path_->text().trimmed();
                if (projectPath.isEmpty())
                {
                    status_->setText("Choose a project directory first.");
                    return;
                }
                emit createRequested(projectName, projectPath);
            });
    connect(open, &QPushButton::clicked, this,
            [this]()
            {
                const auto p = QFileDialog::getOpenFileName(this, "Open Pico Visual Designer project", path_->text(),
                                                            "Project databases (*.sqlite)");
                if (!p.isEmpty())
                    emit openRequested(p);
            });
    connect(save, &QPushButton::clicked, this,
            [this]()
            {
                const QString projectName = name_->text().trimmed();
                if (projectName.isEmpty())
                {
                    status_->setText("Project name is required.");
                    return;
                }
                QString projectPath = path_->text().trimmed();
                if (projectPath.isEmpty())
                {
                    projectPath = QFileDialog::getExistingDirectory(this, "Choose project directory");
                    if (projectPath.isEmpty())
                        return;
                    path_->setText(projectPath);
                }
                emit saveRequested(projectName, projectPath);
            });
    connect(
        snapshot, &QPushButton::clicked, this, [this]()
        { status_->setText("State snapshot request recorded. Save the project to persist current configuration."); });
}
void ProjectColumn3::setProject(const QString& name, const QString& path)
{
    /**Reflects the active project selected by System.*/
    name_->setText(name);
    path_->setText(path);
    status_->setText(path.isEmpty() ? "No project opened." : "Project active: " + name + "\n" + path);
}
void ProjectColumn3::setStatus(const QString& text, bool ok)
{
    status_->setText(text);
    status_->setStyleSheet(ok ? "color:#126b2f;" : "color:#a32626;");
}
void ProjectColumn3::setDirty(bool dirty)
{
    status_->setText(dirty ? "Unsaved project changes." : "Project saved.");
    status_->setStyleSheet(dirty ? "color:#8a5a00;" : "color:#126b2f;");
}
} // namespace pvd
