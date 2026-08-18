//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "cmakegenerator.h"
#include "generic.h"
#include <QDateTime>

static QString CMakeQuote(QString value)
{
    if (value.contains(" ") || value.contains(";"))
        return "\"" + value + "\"";
    return value;
}

static QString CMakeIndentedList(const QList<QString> &values)
{
    QString result;
    foreach (QString value, values)
        result += "    " + CMakeQuote(value) + "\n";
    return result;
}

CMakeGenerator::CMakeGenerator(CMakeQtVersion version)
{
    this->Version = version;
}

QString CMakeGenerator::Generate(const BuildProject &project, const QList<CMakeOption> &options)
{
    const BuildTarget *target = project.PrimaryTarget();
    QString target_name = target != nullptr ? target->Name : project.Name;
    QString cmake_minimum = project.CMakeMinimumVersion.isEmpty() ? "VERSION 3.1.0" : project.CMakeMinimumVersion;

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from qmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    foreach (QString warning, project.Warnings)
        source += "# q2c warning: " + warning + "\n";
    source += "cmake_minimum_required (" + cmake_minimum + ")\n";
    source += "project(" + target_name + ")\n";
    source += this->GenerateOptions(options);

    if (target != nullptr && target->Type == BuildTarget_Subdirs)
    {
        source += "\n";
        source += this->GenerateSubdirs(project, *target);
        return source;
    }
    if (target == nullptr)
        return source;

    source += this->GenerateConfigOptions(*target);

    if (!target->Sources.isEmpty())
        source += this->GenerateFileSet(target_name + "_SOURCES", target->Sources);
    if (!target->Headers.isEmpty())
        source += this->GenerateFileSet(target_name + "_HEADERS", target->Headers);
    if (!target->UiFiles.isEmpty())
        source += this->GenerateFileSet(target_name + "_UI_FILES", target->UiFiles);
    if (!target->ResourceFiles.isEmpty())
        source += this->GenerateFileSet(target_name + "_RESOURCE_FILES", target->ResourceFiles);

    source += this->GenerateDefaultQtLibs(*target);
    source += this->GenerateQtAutomation(*target);

    if (target->Type == BuildTarget_Plugin)
        source += "add_library(" + target_name + " MODULE";
    else if (target->Type == BuildTarget_Library)
        source += "add_library(" + target_name;
    else
        source += "add_executable(" + target_name;

    if (!target->Sources.isEmpty())
        source += " ${" + target_name + "_SOURCES}";
    if (!target->Headers.isEmpty())
        source += " ${" + target_name + "_HEADERS}";
    if (!target->UiFiles.isEmpty())
        source += " ${" + target_name + "_UI_FILES}";
    if (!target->ResourceFiles.isEmpty())
        source += " ${" + target_name + "_RESOURCE_FILES}";
    source += ")\n";

    if (!target->Headers.isEmpty() && this->Version == CMakeQtVersion_Qt4)
        source += "target_sources(" + target_name + " PRIVATE ${" + target_name + "_HEADERS_MOC})\n";
    else if (!target->Headers.isEmpty() && this->Version == CMakeQtVersion_All)
    {
        source += "IF (QT5BUILD)\n";
        source += "ELSE()\n";
        source += Generic::Indent("target_sources(" + target_name + " PRIVATE ${" + target_name + "_HEADERS_MOC})\n");
        source += "ENDIF()\n";
    }

    source += this->GenerateDefines(*target);
    source += this->GenerateIncludePaths(*target);
    source += this->GenerateConditionalScopes(*target);
    source += this->GenerateUIFiles(*target);
    source += this->GenerateResources(*target);
    source += this->GenerateTranslations(*target);
    source += this->GenerateLibraries(*target);
    source += this->GenerateCompileOptions(*target);
    source += this->GenerateLinkOptions(*target);
    source += this->GenerateInstallRules(*target);
    source += this->GenerateQtModules(*target);

    return source;
}

QString CMakeGenerator::GenerateOptions(const QList<CMakeOption> &options)
{
    QString result;
    foreach (CMakeOption option, options)
        result += "option(" + option.Name + " \"" + option.Description + "\" " + option.Default + ")\n";
    return result;
}

QString CMakeGenerator::GenerateFileSet(QString variable, const QList<QString> &files)
{
    QString result;
    result += "set(" + variable + "\n";
    result += CMakeIndentedList(files);
    result += ")\n";
    return result;
}

