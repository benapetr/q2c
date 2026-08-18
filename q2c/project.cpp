//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "project.h"
#include "generic.h"
#include <QStringList>

Project::Project()
{
    this->ProjectName = "";
    this->CMakeMinumumVersion = "VERSION 3.1.0";
    this->KnownSimpleKeywords << "TARGET" << "TEMPLATE";
    this->RequiredKeywords << "TARGET";
    this->KnownComplexKeywords << "SOURCES" << "HEADERS" << "QT" << "CONFIG" << "DEFINES" 
                              << "INCLUDEPATH" << "LIBS" << "FORMS" << "RESOURCES" << "SUBDIRS";
    this->Version = QtVersion_All;
    this->IsSubdirsProject = false;
    this->TargetType = BuildTarget_Application;
    this->TemplateName = "app";
    this->CurrentLineNumber = 0;
    this->TargetLine = 0;
    if (Configuration::only_qt4)
    {
        this->Version = QtVersion_Qt4;
    } else if (Configuration::only_qt5)
    {
        this->Version = QtVersion_Qt5;
    } else if (Configuration::only_qt6)
    {
        this->Version = QtVersion_Qt6;
        this->CMakeMinumumVersion = "VERSION 3.16.0";  // Qt6 requires newer CMake
    }
    this->RemainingRequiredKeywords = this->RequiredKeywords;
    this->Modules << "core";
    this->RefreshModel();
}

QString generateCMakeOptions(QList<CMakeOption> *options)
{
    QString result;
    foreach (CMakeOption option, *options)
    {
        result += "option(" + option.Name + " \"" + option.Description + "\" " + option.Default + ")\n";
    }
    return result;
}

bool Project::Load(QString text)
{
    return this->ParseQmake(text);
}

const BuildProject &Project::GetModel() const
{
    return this->Model;
}

bool Project::ParseQmake(QString text)
{
    ParserState state = ParserState_LookingForKeyword;
    QStringList lines = text.split("\n");
    QString data_buffer;
    QString current_word;
    QString current_line;
    
    for (int i = 0; i < lines.size(); i++)
    {
        this->CurrentLineNumber = i + 1;
        QString line = lines[i];
        // Trim leading spaces
        while (line.startsWith(" "))
            line = line.mid(1);
        // If line starts with hash we can ignore it
        if (line.startsWith("#") || line.isEmpty())
            continue;

        // Check for conditional statements
        if (line.startsWith("win32:") || line.startsWith("unix:") || 
            line.startsWith("linux:") || line.startsWith("macx:") ||
            line.trimmed().startsWith("if(") || line.trimmed().startsWith("else {") ||
            line.trimmed() == "else:" || line.trimmed().startsWith("} else"))
        {
            if (!ProcessScope(line, lines, i))
                return false;
            continue;
        }

        if (state == ParserState_LookingForKeyword)
        {
            // Regular keyword processing as before
            QString keyword = line;
            if (keyword.contains(" "))
                keyword = keyword.mid(0, keyword.indexOf(" "));
            Logs::DebugLog("Possible keyword: " + keyword);
            if (this->KnownSimpleKeywords.contains(keyword))
            {
                if (!this->ProcessSimpleKeyword(keyword, line))
                    return false;
            } else if (this->KnownComplexKeywords.contains(keyword))
            {
                current_word = keyword;
                current_line = line;
                data_buffer = line;
                if (line.endsWith("\\"))
                {
                    state = ParserState_FetchingData;
                } else
                {
                    if (!this->ProcessComplexKeyword(keyword, current_line, data_buffer))
                        return false;
                }
            } else
            {
                Logs::DebugLog("Ignoring unknown keyword: " + keyword);
            }
        } else if (state == ParserState_FetchingData)
        {
            data_buffer += "\n" + line;
            if (!line.endsWith("\\"))
            {
                state = ParserState_LookingForKeyword;
                if (!this->ProcessComplexKeyword(current_word, current_line, data_buffer))
                    return false;
            }
        }
    }
    
    if (!this->RemainingRequiredKeywords.isEmpty())
    {
        foreach (QString word, this->RemainingRequiredKeywords)
        {
            Logs::ErrorLog("Required keyword not found: " + word);
        }
        return false;
    }

    this->RefreshModel();
    return true;
}

