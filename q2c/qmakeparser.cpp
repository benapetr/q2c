//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "qmakeparser.h"
#include "logs.h"

QMakeParser::QMakeParser()
{
    this->Model = nullptr;
    this->KnownSimpleKeywords << "TARGET" << "TEMPLATE";
    this->KnownComplexKeywords << "SOURCES" << "HEADERS" << "QT" << "CONFIG" << "DEFINES"
                               << "INCLUDEPATH" << "LIBS" << "FORMS" << "RESOURCES" << "SUBDIRS";
}

bool QMakeParser::Parse(QString text, BuildProject *model, QString source_file, QString cmake_minimum_version)
{
    this->Model = model;
    this->SourceFile = source_file;
    this->CMakeMinimumVersion = cmake_minimum_version;
    this->ProjectName = "";
    this->TemplateName = "app";
    this->Sources.clear();
    this->Headers.clear();
    this->Modules.clear();
    this->Config.clear();
    this->Defines.clear();
    this->IncludePaths.clear();
    this->Libraries.clear();
    this->UIFiles.clear();
    this->ResourceFiles.clear();
    this->Subdirectories.clear();
    this->ConditionalBlocks.clear();
    this->RequiredKeywords.clear();
    this->RemainingRequiredKeywords.clear();
    this->RequiredKeywords << "TARGET";
    this->RemainingRequiredKeywords = this->RequiredKeywords;
    this->Modules << "core";
    this->TargetType = BuildTarget_Application;
    this->CurrentLineNumber = 0;
    this->TargetLine = 0;
    this->IsSubdirsProject = false;

    ParserState state = ParserState_LookingForKeyword;
    QStringList lines = text.split("\n");
    QString data_buffer;
    QString current_word;
    QString current_line;

    for (int i = 0; i < lines.size(); i++)
    {
        this->CurrentLineNumber = i + 1;
        QString line = lines[i];
        while (line.startsWith(" "))
            line = line.mid(1);
        if (line.startsWith("#") || line.isEmpty())
            continue;

        if (line.startsWith("win32:") || line.startsWith("unix:") ||
            line.startsWith("linux:") || line.startsWith("macx:") ||
            line.trimmed().startsWith("if(") || line.trimmed().startsWith("else {") ||
            line.trimmed() == "else:" || line.trimmed().startsWith("} else"))
        {
            if (!this->ProcessScope(line, lines, i))
                return false;
            continue;
        }

        if (state == ParserState_LookingForKeyword)
        {
            QString keyword = line;
            if (keyword.contains(" "))
                keyword = keyword.mid(0, keyword.indexOf(" "));
            Logs::DebugLog("Possible keyword: " + keyword);
            if (this->KnownSimpleKeywords.contains(keyword))
            {
                if (!this->ProcessSimpleKeyword(keyword, line))
                    return false;
            }
            else if (this->KnownComplexKeywords.contains(keyword))
            {
                current_word = keyword;
                current_line = line;
                data_buffer = line;
                if (line.endsWith("\\"))
                {
                    state = ParserState_FetchingData;
                }
                else if (!this->ProcessComplexKeyword(keyword, current_line, data_buffer))
                {
                    return false;
                }
            }
            else
            {
                Logs::DebugLog("Ignoring unknown keyword: " + keyword);
            }
        }
        else if (state == ParserState_FetchingData)
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
            Logs::ErrorLog("Required keyword not found: " + word);
        return false;
    }

    this->RefreshModel();
    return true;
}

bool QMakeParser::ProcessScope(QString line, QStringList &lines, int &current_line)
{
    ConditionalBlock block;
    QString condition;
    block.line = current_line + 1;

    if (line.startsWith("win32:"))
        condition = "WIN32";
    else if (line.startsWith("unix:"))
        condition = "UNIX";
    else if (line.startsWith("linux:"))
        condition = "UNIX AND NOT APPLE";
    else if (line.startsWith("macx:"))
        condition = "APPLE";
    else if (line.trimmed().startsWith("if("))
    {
        condition = line.mid(line.indexOf("(") + 1);
        condition = condition.left(condition.indexOf(")"));
        condition = this->ParseCondition(condition);
    }

    block.condition = condition;
    block.active = this->EvaluateCondition(condition);

    int brace_count = 0;
    bool in_scope = true;
    while (in_scope && current_line < lines.size() - 1)
    {
        current_line++;
        QString current = lines[current_line].trimmed();

        if (current.startsWith("{"))
        {
            brace_count++;
            continue;
        }
        else if (current.startsWith("}"))
        {
            if (brace_count == 0)
                in_scope = false;
            brace_count--;
            continue;
        }

        if (block.active)
        {
            QString keyword = current;
            if (keyword.contains(" "))
                keyword = keyword.mid(0, keyword.indexOf(" "));

            if (keyword == "SOURCES")
                this->ParseStandardQMakeList(&block.Sources, current, current);
            else if (keyword == "HEADERS")
                this->ParseStandardQMakeList(&block.Headers, current, current);
            else if (keyword == "DEFINES")
                this->ParseStandardQMakeList(&block.Defines, current, current);
            else if (keyword == "INCLUDEPATH")
                this->ParseStandardQMakeList(&block.IncludePaths, current, current);
            else if (keyword == "LIBS")
                this->ParseStandardQMakeList(&block.Libraries, current, current);
            else if (keyword == "CONFIG")
                this->ParseStandardQMakeList(&block.Config, current, current);
        }
    }

    this->ConditionalBlocks.append(block);
    return true;
}

