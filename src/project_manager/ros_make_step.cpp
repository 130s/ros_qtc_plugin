/**
 * @author Levi Armstrong
 * @date January 1, 2016
 *
 * @copyright Copyright (c) 2016, Southwest Research Institute
 *
 * @license Software License Agreement (Apache License)\n
 * \n
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 * \n
 * http://www.apache.org/licenses/LICENSE-2.0\n
 * \n
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ros_make_step.h"
#include "ros_project_constants.h"
#include "ros_project.h"
#include "ui_ros_make_step.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

const char ROS_MS_ID[] = "ROSProjectManager.ROSMakeStep";
const char ROS_MS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("ROSProjectManager::Internal::ROSMakeStep",
                                                     "catkin_make");

const char BUILD_TARGETS_KEY[] = "ROSProjectManager.ROSMakeStep.BuildTargets";
const char MAKE_ARGUMENTS_KEY[] = "ROSProjectManager.ROSMakeStep.MakeArguments";
const char MAKE_COMMAND_KEY[] = "ROSProjectManager.ROSMakeStep.MakeCommand";
const char CLEAN_KEY[] = "ROSProjectManager.ROSMakeStep.Clean";

ROSMakeStep::ROSMakeStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(ROS_MS_ID)),
    m_clean(false)
{
    ctor();
}

ROSMakeStep::ROSMakeStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id),
    m_clean(false)
{
    ctor();
}

ROSMakeStep::ROSMakeStep(BuildStepList *parent, ROSMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand),
    m_clean(bs->m_clean)
{
    ctor();
}

void ROSMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("ROSProjectManager::Internal::ROSMakeStep",
                                                      ROS_MS_DISPLAY_NAME));

    m_percentProgress = QRegExp(QLatin1String("\\[\\s{0,2}(\\d{1,3})%\\]")); // Example: [ 82%] [ 82%] [ 87%]
}

ROSBuildConfiguration *ROSMakeStep::rosBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(buildConfiguration());
}

ROSBuildConfiguration *ROSMakeStep::targetsActiveBuildConfiguration() const
{
    return static_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
}

ROSMakeStep::~ROSMakeStep()
{
}

bool ROSMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    ROSBuildConfiguration *bc = rosBuildConfiguration();
    if (!bc)
        bc = targetsActiveBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    pp->setEnvironment(env);
    pp->setCommand(makeCommand());
    pp->setArguments(allArguments(bc->initialArguments()));
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void ROSMakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool ROSMakeStep::isClean() const
{
    return m_clean;
}

QVariantMap ROSMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_makeArguments);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCommand);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    return map;
}

bool ROSMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

    return BuildStep::fromMap(map);
}

QString ROSMakeStep::allArguments(QString initial_arguments) const
{
    QString args = m_makeArguments;
    args.prepend(initial_arguments + QLatin1Char(' '));

    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString ROSMakeStep::makeCommand() const
{
    QString command = m_makeCommand;
    if (command.isEmpty())
    {
      command = QLatin1String("catkin_make");
    }

    return command;
}

void ROSMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

void ROSMakeStep::processStarted()
{
    futureInterface()->setProgressRange(0, 100);
    AbstractProcessStep::processStarted();
}

void ROSMakeStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    futureInterface()->setProgressValue(100);
}

void ROSMakeStep::stdOutput(const QString &line)
{
    AbstractProcessStep::stdOutput(line);
    int pos = 0;
    while ((pos = m_percentProgress.indexIn(line, pos)) != -1) {
        bool ok = false;
        int percent = m_percentProgress.cap(1).toInt(&ok);
        if (ok)
            futureInterface()->setProgressValue(percent);

        pos += m_percentProgress.matchedLength();
    }
}

BuildStepConfigWidget *ROSMakeStep::createConfigWidget()
{
    return new ROSMakeStepConfigWidget(this);
}

bool ROSMakeStep::immutable() const
{
    return false;
}

bool ROSMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void ROSMakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

//
// ROSMakeStepConfigWidget
//

ROSMakeStepConfigWidget::ROSMakeStepConfigWidget(ROSMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::ROSMakeStep;
    m_ui->setupUi(this);

    ROSProject *pro = static_cast<ROSProject *>(m_makeStep->target()->project());
    foreach (const QString &target, pro->buildTargets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    updateMakeOverrrideLabel();
    updateDetails();

    connect(m_ui->targetsList, &QListWidget::itemChanged,
            this, &ROSMakeStepConfigWidget::itemChanged);
    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
            this, &ROSMakeStepConfigWidget::makeLineEditTextEdited);
    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));

    connect(m_makeStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateMakeOverrrideLabel()));

    connect(pro, &ROSProject::environmentChanged,
            this, &ROSMakeStepConfigWidget::updateMakeOverrrideLabel);
    connect(pro, &ROSProject::environmentChanged,
            this, &ROSMakeStepConfigWidget::updateDetails);
}

ROSMakeStepConfigWidget::~ROSMakeStepConfigWidget()
{
    delete m_ui;
}

QString ROSMakeStepConfigWidget::displayName() const
{
    return tr("Make", "GenericMakestep display name.");
}

void ROSMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(m_makeStep->makeCommand())));
}

void ROSMakeStepConfigWidget::updateDetails()
{
    ROSBuildConfiguration *bc = m_makeStep->rosBuildConfiguration();
    if (!bc)
        bc = m_makeStep->targetsActiveBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments(bc->initialArguments()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString ROSMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ROSMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void ROSMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void ROSMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
    updateDetails();
}

//
// ROSMakeStepFactory
//

ROSMakeStepFactory::ROSMakeStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

BuildStep *ROSMakeStepFactory::create(BuildStepList *parent, const Id id)
{
    Q_UNUSED(id);
    ROSMakeStep *step = new ROSMakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        step->setBuildTarget(QLatin1String("all"), /* on = */ true);
    }
    return step;
}

BuildStep *ROSMakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    return new ROSMakeStep(parent, qobject_cast<ROSMakeStep *>(source));
}

BuildStep *ROSMakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    ROSMakeStep *bs(new ROSMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<ProjectExplorer::BuildStepInfo> ROSMakeStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->target()->project()->id() != Constants::ROSPROJECT_ID)
        return {};

    return {{ROS_MS_ID,  QCoreApplication::translate("ROSProjectManager::Internal::ROSMakeStep", ROS_MS_DISPLAY_NAME)}};
}

} // namespace Internal
} // namespace ROSProjectManager