BuildTargetType Project::TargetTypeFromTemplate(QString value)
{
    value = value.trimmed().toLower();
    if (value == "app")
        return BuildTarget_Application;
    if (value == "lib")
        return BuildTarget_Library;
    if (value == "subdirs")
        return BuildTarget_Subdirs;
    return BuildTarget_Unknown;
}

BuildTargetType Project::TargetTypeFromConfig(BuildTargetType current_type) const
{
    if (this->Config.contains("testcase"))
        return BuildTarget_Test;
    if (this->Config.contains("plugin"))
        return BuildTarget_Plugin;
    return current_type;
}

BuildTarget *Project::PrimaryBuildTarget()
{
    return this->Model.EnsurePrimaryTarget();
}

const BuildTarget *Project::PrimaryBuildTarget() const
{
    return this->Model.PrimaryTarget();
}

void Project::RefreshModel()
{
    this->Model.Clear();
    this->Model.Name = this->ProjectName;
    this->Model.CMakeMinimumVersion = this->CMakeMinumumVersion;
    this->Model.GlobalConfig = this->Config;
    this->Model.GlobalQtModules = this->Modules;

    BuildTarget *target = this->Model.EnsurePrimaryTarget();
    target->Name = this->ProjectName;
    if (target->Name.isEmpty() && this->IsSubdirsProject)
        target->Name = "MainProject";
    target->Type = this->TargetTypeFromConfig(this->TargetType);
    target->Location = BuildSourceLocation(Configuration::InputFile, this->TargetLine);
    target->Sources = this->Sources;
    target->Headers = this->Headers;
    target->UiFiles = this->UIFiles;
    target->ResourceFiles = this->ResourceFiles;
    target->QtModules = this->Modules;
    target->Config = this->Config;
    target->Defines = this->Defines;
    target->IncludePaths = this->IncludePaths;
    target->Libraries = this->Libraries;
    target->Subdirectories = this->Subdirectories;

    foreach (const ConditionalBlock &block, this->ConditionalBlocks)
    {
        BuildConditionalScope scope;
        scope.Condition = block.condition;
        scope.Location = BuildSourceLocation(Configuration::InputFile, block.line);
        scope.Sources = block.Sources;
        scope.Headers = block.Headers;
        scope.Defines = block.Defines;
        scope.IncludePaths = block.IncludePaths;
        scope.Libraries = block.Libraries;
        scope.Config = block.Config;
        target->ConditionalScopes.append(scope);
    }

    foreach (QString subdir, this->Subdirectories)
    {
        BuildTarget subtarget;
        subtarget.Name = subdir;
        subtarget.Type = BuildTarget_Subdirs;
        subtarget.Location = BuildSourceLocation(Configuration::InputFile, 0);
        this->Model.Targets.append(subtarget);
    }
}

bool Project::ProcessScope(QString line, QStringList &lines, int &currentLine)
{
    ConditionalBlock block;
    QString condition;
    block.line = currentLine + 1;
    
    if (line.startsWith("win32:"))
    {
        condition = "WIN32";
    }
    else if (line.startsWith("unix:"))
    {
        condition = "UNIX";
    }
    else if (line.startsWith("linux:"))
    {
        condition = "UNIX AND NOT APPLE";
    }
    else if (line.startsWith("macx:"))
    {
        condition = "APPLE";
    }
    else if (line.trimmed().startsWith("if("))
    {
        // Extract condition from if statement
        condition = line.mid(line.indexOf("(") + 1);
        condition = condition.left(condition.indexOf(")"));
        condition = ParseCondition(condition);
    }
    
    block.condition = condition;
    block.active = EvaluateCondition(condition);
    
    // Process the scope until we hit the matching end brace or next section
    int braceCount = 0;
    bool inScope = true;
    
    while (inScope && currentLine < lines.size() - 1)
    {
        currentLine++;
        QString currentLine_str = lines[currentLine].trimmed();
        
        if (currentLine_str.startsWith("{"))
        {
            braceCount++;
            continue;
        }
        else if (currentLine_str.startsWith("}"))
        {
            if (braceCount == 0)
            {
                inScope = false;
            }
            braceCount--;
            continue;
        }
        
        if (block.active)
        {
            // Process the line within the conditional block
            QString keyword = currentLine_str;
            if (keyword.contains(" "))
                keyword = keyword.mid(0, keyword.indexOf(" "));
                
            if (keyword == "SOURCES")
                ParseStandardQMakeList(&block.Sources, currentLine_str, currentLine_str);
            else if (keyword == "HEADERS")
                ParseStandardQMakeList(&block.Headers, currentLine_str, currentLine_str);
            else if (keyword == "DEFINES")
                ParseStandardQMakeList(&block.Defines, currentLine_str, currentLine_str);
            else if (keyword == "INCLUDEPATH")
                ParseStandardQMakeList(&block.IncludePaths, currentLine_str, currentLine_str);
            else if (keyword == "LIBS")
                ParseStandardQMakeList(&block.Libraries, currentLine_str, currentLine_str);
            else if (keyword == "CONFIG")
                ParseStandardQMakeList(&block.Config, currentLine_str, currentLine_str);
        }
    }
    
    ConditionalBlocks.append(block);
    return true;
}

