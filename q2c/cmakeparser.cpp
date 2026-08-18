//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "cmakeparser.h"
#include "logs.h"
#include <QFileInfo>
#include <QRegularExpression>

CMakeParser::CMakeParser()
{
    this->Model = nullptr;
}

bool CMakeParser::Parse(QString text, BuildProject *model, QString source_file)
{
    this->Model = model;
    this->SourceFile = source_file;
    this->Variables.clear();
    this->ConditionStack.clear();
    this->Model->Clear();
    this->Model->CMakeMinimumVersion = "VERSION 3.1.0";

    QList<CMakeCommand> commands = this->ParseCommands(text);
    foreach (const CMakeCommand &command, commands)
        this->ProcessCommand(command);

    if (this->Model->Name.isEmpty())
    {
        BuildTarget *target = this->Model->PrimaryTarget();
        if (target != nullptr)
            this->Model->Name = target->Name;
    }
    return true;
}

QList<CMakeCommand> CMakeParser::ParseCommands(QString text)
{
    QList<CMakeCommand> commands;
    QString buffer;
    int paren_depth = 0;
    int start_line = 0;
    QStringList lines = text.split("\n");

    for (int i = 0; i < lines.size(); i++)
    {
        QString line = this->StripComment(lines[i]).trimmed();
        if (line.isEmpty())
            continue;

        if (buffer.isEmpty())
            start_line = i + 1;
        buffer += line + "\n";

        bool in_quote = false;
        QChar quote_char;
        for (int j = 0; j < line.length(); j++)
        {
            QChar ch = line[j];
            if ((ch == '"' || ch == '\'') && (j == 0 || line[j - 1] != '\\'))
            {
                if (in_quote && ch == quote_char)
                    in_quote = false;
                else if (!in_quote)
                {
                    in_quote = true;
                    quote_char = ch;
                }
                continue;
            }
            if (in_quote)
                continue;
            if (ch == '(')
                paren_depth++;
            else if (ch == ')' && paren_depth > 0)
                paren_depth--;
        }

        if (paren_depth == 0 && buffer.contains("("))
        {
            int open = buffer.indexOf("(");
            int close = buffer.lastIndexOf(")");
            if (open > 0 && close > open)
            {
                CMakeCommand command;
                command.Name = buffer.left(open).trimmed().toLower();
                command.Arguments = this->TokenizeArguments(buffer.mid(open + 1, close - open - 1));
                command.Line = start_line;
                commands.append(command);
            }
            buffer.clear();
        }
    }

    if (!buffer.trimmed().isEmpty())
        this->AddWarning("Unterminated CMake command near line " + QString::number(start_line));
    return commands;
}

QString CMakeParser::StripComment(QString line)
{
    bool in_quote = false;
    QChar quote_char;
    for (int i = 0; i < line.length(); i++)
    {
        QChar ch = line[i];
        if ((ch == '"' || ch == '\'') && (i == 0 || line[i - 1] != '\\'))
        {
            if (in_quote && ch == quote_char)
                in_quote = false;
            else if (!in_quote)
            {
                in_quote = true;
                quote_char = ch;
            }
        }
        else if (ch == '#' && !in_quote)
        {
            return line.left(i);
        }
    }
    return line;
}

QStringList CMakeParser::TokenizeArguments(QString text)
{
    QStringList tokens;
    QString current;
    bool in_quote = false;
    QChar quote_char;
    int bracket_depth = 0;

    for (int i = 0; i < text.length(); i++)
    {
        QChar ch = text[i];
        if ((ch == '"' || ch == '\'') && (i == 0 || text[i - 1] != '\\'))
        {
            if (in_quote && ch == quote_char)
                in_quote = false;
            else if (!in_quote)
            {
                in_quote = true;
                quote_char = ch;
            }
            continue;
        }
        if (!in_quote && ch == '$' && i + 1 < text.length() && text[i + 1] == '<')
            bracket_depth++;
        else if (!in_quote && ch == '>' && bracket_depth > 0)
            bracket_depth--;

        if (ch.isSpace() && !in_quote && bracket_depth == 0)
        {
            if (!current.isEmpty())
            {
                tokens.append(current);
                current.clear();
            }
            continue;
        }
        current += ch;
    }
    if (!current.isEmpty())
        tokens.append(current);
    return tokens;
}