QString QMakeParser::ParseCondition(QString condition)
{
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

bool QMakeParser::EvaluateCondition(QString condition)
{
    Q_UNUSED(condition);
    return true;
}

bool QMakeParser::ParseStandardQMakeList(QList<QString> *list, QString line, QString text)
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
            list->removeAll(rm);
        return true;
    }

    text = text.mid(text.indexOf("=") + 1);
    text = text.replace("\n", " ");
    text = text.replace("\\", " ");
    if (!line.contains("+="))
        list->clear();

    QStringList items = text.split(" ", Qt::SkipEmptyParts);
    foreach (QString item, items)
    {
        if (!list->contains(item))
            list->append(item);
    }
    return true;
}

bool QMakeParser::ProcessSimpleKeyword(QString word, QString line)
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
        target_name.replace("\"", "");
        target_name = target_name.trimmed();
        target_name = target_name.replace(" ", "_");
        this->ProjectName = target_name;
        this->TargetLine = this->CurrentLineNumber;
    }
    else if (word == "TEMPLATE")
    {
        QString value = line.mid(line.indexOf("=") + 1).trimmed();
        this->TemplateName = value;
        this->TargetType = this->TargetTypeFromTemplate(value);
        if (value == "subdirs")
        {
            this->IsSubdirsProject = true;
            this->RequiredKeywords.removeAll("TARGET");
            this->RemainingRequiredKeywords.removeAll("TARGET");
        }
    }

    return true;
}

bool QMakeParser::ProcessComplexKeyword(QString word, QString line, QString data_buffer)
{
    if (this->RemainingRequiredKeywords.contains(word))
        this->RemainingRequiredKeywords.removeAll(word);

    if (word == "SOURCES")
        return this->ParseStandardQMakeList(&this->Sources, line, data_buffer);
    if (word == "HEADERS")
        return this->ParseStandardQMakeList(&this->Headers, line, data_buffer);
    if (word == "QT")
    {
        if (line.contains("-="))
        {
            QString modules_text = data_buffer.mid(data_buffer.indexOf("-=") + 2);
            modules_text = modules_text.replace("\n", " ").replace("\\", " ");
            QStringList modules_to_remove = modules_text.split(" ", Qt::SkipEmptyParts);
            foreach (QString module, modules_to_remove)
                this->Modules.removeAll(module);
            return true;
        }
        return this->ParseStandardQMakeList(&this->Modules, line, data_buffer);
    }
    if (word == "CONFIG")
        return this->ParseStandardQMakeList(&this->Config, line, data_buffer);
    if (word == "DEFINES")
        return this->ParseStandardQMakeList(&this->Defines, line, data_buffer);
    if (word == "INCLUDEPATH")
        return this->ParseStandardQMakeList(&this->IncludePaths, line, data_buffer);
    if (word == "LIBS")
        return this->ParseStandardQMakeList(&this->Libraries, line, data_buffer);
    if (word == "FORMS")
        return this->ParseStandardQMakeList(&this->UIFiles, line, data_buffer);
    if (word == "RESOURCES")
        return this->ParseStandardQMakeList(&this->ResourceFiles, line, data_buffer);
    if (word == "SUBDIRS")
        return this->ParseStandardQMakeList(&this->Subdirectories, line, data_buffer);

    return true;
}

BuildTargetType QMakeParser::TargetTypeFromTemplate(QString value)
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

BuildTargetType QMakeParser::TargetTypeFromConfig(BuildTargetType current_type) const
{
    if (this->Config.contains("testcase"))
        return BuildTarget_Test;
    if (this->Config.contains("plugin"))
        return BuildTarget_Plugin;
    return current_type;
}

void QMakeParser::RefreshModel()
{
    this->Model->Clear();
    this->Model->Name = this->ProjectName;
    this->Model->CMakeMinimumVersion = this->CMakeMinimumVersion;
    this->Model->GlobalConfig = this->Config;
    this->Model->GlobalQtModules = this->Modules;

    BuildTarget *target = this->Model->EnsurePrimaryTarget();
    target->Name = this->ProjectName;
    if (target->Name.isEmpty() && this->IsSubdirsProject)
        target->Name = "MainProject";
    target->Type = this->TargetTypeFromConfig(this->TargetType);
    target->Location = BuildSourceLocation(this->SourceFile, this->TargetLine);
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
        scope.Location = BuildSourceLocation(this->SourceFile, block.line);
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
        subtarget.Location = BuildSourceLocation(this->SourceFile, 0);
        this->Model->Targets.append(subtarget);
    }
}