QString Project::ParseCondition(QString condition)
{
    // Convert qmake conditions to CMake conditions and store in the condition string
    condition = condition.replace("$$QT_MAJOR_VERSION", "QT_VERSION_MAJOR");
    condition = condition.replace(">=", " GREATER_EQUAL ");
    condition = condition.replace("<=", " LESS_EQUAL ");
    condition = condition.replace(">", " GREATER ");
    condition = condition.replace("<", " LESS ");
    condition = condition.replace("==", " EQUAL ");
    condition = condition.replace("&&", " AND ");
    condition = condition.replace("||", " OR ");
    condition = condition.replace("!", " NOT ");
    
    return condition;
}

bool Project::EvaluateCondition(QString condition)
{
    Q_UNUSED(condition);
    // CMake will evaluate the generated condition at configure time.
    return true;
}

QString Project::ProcessPlatformSpecific()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    if (target == nullptr)
        return result;
    QString target_name = target->Name;

    foreach (const BuildConditionalScope &block, target->ConditionalScopes)
    {
        if (!block.Condition.isEmpty())
        {
            result += "\nif(" + block.Condition + ")\n";
            
            // Add platform-specific sources
            if (!block.Sources.isEmpty())
            {
                result += "    target_sources(" + target_name + " PRIVATE\n";
                foreach (const QString &source, block.Sources)
                    result += "        " + source + "\n";
                result += "    )\n";
            }
            
            // Add platform-specific definitions
            foreach (const QString &define, block.Defines)
                result += "    add_definitions(-D" + define + ")\n";
            
            // Add platform-specific include paths
            foreach (const QString &include, block.IncludePaths)
                result += "    include_directories(" + include + ")\n";
            
            // Add platform-specific libraries
            foreach (const QString &lib, block.Libraries)
            {
                if (lib.startsWith("-l"))
                    result += "    target_link_libraries(" + target_name + " " + lib.mid(2) + ")\n";
                else if (lib.startsWith("-L"))
                    result += "    link_directories(" + lib.mid(2) + ")\n";
                else
                    result += "    target_link_libraries(" + target_name + " " + lib + ")\n";
            }
            
            result += "endif()\n";
        }
    }
    return result;
}

QString Project::ToQmake()
{
    const BuildTarget *target = this->PrimaryBuildTarget();
    QString target_name = target != nullptr ? target->Name : this->ProjectName;

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "TARGET = " + target_name;
    return source;
}