QString CMakeParser::ExpandVariables(QString text)
{
    QRegularExpression variable("\\$\\{([^}]+)\\}");
    QRegularExpressionMatchIterator it = variable.globalMatch(text);
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString name = match.captured(1);
        QString replacement = this->Variables.value(name).join(" ");
        text.replace(match.captured(0), replacement);
    }
    return text;
}

void CMakeParser::ProcessCommand(const CMakeCommand &command)
{
    QString name = command.Name;
    QStringList args = command.Arguments;
    if (name == "if")
    {
        this->ConditionStack.append(this->NormalizeCondition(args));
        return;
    }
    if (name == "else")
    {
        QString previous = this->ConditionStack.isEmpty() ? QString("FALSE") : this->ConditionStack.takeLast();
        this->ConditionStack.append("NOT " + previous);
        return;
    }
    if (name == "endif")
    {
        if (!this->ConditionStack.isEmpty())
            this->ConditionStack.removeLast();
        return;
    }
    if (name == "cmake_minimum_required")
    {
        if (!args.isEmpty())
            this->Model->CMakeMinimumVersion = args.join(" ");
        return;
    }
    if (name == "project")
    {
        if (!args.isEmpty())
            this->Model->Name = args.first();
        return;
    }
    if (name == "set")
    {
        if (args.isEmpty())
            return;
        QString variable = args.takeFirst();
        this->Variables.insert(variable, args);
        return;
    }
    if (name == "find_package")
    {
        if (args.isEmpty())
            return;
        QString package = args.first();
        if (package == "Qt4" || package == "Qt5" || package == "Qt6" || package == "Qt")
        {
            int components = args.indexOf("COMPONENTS");
            if (components >= 0)
            {
                for (int i = components + 1; i < args.size(); i++)
                {
                    if (args[i] == "REQUIRED" || args[i] == "OPTIONAL_COMPONENTS")
                        break;
                    this->AddUnique(&this->Model->GlobalQtModules, args[i].toLower());
                }
            }
        }
        return;
    }
    if (name == "add_executable" || name == "qt_add_executable")
    {
        if (args.isEmpty())
            return;
        QString target_name = args.takeFirst();
        BuildTarget *target = this->FindOrCreateTarget(target_name, BuildTarget_Application);
        target->Location = BuildSourceLocation(this->SourceFile, command.Line);
        this->ProcessTargetFiles(target, args);
        return;
    }
    if (name == "add_library" || name == "qt_add_library")
    {
        if (args.isEmpty())
            return;
        QString target_name = args.takeFirst();
        BuildTarget *target = this->FindOrCreateTarget(target_name, BuildTarget_Library);
        target->Location = BuildSourceLocation(this->SourceFile, command.Line);
        if (!args.isEmpty() && (args.first() == "STATIC" || args.first() == "SHARED" || args.first() == "MODULE" || args.first() == "OBJECT" || args.first() == "INTERFACE"))
            args.removeFirst();
        this->ProcessTargetFiles(target, args);
        return;
    }
    if (name == "add_subdirectory")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->Model->EnsurePrimaryTarget();
        if (target->Name.isEmpty())
        {
            target->Name = this->Model->Name.isEmpty() ? QString("MainProject") : this->Model->Name;
            target->Type = BuildTarget_Subdirs;
        }
        this->AddUnique(&target->Subdirectories, args.first());
        BuildTarget subdir;
        subdir.Name = args.first();
        subdir.Type = BuildTarget_Subdirs;
        subdir.Location = BuildSourceLocation(this->SourceFile, command.Line);
        this->Model->Targets.append(subdir);
        return;
    }
    if (name == "target_sources")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        this->ProcessTargetFiles(target, args);
        return;
    }
    if (name == "target_link_libraries")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        QStringList libs;
        foreach (QString arg, args)
        {
            if (this->IsVisibilityKeyword(arg))
                continue;
            if (this->IsQtImportedTarget(arg))
                this->AddUnique(&target->QtModules, this->QtModuleFromImportedTarget(arg));
            else
                libs << arg;
        }
        this->ProcessTargetList(target, &target->Libraries, libs);
        return;
    }
    if (name == "target_include_directories")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        this->ProcessTargetList(target, &target->IncludePaths, args);
        return;
    }
    if (name == "target_compile_definitions")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        this->ProcessTargetList(target, &target->Defines, args);
        return;
    }
    if (name == "target_compile_options")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        this->ProcessTargetList(target, &target->CompileOptions, args);
        return;
    }
    if (name == "target_link_options")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        this->ProcessTargetList(target, &target->LinkOptions, args);
        return;
    }
    if (name == "qt_wrap_cpp" || name == "qt5_wrap_cpp" || name == "qt6_wrap_cpp" ||
        name == "qt_add_resources" || name == "qt5_add_resources" || name == "qt6_add_resources")
    {
        this->AddWarning("Qt helper command parsed as generated-output hint at line " + QString::number(command.Line) + ": " + name);
        return;
    }
    if (name == "qt_add_translations")
    {
        if (args.isEmpty())
            return;
        BuildTarget *target = this->FindOrCreateTarget(args.takeFirst(), BuildTarget_Application);
        int ts_index = args.indexOf("TS_FILES");
        if (ts_index >= 0)
        {
            for (int i = ts_index + 1; i < args.size(); i++)
                this->AddUnique(&target->TranslationFiles, args[i]);
        }
        else
        {
            this->ProcessTargetList(target, &target->TranslationFiles, args);
        }
        return;
    }
    if (name == "set_target_properties")
    {
        this->AddWarning("set_target_properties is not fully represented at line " + QString::number(command.Line));
        return;
    }
}

