QTC_SOURCE = $$(HOME)/qtc_source/qt-creator
QTC_BUILD = $$(HOME)/qtc_source/qt-creator-build

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$QTC_SOURCE

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$QTC_BUILD

include(ros_project_manager_dependencies.pri)
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

HEADERS = \
    ros_project_wizard.h \
    ros_project_plugin.h \
    ros_project_manager.h \
    ros_project.h \
    ros_project_constants.h \
    ros_make_step.h \
    ros_build_configuration.h \
    ros_project_nodes.h \
    ros_utils.h

SOURCES = \
    ros_project_wizard.cpp \
    ros_project_plugin.cpp \
    ros_project_manager.cpp \
    ros_project.cpp \
    ros_make_step.cpp \
    ros_build_configuration.cpp \
    ros_project_nodes.cpp \
    ros_utils.cpp

RESOURCES += ros_project.qrc
FORMS += ros_make_step.ui \
    ros_build_configuration.ui \
    ros_import_wizard_page.ui