QString Project::ToCmake()
{
    this->RefreshModel();
    const BuildTarget *target = this->PrimaryBuildTarget();
    QString target_name = target != nullptr ? target->Name : this->ProjectName;

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from qmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "cmake_minimum_required (" + this->Model.CMakeMinimumVersion + ")\n";

    if (target != nullptr && target->Type == BuildTarget_Subdirs)
        source += "project(" + target_name + ")\n";
    else
        source += "project(" + target_name + ")\n";

    source += generateCMakeOptions(&this->CMakeOptions);

    if (target != nullptr && target->Type == BuildTarget_Subdirs)
    {
        source += "\n";
        source += ProcessSubdirsInCMake();
        return source;
    }

    // Process CONFIG options
    source += ProcessConfigOptions();

    // Process defines
    source += ProcessDefines();

    // Process include paths
    source += ProcessIncludePaths();

    // Sources, headers and so on
    if (target != nullptr && !target->Sources.isEmpty())
    {
        source += "set(" + target_name + "_SOURCES";
        foreach (QString src, target->Sources)
        {
            source += " \"" + src + "\"";
        }
        source += ")\n";
    }
    if (target != nullptr && !target->Headers.isEmpty())
    {
        source += "set(" + target_name + "_HEADERS";
        foreach (QString src, target->Headers)
        {
            source += " \"" + src + "\"";
        }
        source += ")\n";
    }

    // Qt libs
    source += this->GetCMakeDefaultQtLibs();

    if (target != nullptr && (target->Type == BuildTarget_Library || target->Type == BuildTarget_Plugin))
        source += "add_library(" + target_name;
    else
        source += "add_executable(" + target_name;

    if (target != nullptr && !target->Sources.isEmpty())
        source += " ${" + target_name + "_SOURCES}";
    if (target != nullptr && !target->Headers.isEmpty())
        source += " ${" + target_name + "_HEADERS}";
    source += ")\n";

    if (target != nullptr && !target->Headers.isEmpty())
        source += "target_sources(" + target_name + " PRIVATE ${" + target_name + "_HEADERS_MOC})\n";

    // Process platform-specific code after the target exists.
    source += ProcessPlatformSpecific();

    // Process UI files
    source += ProcessUIFiles();

    // Process resource files
    source += ProcessResources();

    // Process libraries
    source += ProcessLibs();

    // Qt5 modules
    source += this->GetCMakeQtModules();

    return source;
}

QString Project::ProcessSubdirsInCMake()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> qt_modules = target != nullptr ? target->QtModules : this->Modules;
    QList<QString> subdirectories = target != nullptr ? target->Subdirectories : this->Subdirectories;
    
    // First, check if we have any Qt-wide settings that should apply to all subprojects
    if (!qt_modules.isEmpty())
    {
        result += "# Global Qt settings that apply to all subprojects\n";
        if (this->Version == QtVersion_All)
        {
            result += "option(QT5BUILD \"Build using Qt5 libs\" TRUE)\n\n";
        }
        result += GetCMakeDefaultQtLibs();
        result += "\n";
    }

    result += "# Add all subprojects\n";
    foreach (QString subdir, subdirectories)
    {
        result += "add_subdirectory(" + subdir + ")\n";
    }
    
    return result;
}

QString Project::ProcessUIFiles()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    if (target != nullptr && !target->UiFiles.isEmpty())
    {
        QString target_name = target->Name;
        result += "\n# UI files\n";
        result += "set(" + target_name + "_UI_FILES";
        foreach (QString ui, target->UiFiles)
        {
            result += " \"" + ui + "\"";
        }
        result += ")\n";
        
        if (this->Version == QtVersion_Qt6)
        {
            result += "qt6_wrap_ui(" + target_name + "_UI_HEADERS ${" + target_name + "_UI_FILES})\n";
        }
        else if (this->Version == QtVersion_Qt5)
        {
            result += "qt5_wrap_ui(" + target_name + "_UI_HEADERS ${" + target_name + "_UI_FILES})\n";
        }
        else if (this->Version == QtVersion_Qt4)
        {
            result += "qt4_wrap_ui(" + target_name + "_UI_HEADERS ${" + target_name + "_UI_FILES})\n";
        }
        else
        {
            result += "IF (QT5BUILD)\n";
            result += Generic::Indent("qt5_wrap_ui(" + target_name + "_UI_HEADERS ${" + target_name + "_UI_FILES})\n");
            result += "ELSE()\n";
            result += Generic::Indent("qt4_wrap_ui(" + target_name + "_UI_HEADERS ${" + target_name + "_UI_FILES})\n");
            result += "ENDIF()\n";
        }
        
        // Add generated headers to target
        result += "target_sources(" + target_name + " PRIVATE ${" + target_name + "_UI_HEADERS})\n";
    }
    return result;
}

