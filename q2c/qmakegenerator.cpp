//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "qmakegenerator.h"
#include <QDateTime>
#include <QRegularExpression>

QString QMakeGenerator::Generate(const BuildProject &project)
{
    const BuildTarget *primary = project.PrimaryTarget();

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    foreach (QString warning, project.Warnings)
        source += "# q2c warning: " + warning + "\n";

    if (primary == nullptr)
    {
        source += "TARGET = " + project.Name + "\n";
        return source;
    }

    source += this->GenerateTarget(project, *primary);
    source += this->GenerateAdditionalTargetNotes(project, *primary);
    return source;
}

QString QMakeGenerator::GenerateTarget(const BuildProject &project, const BuildTarget &target)
{
    Q_UNUSED(project);
    QString target_name = target.Name;
    QString source;

    source += "TARGET = " + target_name + "\n";
    source += "TEMPLATE = " + this->ConfigForTarget(target) + "\n";

    source += this->GenerateAssignments(target);
    source += this->GenerateConditionalScopes(target);
    return source;
}

QString QMakeGenerator::GenerateAssignments(const BuildTarget &target)
{
    QString source;
    QList<QString> config = target.Config;
    if (target.Type == BuildTarget_Plugin && !config.contains("plugin"))
        config.append("plugin");
    if (target.Type == BuildTarget_Test && !config.contains("testcase"))
        config.append("testcase");

    source += this->Assignment("QT", target.QtModules);
    source += this->Assignment("CONFIG", config);
    source += this->Assignment("DEFINES", target.Defines);
    source += this->Assignment("INCLUDEPATH", target.IncludePaths);
    source += this->Assignment("SOURCES", target.Sources);
    source += this->Assignment("HEADERS", target.Headers);
    source += this->Assignment("FORMS", target.UiFiles);
    source += this->Assignment("RESOURCES", target.ResourceFiles);
    source += this->Assignment("TRANSLATIONS", target.TranslationFiles);
    source += this->Assignment("LIBS", this->LibrariesForQmake(target.Libraries));
    source += this->Assignment("QMAKE_CXXFLAGS", this->CompileOptionsForQmake(target.CompileOptions));
    source += this->Assignment("QMAKE_LFLAGS", this->LinkOptionsForQmake(target.LinkOptions));
    source += this->Assignment("INSTALLS", target.InstallRules);
    source += this->Assignment("SUBDIRS", target.Subdirectories);

    if (this->HasUnsupportedGeneratorExpression(target.Sources) ||
        this->HasUnsupportedGeneratorExpression(target.Headers) ||
        this->HasUnsupportedGeneratorExpression(target.UiFiles) ||
        this->HasUnsupportedGeneratorExpression(target.ResourceFiles) ||
        this->HasUnsupportedGeneratorExpression(target.Libraries) ||
        this->HasUnsupportedGeneratorExpression(target.CompileOptions) ||
        this->HasUnsupportedGeneratorExpression(target.LinkOptions))
    {
        source += "# q2c warning: CMake generator expressions require manual qmake review.\n";
    }
    return source;
}

QString QMakeGenerator::GenerateConditionalScopes(const BuildTarget &target)
{
    QString source;
    foreach (const BuildConditionalScope &scope, target.ConditionalScopes)
    {
        bool supported = true;
        QString condition = this->MapCondition(scope.Condition, &supported);
        QString block;
        if (!supported)
            source += "# q2c warning: Unsupported CMake condition for qmake scope: " + scope.Condition + "\n";

        block += condition + " {\n";
        block += this->ScopedAssignment("SOURCES", scope.Sources);
        block += this->ScopedAssignment("HEADERS", scope.Headers);
        block += this->ScopedAssignment("DEFINES", scope.Defines);
        block += this->ScopedAssignment("INCLUDEPATH", scope.IncludePaths);
        block += this->ScopedAssignment("LIBS", this->LibrariesForQmake(scope.Libraries));
        block += this->ScopedAssignment("TRANSLATIONS", scope.TranslationFiles);
        block += this->ScopedAssignment("QMAKE_CXXFLAGS", this->CompileOptionsForQmake(scope.CompileOptions));
        block += this->ScopedAssignment("QMAKE_LFLAGS", this->LinkOptionsForQmake(scope.LinkOptions));
        block += this->ScopedAssignment("INSTALLS", scope.InstallRules);
        block += "}\n";
        if (!supported)
            block = "# " + block.replace("\n", "\n# ").trimmed() + "\n";
        source += block;
    }
    return source;
}

