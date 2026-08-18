//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef CMAKEGENERATOR_H
#define CMAKEGENERATOR_H

#include <QList>
#include <QString>
#include "buildmodel.h"

enum CMakeQtVersion
{
    CMakeQtVersion_Qt4,
    CMakeQtVersion_Qt5,
    CMakeQtVersion_Qt6,
    CMakeQtVersion_All
};

class CMakeOption
{
    public:
        CMakeOption(QString name, QString description, QString __default);
        QString Name;
        QString Description;
        QString Default;
};

class CMakeGenerator
{
    public:
        CMakeGenerator(CMakeQtVersion version);
        QString Generate(const BuildProject &project, const QList<CMakeOption> &options);

    private:
        QString GenerateOptions(const QList<CMakeOption> &options);
        QString GenerateFileSet(QString variable, const QList<QString> &files);
        QString GenerateDefaultQtLibs(const BuildTarget &target);
        QString GenerateQt4Libs();
        QString GenerateQt5Libs(const BuildTarget &target);
        QString GenerateQt6Libs(const BuildTarget &target);
        QString GenerateQtAutomation(const BuildTarget &target);
        QString GenerateQtModules(const BuildTarget &target);
        QString GenerateConfigOptions(const BuildTarget &target);
        QString GenerateDefines(const BuildTarget &target);
        QString GenerateIncludePaths(const BuildTarget &target);
        QString GenerateLibraries(const BuildTarget &target);
        QString GenerateCompileOptions(const BuildTarget &target);
        QString GenerateLinkOptions(const BuildTarget &target);
        QString GenerateUIFiles(const BuildTarget &target);
        QString GenerateResources(const BuildTarget &target);
        QString GenerateTranslations(const BuildTarget &target);
        QString GenerateInstallRules(const BuildTarget &target);
        QString GenerateSubdirs(const BuildProject &project, const BuildTarget &target);
        QString GenerateConditionalScopes(const BuildTarget &target);
        QString QtComponentName(QString module) const;
        QString QtTargetName(QString module) const;

        CMakeQtVersion Version;
};

#endif // CMAKEGENERATOR_H