QString Project::ProcessResources()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    if (target != nullptr && !target->ResourceFiles.isEmpty())
    {
        QString target_name = target->Name;
        result += "\n# Resource files\n";
        result += "set(" + target_name + "_RESOURCE_FILES";
        foreach (QString qrc, target->ResourceFiles)
        {
            result += " \"" + qrc + "\"";
        }
        result += ")\n";
        
        if (this->Version == QtVersion_Qt6)
        {
            result += "qt6_add_resources(" + target_name + "_RESOURCES ${" + target_name + "_RESOURCE_FILES})\n";
        }
        else if (this->Version == QtVersion_Qt5)
        {
            result += "qt5_add_resources(" + target_name + "_RESOURCES ${" + target_name + "_RESOURCE_FILES})\n";
        }
        else if (this->Version == QtVersion_Qt4)
        {
            result += "qt4_add_resources(" + target_name + "_RESOURCES ${" + target_name + "_RESOURCE_FILES})\n";
        }
        else
        {
            result += "IF (QT5BUILD)\n";
            result += Generic::Indent("qt5_add_resources(" + target_name + "_RESOURCES ${" + target_name + "_RESOURCE_FILES})\n");
            result += "ELSE()\n";
            result += Generic::Indent("qt4_add_resources(" + target_name + "_RESOURCES ${" + target_name + "_RESOURCE_FILES})\n");
            result += "ENDIF()\n";
        }
        
        // Add generated resource files to target
        result += "target_sources(" + target_name + " PRIVATE ${" + target_name + "_RESOURCES})\n";
    }
    return result;
}

QString Project::ProcessConfigOptions()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> config_list = target != nullptr ? target->Config : this->Config;

    foreach (QString config, config_list)
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
        // Add more config mappings as needed
    }
    return result;
}

QString Project::ProcessDefines()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> defines = target != nullptr ? target->Defines : this->Defines;

    if (!defines.isEmpty())
    {
        foreach (QString define, defines)
        {
            result += "add_definitions(-D" + define + ")\n";
        }
    }
    return result;
}

QString Project::ProcessIncludePaths()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> include_paths = target != nullptr ? target->IncludePaths : this->IncludePaths;

    if (!include_paths.isEmpty())
    {
        foreach (QString path, include_paths)
        {
            result += "include_directories(" + path + ")\n";
        }
    }
    return result;
}

QString Project::ProcessLibs()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QString target_name = target != nullptr ? target->Name : this->ProjectName;
    QList<QString> libraries = target != nullptr ? target->Libraries : this->Libraries;

    if (!libraries.isEmpty())
    {
        foreach (QString lib, libraries)
        {
            // Handle -l and -L flags
            if (lib.startsWith("-l"))
                result += "target_link_libraries(" + target_name + " " + lib.mid(2) + ")\n";
            else if (lib.startsWith("-L"))
                result += "link_directories(" + lib.mid(2) + ")\n";
            else
                result += "target_link_libraries(" + target_name + " " + lib + ")\n";
        }
    }
    return result;
}

QString Project::FinishCut(QString text)
{
    if (text.contains("\n"))
    {
        text = text.mid(0, text.indexOf("\n"));
    }
    if (text.contains(" "))
    {
        text = text.mid(0, text.indexOf(" "));
    }
    return text;
}

bool Project::ParseStandardQMakeList(QList<QString> *list, QString line, QString text)
{
    if (!line.contains("="))
    {
        Logs::ErrorLog("Syntax error: expected '=' or '+=', neither of these 2 found");
        Logs::ErrorLog("Line: " + line);
        return false;
    }
    if (line.contains("-="))
    {
        text = text.mid(text.indexOf("-=") + 2);
        text = text.replace("\n", " ");
        text = text.replace("\\", " ");
        QStringList items = text.split(" ", Qt::SkipEmptyParts);
        foreach (QString rm, items)
        {
            list->removeAll(rm);
        }
        return true;
    }

    text = text.mid(text.indexOf("=") + 1);
    text = text.replace("\n", " ");
    text = text.replace("\\", " ");
    if (!line.contains("+="))
    {
        // Wipe current buffer
        list->clear();
    }
    QStringList items = text.split(" ", Qt::SkipEmptyParts);
    foreach (QString item, items)
    {
        if (!list->contains(item))
            list->append(item);
    }
    return true;
}