QString QMakeGenerator::GenerateAdditionalTargetNotes(const BuildProject &project, const BuildTarget &primary)
{
    QString source;
    foreach (const BuildTarget &target, project.Targets)
    {
        if (&target == &primary || target.Name == primary.Name)
            continue;
        if (target.Type == BuildTarget_Subdirs && primary.Subdirectories.contains(target.Name))
            continue;
        source += "\n# q2c warning: Additional CMake target '" + target.Name + "' is not emitted as a separate .pro/.pri file yet.\n";
        source += "# Suggested qmake template: " + this->ConfigForTarget(target) + "\n";
    }
    return source;
}

QString QMakeGenerator::Assignment(QString variable, const QList<QString> &items) const
{
    if (items.isEmpty())
        return "";

    if (items.size() == 1)
        return variable + " += " + this->Quote(items.first()) + "\n";

    QString result = variable + " += \\\n";
    for (int i = 0; i < items.size(); i++)
    {
        result += "    " + this->Quote(items[i]);
        if (i + 1 < items.size())
            result += " \\";
        result += "\n";
    }
    return result;
}

QString QMakeGenerator::ScopedAssignment(QString variable, const QList<QString> &items) const
{
    QString assignment = this->Assignment(variable, items);
    if (assignment.isEmpty())
        return "";
    assignment = assignment.replace("\n", "\n    ").trimmed();
    return "    " + assignment + "\n";
}

QString QMakeGenerator::ConfigForTarget(const BuildTarget &target) const
{
    if (target.Type == BuildTarget_Subdirs)
        return "subdirs";
    if (target.Type == BuildTarget_Library || target.Type == BuildTarget_Plugin)
        return "lib";
    return "app";
}

QStringList QMakeGenerator::LibrariesForQmake(const QList<QString> &libraries) const
{
    QStringList result;
    foreach (QString library, libraries)
    {
        if (library.startsWith("$<"))
            continue;
        if (library.contains("::"))
            result << library.mid(library.indexOf("::") + 2);
        else
            result << library;
    }
    return result;
}

QStringList QMakeGenerator::CompileOptionsForQmake(const QList<QString> &options) const
{
    QStringList result;
    foreach (QString option, options)
    {
        if (!option.startsWith("$<"))
            result << option;
    }
    return result;
}

QStringList QMakeGenerator::LinkOptionsForQmake(const QList<QString> &options) const
{
    QStringList result;
    foreach (QString option, options)
    {
        if (!option.startsWith("$<"))
            result << option;
    }
    return result;
}

QString QMakeGenerator::MapCondition(QString condition, bool *supported) const
{
    QString normalized = condition.trimmed();
    normalized.remove("(");
    normalized.remove(")");
    normalized = normalized.simplified();
    QString upper = normalized.toUpper();

    if (upper.startsWith("NOT "))
    {
        bool inner_supported = true;
        QString mapped = this->MapCondition(normalized.mid(4), &inner_supported);
        *supported = inner_supported;
        return "!" + mapped;
    }
    if (upper.contains(" AND "))
    {
        QStringList parts = normalized.split(QRegularExpression("\\s+AND\\s+"), Qt::SkipEmptyParts);
        QStringList mapped_parts;
        bool all_supported = true;
        foreach (QString part, parts)
        {
            bool part_supported = true;
            mapped_parts << this->MapCondition(part, &part_supported);
            all_supported = all_supported && part_supported;
        }
        *supported = all_supported;
        return mapped_parts.join(":");
    }

    *supported = true;
    if (upper == "WIN32" || upper == "MSVC")
        return "win32";
    if (upper == "APPLE" || upper == "MACOS" || upper == "DARWIN")
        return "macx";
    if (upper == "UNIX")
        return "unix";
    if (upper == "LINUX")
        return "linux";
    if (upper == "MINGW")
        return "mingw";
    if (upper == "FALSE")
        return "false";

    *supported = false;
    return normalized;
}

QString QMakeGenerator::Quote(QString value) const
{
    if (value.contains(" ") || value.contains(";") || value.contains(":"))
        return "\"" + value + "\"";
    return value;
}

bool QMakeGenerator::HasUnsupportedGeneratorExpression(const QList<QString> &items) const
{
    foreach (QString item, items)
    {
        if (item.contains("$<"))
            return true;
    }
    return false;
}