QString CMakeParser::NormalizeCondition(QStringList args) const
{
    return args.join(" ");
}

BuildTarget *CMakeParser::FindOrCreateTarget(QString name, BuildTargetType type)
{
    BuildTarget *target = this->FindTarget(name);
    if (target != nullptr)
    {
        if (target->Type == BuildTarget_Unknown || target->Type == BuildTarget_Subdirs)
            target->Type = type;
        foreach (QString module, this->Model->GlobalQtModules)
            this->AddUnique(&target->QtModules, module);
        return target;
    }

    BuildTarget *primary = this->Model->PrimaryTarget();
    if (primary != nullptr && primary->Name.isEmpty())
    {
        primary->Name = name;
        primary->Type = type;
        foreach (QString module, this->Model->GlobalQtModules)
            this->AddUnique(&primary->QtModules, module);
        if (this->Model->Name.isEmpty())
            this->Model->Name = name;
        return primary;
    }

    BuildTarget new_target;
    new_target.Name = name;
    new_target.Type = type;
    new_target.QtModules = this->Model->GlobalQtModules;
    this->Model->Targets.append(new_target);
    if (this->Model->Name.isEmpty())
        this->Model->Name = name;
    return &this->Model->Targets[this->Model->Targets.size() - 1];
}

BuildTarget *CMakeParser::FindTarget(QString name)
{
    for (int i = 0; i < this->Model->Targets.size(); i++)
    {
        if (this->Model->Targets[i].Name == name)
            return &this->Model->Targets[i];
    }
    return nullptr;
}

BuildTarget *CMakeParser::PrimaryTarget()
{
    return this->Model->EnsurePrimaryTarget();
}