QString CMakeGenerator::GenerateSubdirs(const BuildProject &project, const BuildTarget &target)
{
    Q_UNUSED(project);
    QString result;
    if (!target.QtModules.isEmpty())
    {
        result += "# Global Qt settings that apply to all subprojects\n";
        if (this->Version == CMakeQtVersion_All)
            result += "option(QT5BUILD \"Build using Qt5 libs\" TRUE)\n\n";
        result += this->GenerateDefaultQtLibs(target);
        result += "\n";
    }

    result += "# Add all subprojects\n";
    foreach (QString subdir, target.Subdirectories)
        result += "add_subdirectory(" + subdir + ")\n";
    return result;
}

QString CMakeGenerator::GenerateConditionalScopes(const BuildTarget &target)
{
    QString result;
    foreach (const BuildConditionalScope &block, target.ConditionalScopes)
    {
        if (block.Condition.isEmpty())
            continue;

        result += "\nif(" + block.Condition + ")\n";
        if (!block.Sources.isEmpty())
        {
            result += "    target_sources(" + target.Name + " PRIVATE\n";
            foreach (const QString &source, block.Sources)
                result += "        " + CMakeQuote(source) + "\n";
            result += "    )\n";
        }
        if (!block.Headers.isEmpty())
        {
            result += "    target_sources(" + target.Name + " PRIVATE\n";
            foreach (const QString &header, block.Headers)
                result += "        " + CMakeQuote(header) + "\n";
            result += "    )\n";
        }
        foreach (const QString &define, block.Defines)
            result += "    target_compile_definitions(" + target.Name + " PRIVATE " + define + ")\n";
        foreach (const QString &include, block.IncludePaths)
            result += "    target_include_directories(" + target.Name + " PRIVATE " + CMakeQuote(include) + ")\n";
        foreach (const QString &option, block.CompileOptions)
            result += "    target_compile_options(" + target.Name + " PRIVATE " + option + ")\n";
        foreach (const QString &option, block.LinkOptions)
            result += "    target_link_options(" + target.Name + " PRIVATE " + option + ")\n";
        for (int i = 0; i < block.Libraries.size(); i++)
        {
            QString lib = block.Libraries[i];
            if (lib == "-framework" && i + 1 < block.Libraries.size())
            {
            result += "    target_link_libraries(" + target.Name + " PRIVATE \"-framework " + block.Libraries[i + 1] + "\")\n";
                i++;
                continue;
            }
            if (lib.startsWith("-l"))
                result += "    target_link_libraries(" + target.Name + " PRIVATE " + lib.mid(2) + ")\n";
            else if (lib.startsWith("-L"))
                result += "    target_link_directories(" + target.Name + " PRIVATE " + CMakeQuote(lib.mid(2)) + ")\n";
            else
                result += "    target_link_libraries(" + target.Name + " PRIVATE " + CMakeQuote(lib) + ")\n";
        }
        foreach (const QString &rule, block.InstallRules)
            result += "    # qmake INSTALLS entry: " + rule + "\n";
        result += "endif()\n";
    }
    return result;
}

QString CMakeGenerator::GenerateUIFiles(const BuildTarget &target)
{
    QString result;
    if (target.UiFiles.isEmpty())
        return result;
    if (this->Version == CMakeQtVersion_Qt5 || this->Version == CMakeQtVersion_Qt6)
        return result;

    result += "\n# UI files\n";
    if (this->Version == CMakeQtVersion_Qt4)
        result += "qt4_wrap_ui(" + target.Name + "_UI_HEADERS ${" + target.Name + "_UI_FILES})\n";
    else
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent("qt5_wrap_ui(" + target.Name + "_UI_HEADERS ${" + target.Name + "_UI_FILES})\n");
        result += "ELSE()\n";
        result += Generic::Indent("qt4_wrap_ui(" + target.Name + "_UI_HEADERS ${" + target.Name + "_UI_FILES})\n");
        result += "ENDIF()\n";
    }
    result += "target_sources(" + target.Name + " PRIVATE ${" + target.Name + "_UI_HEADERS})\n";
    return result;
}

QString CMakeGenerator::GenerateResources(const BuildTarget &target)
{
    QString result;
    if (target.ResourceFiles.isEmpty())
        return result;
    if (this->Version == CMakeQtVersion_Qt5 || this->Version == CMakeQtVersion_Qt6)
        return result;

    result += "\n# Resource files\n";
    if (this->Version == CMakeQtVersion_Qt4)
        result += "qt4_add_resources(" + target.Name + "_RESOURCES ${" + target.Name + "_RESOURCE_FILES})\n";
    else
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent("qt5_add_resources(" + target.Name + "_RESOURCES ${" + target.Name + "_RESOURCE_FILES})\n");
        result += "ELSE()\n";
        result += Generic::Indent("qt4_add_resources(" + target.Name + "_RESOURCES ${" + target.Name + "_RESOURCE_FILES})\n");
        result += "ENDIF()\n";
    }
    result += "target_sources(" + target.Name + " PRIVATE ${" + target.Name + "_RESOURCES})\n";
    return result;
}

