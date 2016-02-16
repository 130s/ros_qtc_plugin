/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    ROSProject(Manager *manager, const QString &filename);
    ~ROSProject() override;

    QString displayName() const override;
    Manager *projectManager() const override;

    QStringList files(FilesMode fileMode) const override;

    QStringList buildTargets() const;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    bool addIncludes(const QStringList &includePaths);
    bool setIncludes(const QStringList &includePaths);

    enum UpdateOptions
    {
        Files        = 0x01,
        IncludePaths = 0x02,
    };

    void refresh();

    QStringList projectIncludePaths() const;
    QStringList files() const;

    Utils::FileName buildDirectory() const;
    Utils::FileName sourceDirectory() const;

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);

private:
    bool saveRawList(const QStringList &rawList, const ROSProject::UpdateOptions &updateOption);
    void parseProject();
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    void refreshCppCodeModel();

    QString m_projectName;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_rawProjectIncludePaths;
    QStringList m_projectIncludePaths;
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