void CMakeParser::ProcessTargetFiles(BuildTarget *target, QStringList args)
{
    bool scoped = !this->ConditionStack.isEmpty();
    BuildConditionalScope scope;
    if (scoped)
        scope.Condition = this->ConditionStack.join(" AND ");

    foreach (QString raw_arg, args)
    {
        QStringList expanded_args;
        if (raw_arg.contains("${"))
            expanded_args = this->ExpandVariables(raw_arg).split(" ", Qt::SkipEmptyParts);
        else
            expanded_args << raw_arg;

        foreach (QString arg, expanded_args)
        {
        if (this->IsVisibilityKeyword(arg) || arg == "WIN32" || arg == "MACOSX_BUNDLE" || arg == "EXCLUDE_FROM_ALL")
            continue;
        if (scoped)
        {
            if (arg.endsWith(".h") || arg.endsWith(".hpp") || arg.endsWith(".hh"))
                this->AddUnique(&scope.Headers, arg);
            else if (!arg.endsWith(".ui") && !arg.endsWith(".qrc") && !arg.endsWith(".ts") && !arg.startsWith("$<"))
                this->AddUnique(&scope.Sources, arg);
            continue;
        }
        else if (arg.endsWith(".h") || arg.endsWith(".hpp") || arg.endsWith(".hh"))
            this->AddUnique(&target->Headers, arg);
        else if (arg.endsWith(".ui"))
            this->AddUnique(&target->UiFiles, arg);
        else if (arg.endsWith(".qrc"))
            this->AddUnique(&target->ResourceFiles, arg);
        else if (arg.endsWith(".ts"))
            this->AddUnique(&target->TranslationFiles, arg);
        else if (!arg.startsWith("$<"))
            this->AddUnique(&target->Sources, arg);
        }
    }

    if (scoped)
        target->ConditionalScopes.append(scope);
}

void CMakeParser::ProcessTargetList(BuildTarget *target, QList<QString> *list, QStringList args)
{
    bool scoped = !this->ConditionStack.isEmpty();
    BuildConditionalScope scope;
    if (scoped)
        scope.Condition = this->ConditionStack.join(" AND ");

    foreach (QString raw_arg, args)
    {
        QStringList expanded_args;
        if (raw_arg.contains("${"))
            expanded_args = this->ExpandVariables(raw_arg).split(" ", Qt::SkipEmptyParts);
        else
            expanded_args << raw_arg;

        foreach (QString arg, expanded_args)
        {
        if (this->IsVisibilityKeyword(arg))
            continue;
        if (!scoped)
            this->AddUnique(list, arg);
        else if (list == &target->Defines)
            this->AddUnique(&scope.Defines, arg);
        else if (list == &target->IncludePaths)
            this->AddUnique(&scope.IncludePaths, arg);
        else if (list == &target->Libraries)
            this->AddUnique(&scope.Libraries, arg);
        else if (list == &target->CompileOptions)
            this->AddUnique(&scope.CompileOptions, arg);
        else if (list == &target->LinkOptions)
            this->AddUnique(&scope.LinkOptions, arg);
        }
    }

    if (scoped)
        target->ConditionalScopes.append(scope);
}

void CMakeParser::AddUnique(QList<QString> *list, QString value)
{
    if (!value.isEmpty() && !list->contains(value))
        list->append(value);
}

void CMakeParser::AddWarning(QString warning)
{
    if (this->Model != nullptr)
        this->Model->AddWarning(warning);
    Logs::DebugLog(warning);
}

bool CMakeParser::IsVisibilityKeyword(QString value) const
{
    return value == "PRIVATE" || value == "PUBLIC" || value == "INTERFACE";
}

bool CMakeParser::IsQtImportedTarget(QString value) const
{
    return value.startsWith("Qt4::") || value.startsWith("Qt5::") || value.startsWith("Qt6::") || value.startsWith("Qt::");
}

QString CMakeParser::QtModuleFromImportedTarget(QString value) const
{
    QString module = value.mid(value.indexOf("::") + 2);
    return module.toLower();
}