QString CMakeGenerator::GenerateTranslations(const BuildTarget &target)
{
    QString result;
    if (target.TranslationFiles.isEmpty())
        return result;

    result += "\n# Translation files\n";
    result += "set(" + target.Name + "_TRANSLATIONS";
    foreach (QString translation, target.TranslationFiles)
        result += " \"" + translation + "\"";
    result += ")\n";
    return result;
}

QString CMakeGenerator::GenerateConfigOptions(const BuildTarget &target)
{
    QString result;
    foreach (QString config, target.Config)
    {
        if (config == "c++11")
            result += "set(CMAKE_CXX_STANDARD 11)\n";
        else if (config == "c++14")
            result += "set(CMAKE_CXX_STANDARD 14)\n";
        else if (config == "c++17")
            result += "set(CMAKE_CXX_STANDARD 17)\n";
        else if (config == "debug")
            result += "set(CMAKE_BUILD_TYPE Debug)\n";
        else if (config == "release")
            result += "set(CMAKE_BUILD_TYPE Release)\n";
    }
    return result;
}

QString CMakeGenerator::GenerateDefines(const BuildTarget &target)
{
    QString result;
    if (target.Defines.isEmpty())
        return result;

    result += "target_compile_definitions(" + target.Name + " PRIVATE\n";
    result += CMakeIndentedList(target.Defines);
    result += ")\n";
    return result;
}

QString CMakeGenerator::GenerateIncludePaths(const BuildTarget &target)
{
    QString result;
    if (target.IncludePaths.isEmpty())
        return result;

    result += "target_include_directories(" + target.Name + " PRIVATE\n";
    result += CMakeIndentedList(target.IncludePaths);
    result += ")\n";
    return result;
}

QString CMakeGenerator::GenerateLibraries(const BuildTarget &target)
{
    QString result;
    for (int i = 0; i < target.Libraries.size(); i++)
    {
        QString lib = target.Libraries[i];
        if (lib == "-framework" && i + 1 < target.Libraries.size())
        {
            result += "target_link_libraries(" + target.Name + " PRIVATE \"-framework " + target.Libraries[i + 1] + "\")\n";
            i++;
            continue;
        }
        if (lib.startsWith("-l"))
            result += "target_link_libraries(" + target.Name + " PRIVATE " + lib.mid(2) + ")\n";
        else if (lib.startsWith("-L"))
            result += "target_link_directories(" + target.Name + " PRIVATE " + CMakeQuote(lib.mid(2)) + ")\n";
        else
            result += "target_link_libraries(" + target.Name + " PRIVATE " + CMakeQuote(lib) + ")\n";
    }
    return result;
}

QString CMakeGenerator::GenerateCompileOptions(const BuildTarget &target)
{
    QString result;
    foreach (QString option, target.CompileOptions)
        result += "target_compile_options(" + target.Name + " PRIVATE " + option + ")\n";
    return result;
}

QString CMakeGenerator::GenerateLinkOptions(const BuildTarget &target)
{
    QString result;
    foreach (QString option, target.LinkOptions)
        result += "target_link_options(" + target.Name + " PRIVATE " + option + ")\n";
    return result;
}

QString CMakeGenerator::GenerateInstallRules(const BuildTarget &target)
{
    QString result;
    foreach (QString rule, target.InstallRules)
        result += "# qmake INSTALLS entry: " + rule + "\n";
    return result;
}

