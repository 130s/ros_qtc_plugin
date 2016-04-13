#ifndef ROSPROJECT_H
#define ROSPROJECT_H

#include "ros_project_manager.h"
#include "ros_project_nodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>
#include <QFileSystemWatcher>

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    ROSProject(ROSManager *manager, const QString &filename);
    ~ROSProject() override;

    QString displayName() const override;
    ROSManager *projectManager() const override;

    QStringList files(FilesMode fileMode) const override;

    QStringList buildTargets() const;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setWorkspaceFiles(const QHash<QString, QStringList> &workspaceFiles);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    bool addIncludes(const QStringList &includePaths);
    bool setIncludes(const QStringList &includePaths);

    void refresh();

    QStringList projectIncludePaths() const;
    QHash<QString, QStringList> workspaceFiles() const;

    Utils::FileName buildDirectory() const;
    Utils::FileName sourceDirectory() const;

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);

private:
    bool saveWorkspaceFiles(const QHash<QString, QStringList> &workspaceFiles);
    bool saveProjectIncludePaths();
    void parseProject();
    void refreshCppCodeModel();

    QString m_projectName;
    QHash<QString, QStringList> m_workspaceFiles;
    QStringList m_workspaceFileList;
    QStringList m_projectIncludePaths;
    QFileSystemWatcher watcher;
    QFuture<void> m_codeModelFuture;
};

class ROSProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    ROSProjectFile(ROSProject *parent, QString fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave) override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    ROSProject *m_project;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