bool Project::ProcessSimpleKeyword(QString word, QString line)
{
    if (this->RemainingRequiredKeywords.contains(word))
        this->RemainingRequiredKeywords.removeAll(word);
    if (word == "TARGET")
    {
        if (!line.contains("="))
        {
            Logs::ErrorLog("Syntax error: expected '=' not found");
            Logs::ErrorLog("Line: " + line);
            return false;
        }
        QString target_name = line.mid(line.indexOf("=") + 1);
        while (target_name.startsWith(" "))
            target_name = target_name.mid(1);
        // Remove quotes
        target_name.replace("\"", "");
        // Project name should not end with spaces either
        target_name = target_name.trimmed();
        target_name = target_name.replace(" ", "_");
        this->ProjectName = target_name;
        this->TargetLine = this->CurrentLineNumber;
    }
    else if (word == "TEMPLATE")
    {
        QString value = line.mid(line.indexOf("=") + 1).trimmed();
        this->TemplateName = value;
        this->TargetType = TargetTypeFromTemplate(value);
        if (value == "subdirs")
        {
            this->IsSubdirsProject = true;
            // For subdirs projects, TARGET is not required
            this->RequiredKeywords.removeAll("TARGET");
            this->RemainingRequiredKeywords.removeAll("TARGET");
        }
    }

    return true;
}

bool Project::ProcessComplexKeyword(QString word, QString line, QString data_buffer)
{
    if (this->RemainingRequiredKeywords.contains(word))
        this->RemainingRequiredKeywords.removeAll(word);
    if (word == "SOURCES")
    {
        if (!this->ParseStandardQMakeList(&this->Sources, line, data_buffer))
            return false;
    } else if (word == "HEADERS")
    {
        if (!this->ParseStandardQMakeList(&this->Headers, line, data_buffer))
            return false;
    } else if (word == "QT")
    {
        if (line.contains("-="))
        {
            // For -= operations, remove the modules
            QString modulesText = data_buffer.mid(data_buffer.indexOf("-=") + 2);
            modulesText = modulesText.replace("\n", " ").replace("\\", " ");
            QStringList modulesToRemove = modulesText.split(" ", Qt::SkipEmptyParts);
            foreach (QString module, modulesToRemove)
            {
                this->Modules.removeAll(module);
            }
        }
        else if (!this->ParseStandardQMakeList(&this->Modules, line, data_buffer))
            return false;
    } else if (word == "CONFIG")
    {
        if (!this->ParseStandardQMakeList(&this->Config, line, data_buffer))
            return false;
    } else if (word == "DEFINES")
    {
        if (!this->ParseStandardQMakeList(&this->Defines, line, data_buffer))
            return false;
    } else if (word == "INCLUDEPATH")
    {
        if (!this->ParseStandardQMakeList(&this->IncludePaths, line, data_buffer))
            return false;
    } else if (word == "LIBS")
    {
        if (!this->ParseStandardQMakeList(&this->Libraries, line, data_buffer))
            return false;
    } else if (word == "FORMS")
    {
        if (!this->ParseStandardQMakeList(&this->UIFiles, line, data_buffer))
            return false;
    } else if (word == "RESOURCES")
    {
        if (!this->ParseStandardQMakeList(&this->ResourceFiles, line, data_buffer))
            return false;
    } else if (word == "SUBDIRS")
    {
        if (!this->ParseStandardQMakeList(&this->Subdirectories, line, data_buffer))
            return false;
    }
    return true;
}

QString Project::GetCMakeDefaultQtLibs()
{
    QString result;
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> config = target != nullptr ? target->Config : this->Config;
    QList<QString> headers = target != nullptr ? target->Headers : this->Headers;
    QString target_name = target != nullptr ? target->Name : this->ProjectName;

    bool has_cxx_standard = config.contains("c++11") ||
                            config.contains("c++14") ||
                            config.contains("c++17");
    if (!has_cxx_standard)
    {
        // Set C++ standard - Qt6 requires C++17
        if (this->Version == QtVersion_Qt6)
        {
            result += "set(CMAKE_CXX_STANDARD 17)\n";
        }
        else
        {
            result += "set(CMAKE_CXX_STANDARD 11)\n";
        }
    }
    result += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";

    if (this->Version == QtVersion_Qt4)
        result += this->GetCMakeQt4Libs();
    else if (this->Version == QtVersion_Qt5)
        result += this->GetCMakeQt5Libs();
    else if (this->Version == QtVersion_Qt6)
        result += this->GetCMakeQt6Libs();
    else
    {
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent(this->GetCMakeQt5Libs());
        result += "ELSE()\n";
        result += Generic::Indent(this->GetCMakeQt4Libs());
        result += "ENDIF()\n";
    }

    // Add MOC headers generation
    if (!headers.isEmpty())
    {
        if (this->Version == QtVersion_Qt6)
        {
            result += "qt6_wrap_cpp(" + target_name + "_HEADERS_MOC ${" + target_name + "_HEADERS})\n";
        }
        else if (this->Version == QtVersion_Qt5)
        {
            result += "qt5_wrap_cpp(" + target_name + "_HEADERS_MOC ${" + target_name + "_HEADERS})\n";
        }
        else if (this->Version == QtVersion_Qt4)
        {
            result += "qt4_wrap_cpp(" + target_name + "_HEADERS_MOC ${" + target_name + "_HEADERS})\n";
        }
        else
        {
            result += "IF (QT5BUILD)\n";
            result += Generic::Indent("qt5_wrap_cpp(" + target_name + "_HEADERS_MOC ${" + target_name + "_HEADERS})\n");
            result += "ELSE()\n";
            result += Generic::Indent("qt4_wrap_cpp(" + target_name + "_HEADERS_MOC ${" + target_name + "_HEADERS})\n");
            result += "ENDIF()\n";
        }
    }

    return result;
}