QString CMakeGenerator::GenerateDefaultQtLibs(const BuildTarget &target)
{
    QString result;
    bool has_cxx_standard = target.Config.contains("c++11") ||
                            target.Config.contains("c++14") ||
                            target.Config.contains("c++17");
    if (!has_cxx_standard)
    {
        if (this->Version == CMakeQtVersion_Qt6)
            result += "set(CMAKE_CXX_STANDARD 17)\n";
        else
            result += "set(CMAKE_CXX_STANDARD 11)\n";
    }
    result += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";

    if (this->Version == CMakeQtVersion_Qt4)
        result += this->GenerateQt4Libs();
    else if (this->Version == CMakeQtVersion_Qt5)
        result += this->GenerateQt5Libs(target);
    else if (this->Version == CMakeQtVersion_Qt6)
        result += this->GenerateQt6Libs(target);
    else
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent(this->GenerateQt5Libs(target));
        result += "ELSE()\n";
        result += Generic::Indent(this->GenerateQt4Libs());
        result += "ENDIF()\n";
    }

    if (!target.Headers.isEmpty() && this->Version == CMakeQtVersion_Qt4)
    {
        result += "qt4_wrap_cpp(" + target.Name + "_HEADERS_MOC ${" + target.Name + "_HEADERS})\n";
    }
    else if (!target.Headers.isEmpty() && this->Version == CMakeQtVersion_All)
    {
        result += "IF (QT5BUILD)\n";
        result += "ELSE()\n";
        result += Generic::Indent("qt4_wrap_cpp(" + target.Name + "_HEADERS_MOC ${" + target.Name + "_HEADERS})\n");
        result += "ENDIF()\n";
    }

    return result;
}

QString CMakeGenerator::GenerateQtAutomation(const BuildTarget &target)
{
    QString result;
    Q_UNUSED(target);

    if (this->Version == CMakeQtVersion_Qt5 || this->Version == CMakeQtVersion_Qt6)
    {
        result += "set(CMAKE_AUTOMOC ON)\n";
        result += "set(CMAKE_AUTOUIC ON)\n";
        result += "set(CMAKE_AUTORCC ON)\n";
    }
    else if (this->Version == CMakeQtVersion_All)
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent("set(CMAKE_AUTOMOC ON)\n");
        result += Generic::Indent("set(CMAKE_AUTOUIC ON)\n");
        result += Generic::Indent("set(CMAKE_AUTORCC ON)\n");
        result += "ENDIF()\n";
    }
    if (!result.isEmpty())
        result += "\n";
    return result;
}

QString CMakeGenerator::GenerateQt4Libs()
{
    QString result;
    result += "find_package(Qt4 REQUIRED)\n";
    result += "include(${QT_USE_FILE})\n";
    return result;
}

QString CMakeGenerator::GenerateQt5Libs(const BuildTarget &target)
{
    QString components = "COMPONENTS";
    QList<QString> modules = target.QtModules;
    if (modules.isEmpty())
        modules << "core";

    foreach (QString module, modules)
        components += " " + this->QtComponentName(module);
    return "find_package(Qt5 " + components + " REQUIRED)\n\n";
}

QString CMakeGenerator::GenerateQt6Libs(const BuildTarget &target)
{
    QString components = "COMPONENTS";
    QList<QString> modules = target.QtModules;
    if (modules.isEmpty())
        modules << "core";

    foreach (QString module, modules)
        components += " " + this->QtComponentName(module);
    return "find_package(Qt6 " + components + " REQUIRED)\n\n";
}

QString CMakeGenerator::GenerateQtModules(const BuildTarget &target)
{
    if (this->Version == CMakeQtVersion_Qt4 || target.QtModules.isEmpty())
        return "";

    QString result;
    if (this->Version == CMakeQtVersion_Qt6)
    {
        result += "target_link_libraries(" + target.Name + " PRIVATE";
        foreach (QString module, target.QtModules)
            result += " Qt6::" + this->QtTargetName(module);
        result += ")\n";
    }
    else if (this->Version == CMakeQtVersion_Qt5)
    {
        result += "target_link_libraries(" + target.Name + " PRIVATE";
        foreach (QString module, target.QtModules)
            result += " Qt5::" + this->QtTargetName(module);
        result += ")\n";
    }
    else
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent("target_link_libraries(" + target.Name + " PRIVATE");
        foreach (QString module, target.QtModules)
            result += " Qt5::" + this->QtTargetName(module);
        result += ")\n";
        result += "ELSE()\n";
        result += Generic::Indent("target_link_libraries(" + target.Name + " ${QT_LIBRARIES})\n");
        result += "ENDIF()\n";
    }
    return result;
}

QString CMakeGenerator::QtComponentName(QString module) const
{
    if (this->Version == CMakeQtVersion_Qt6 && module == "webkit")
        return "WebEngineCore";
    if (this->Version == CMakeQtVersion_Qt6 && module == "webkitwidgets")
        return "WebEngineWidgets";
    if (module == "webkit")
        return "WebKit";
    if (module == "webkitwidgets")
        return "WebKitWidgets";
    return Generic::CapitalFirst(module);
}

QString CMakeGenerator::QtTargetName(QString module) const
{
    return this->QtComponentName(module);
}

CMakeOption::CMakeOption(QString name, QString description, QString __default)
{
    this->Name = name;
    this->Description = description;
    this->Default = __default;
}