QString Project::GetCMakeQt4Libs()
{
    QString result;
    result += "find_package(Qt4 REQUIRED)\n";
    result += "include(${QT_USE_FILE})\n";
    return result;
}

QString Project::GetCMakeQt5Libs()
{
    QString result;
    QString components = "COMPONENTS";
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> modules = target != nullptr ? target->QtModules : this->Modules;

    // Always include Core if no modules specified
    if (modules.isEmpty())
        modules << "core";

    foreach (QString module, modules)
    {
        QString capitalModule = Generic::CapitalFirst(module);
        if (module == "webkit")
            capitalModule = "WebKit";
        else if (module == "webkitwidgets")
            capitalModule = "WebKitWidgets";
        components += " " + capitalModule;
    }

    result += "find_package(Qt5 " + components + " REQUIRED)\n\n";
    return result;
}

QString Project::GetCMakeQt6Libs()
{
    QString result;
    QString components = "COMPONENTS";
    const BuildTarget *target = this->PrimaryBuildTarget();
    QList<QString> modules = target != nullptr ? target->QtModules : this->Modules;

    // Always include Core if no modules specified
    if (modules.isEmpty())
        modules << "core";

    foreach (QString module, modules)
    {
        QString capitalModule = Generic::CapitalFirst(module);
        if (module == "webkit")
            capitalModule = "WebEngineCore"; // Qt6 uses QtWebEngine instead of QtWebKit
        else if (module == "webkitwidgets")
            capitalModule = "WebEngineWidgets";
        components += " " + capitalModule;
    }

    result += "find_package(Qt6 " + components + " REQUIRED)\n\n";
    return result;
}

QString Project::GetCMakeQtModules()
{
    const BuildTarget *target = this->PrimaryBuildTarget();
    QString target_name = target != nullptr ? target->Name : this->ProjectName;
    QList<QString> modules = target != nullptr ? target->QtModules : this->Modules;

    if (this->Version == QtVersion_Qt4 || modules.isEmpty())
        return "";

    QString result;
    if (this->Version == QtVersion_Qt6)
    {
        result += "target_link_libraries(" + target_name + " PRIVATE";
        foreach (QString module, modules)
            result += " Qt6::" + Generic::CapitalFirst(module);
        result += ")\n";
    }
    else if (this->Version == QtVersion_Qt5)
    {
        result += "target_link_libraries(" + target_name + " PRIVATE";
        foreach (QString module, modules)
            result += " Qt5::" + Generic::CapitalFirst(module);
        result += ")\n";
    }
    else
    {
        // Handle auto-detection mode
        result += "IF (QT5BUILD)\n";
        result += Generic::Indent("target_link_libraries(" + target_name + " PRIVATE");
        foreach (QString module, modules)
            result += " Qt5::" + Generic::CapitalFirst(module);
        result += ")\n";
        result += "ELSE()\n";
        result += Generic::Indent("target_link_libraries(" + target_name + " ${QT_LIBRARIES})\n");
        result += "ENDIF()\n";
    }
    return result;
}

CMakeOption::CMakeOption(QString name, QString description, QString __default)
{
    this->Name = name;
    this->Description = description;
    this->Default = __default;
}
