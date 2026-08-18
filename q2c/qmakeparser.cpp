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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>

QMakeParser::QMakeParser()
{
    this->Model = nullptr;
    this->KnownSimpleKeywords << "TARGET" << "TEMPLATE";
    this->KnownComplexKeywords << "SOURCES" << "HEADERS" << "QT" << "CONFIG" << "DEFINES"
                               << "INCLUDEPATH" << "DEPENDPATH" << "LIBS" << "FORMS"
                               << "RESOURCES" << "TRANSLATIONS" << "SUBDIRS" << "INSTALLS"
                               << "QMAKE_CXXFLAGS" << "QMAKE_LFLAGS" << "QMAKE_POST_LINK"
                               << "DESTDIR" << "OBJECTS_DIR" << "MOC_DIR" << "RCC_DIR" << "UI_DIR";
}

bool QMakeParser::Parse(QString text, BuildProject *model, QString source_file, QString cmake_minimum_version)
{
    this->Model = model;
    this->SourceFile = source_file;
    QFileInfo source_info(source_file);
    this->BaseDirectory = source_info.absoluteDir().absolutePath();
    if (this->BaseDirectory.isEmpty())
        this->BaseDirectory = QDir::currentPath();
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
    this->Variables.clear();
    this->UIFiles.clear();
    this->ResourceFiles.clear();
    this->TranslationFiles.clear();
    this->CompileOptions.clear();
    this->LinkOptions.clear();
    this->InstallRules.clear();
    this->Subdirectories.clear();
    this->ConditionalBlocks.clear();
    this->RequiredKeywords.clear();
    this->RemainingRequiredKeywords.clear();
    this->RequiredKeywords << "TARGET";
    this->RemainingRequiredKeywords = this->RequiredKeywords;
    this->Modules << "core";
    this->Variables.insert("QT", this->Modules);
    this->Variables.insert("PWD", QStringList() << this->BaseDirectory);
    this->Variables.insert("OUT_PWD", QStringList() << ".");
    this->TargetType = BuildTarget_Application;
    this->CurrentLineNumber = 0;
    this->TargetLine = 0;
    this->IsSubdirsProject = false;

    QStringList lines = this->NormalizeLines(text);
    for (int i = 0; i < lines.size(); i++)
    {
        this->CurrentLineNumber = i + 1;
        QString line = this->StripComment(lines[i]).trimmed();
        if (line.isEmpty())
            continue;

        if ((line.contains("{") && (line.startsWith("win32") || line.startsWith("unix") ||
             line.startsWith("linux") || line.startsWith("macx") || line.startsWith("if("))) ||
            line.startsWith("else") || line.startsWith("} else"))
        {
            if (!this->ProcessScope(line, lines, i))
                return false;
            continue;
        }

        if (!this->ProcessLine(line))
            return false;
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

QStringList QMakeParser::NormalizeLines(QString text)
{
    QStringList physical_lines = text.split("\n");
    QStringList result;
    QString current;
    foreach (QString physical_line, physical_lines)
    {
        QString line = physical_line.trimmed();
        if (line.endsWith("\\"))
        {
            current += line.left(line.length() - 1) + " ";
            continue;
        }
        current += line;
        result << current;
        current.clear();
    }
    if (!current.isEmpty())
        result << current;
    return result;
}

QString QMakeParser::StripComment(QString line)
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

bool QMakeParser::ProcessLine(QString line)
{
    if (this->ProcessInclude(line))
        return true;

    int colon = this->FindScopeColon(line);
    if (colon > 0 && !line.left(colon).contains("="))
    {
        QString condition = line.left(colon).trimmed();
        QString scoped_line = line.mid(colon + 1).trimmed();
        if (!scoped_line.isEmpty() && scoped_line != "{")
            return this->ProcessInlineScope(condition, scoped_line, this->CurrentLineNumber);
    }

    QString word;
    QString op;
    QString data;
    if (!this->ExtractAssignment(line, &word, &op, &data))
    {
        if (line.contains("("))
            this->AddWarning("Unsupported qmake function or statement at line " + QString::number(this->CurrentLineNumber) + ": " + line);
        else
            Logs::DebugLog("Ignoring unknown qmake line: " + line);
        return true;
    }

    return this->ProcessAssignment(word, op, data);
}

bool QMakeParser::ExtractAssignment(QString line, QString *word, QString *op, QString *data)
{
    QStringList ops;
    ops << "+=" << "-=" << "*=" << "~=" << "=";
    foreach (QString candidate, ops)
    {
        int index = line.indexOf(candidate);
        if (index <= 0)
            continue;

        *word = line.left(index).trimmed();
        *op = candidate;
        *data = line.mid(index + candidate.length()).trimmed();
        return !word->isEmpty();
    }
    return false;
}

int QMakeParser::FindScopeColon(QString line)
{
    bool in_quote = false;
    QChar quote_char;
    int paren_depth = 0;

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
            continue;
        }
        if (in_quote)
            continue;
        if (ch == '(')
            paren_depth++;
        else if (ch == ')' && paren_depth > 0)
            paren_depth--;
        else if (ch == ':' && paren_depth == 0)
            return i;
    }
    return -1;
}

bool QMakeParser::ProcessInclude(QString line)
{
    QString trimmed = line.trimmed();
    if (!trimmed.startsWith("include(") || !trimmed.endsWith(")"))
        return false;

    QString include_path = trimmed.mid(QString("include(").length());
    include_path.chop(1);
    include_path = this->ExpandVariables(include_path.trimmed());
    include_path.replace("\"", "");
    include_path.replace("'", "");

    QString included_text = this->LoadIncludedFile(include_path);
    if (included_text.isEmpty())
    {
        this->AddWarning("Unable to read included qmake file: " + include_path);
        return true;
    }

    QString previous_source = this->SourceFile;
    QString previous_base = this->BaseDirectory;
    QFileInfo include_info(include_path);
    if (include_info.isRelative())
        include_info = QFileInfo(QDir(previous_base), include_path);
    this->SourceFile = include_info.absoluteFilePath();
    this->BaseDirectory = include_info.absoluteDir().absolutePath();

    QStringList lines = this->NormalizeLines(included_text);
    for (int i = 0; i < lines.size(); i++)
    {
        this->CurrentLineNumber = i + 1;
        QString included_line = this->StripComment(lines[i]).trimmed();
        if (!included_line.isEmpty() && !this->ProcessLine(included_line))
            return false;
    }

    this->SourceFile = previous_source;
    this->BaseDirectory = previous_base;
    return true;
}

QString QMakeParser::LoadIncludedFile(QString include_path)
{
    QFileInfo info(include_path);
    if (info.isRelative())
        info = QFileInfo(QDir(this->BaseDirectory), include_path);

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return "";
    QString text = QString(file.readAll());
    file.close();
    return text;
}

QString QMakeParser::ExpandVariables(QString text)
{
    text.replace("$$PWD", this->BaseDirectory);
    text.replace("$${PWD}", this->BaseDirectory);
    text.replace("$$OUT_PWD", ".");
    text.replace("$${OUT_PWD}", ".");

    QRegularExpression braced("\\$\\$\\{([^}]+)\\}");
    QRegularExpressionMatchIterator braced_it = braced.globalMatch(text);
    while (braced_it.hasNext())
    {
        QRegularExpressionMatch match = braced_it.next();
        QString name = match.captured(1);
        QString replacement;
        if (this->Variables.contains(name))
            replacement = this->Variables.value(name).join(" ");
        else
            replacement = QProcessEnvironment::systemEnvironment().value(name);
        text.replace(match.captured(0), replacement);
    }

    QRegularExpression simple("\\$\\$([A-Za-z_][A-Za-z0-9_]*)");
    QRegularExpressionMatchIterator simple_it = simple.globalMatch(text);
    while (simple_it.hasNext())
    {
        QRegularExpressionMatch match = simple_it.next();
        QString name = match.captured(1);
        QString replacement;
        if (this->Variables.contains(name))
            replacement = this->Variables.value(name).join(" ");
        else
            replacement = QProcessEnvironment::systemEnvironment().value(name);
        text.replace(match.captured(0), replacement);
    }
    return text;
}

QStringList QMakeParser::TokenizeValueList(QString text)
{
    QStringList raw_tokens;
    QString token;
    bool in_quote = false;
    QChar quote_char;

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
        if (ch.isSpace() && !in_quote)
        {
            if (!token.isEmpty())
            {
                raw_tokens << token;
                token.clear();
            }
            continue;
        }
        token += ch;
    }
    if (!token.isEmpty())
        raw_tokens << token;

    QStringList result;
    foreach (QString raw_token, raw_tokens)
        result.append(this->ExpandToken(raw_token));
    return result;
}

QStringList QMakeParser::ExpandToken(QString token)
{
    QRegularExpression braced("^\\$\\$\\{([^}]+)\\}$");
    QRegularExpression simple("^\\$\\$([A-Za-z_][A-Za-z0-9_]*)$");
    QRegularExpressionMatch match = braced.match(token);
    if (!match.hasMatch())
        match = simple.match(token);

    if (match.hasMatch())
    {
        QString name = match.captured(1);
        if (this->Variables.contains(name))
            return this->Variables.value(name);
        QString env_value = QProcessEnvironment::systemEnvironment().value(name);
        if (!env_value.isEmpty())
            return QStringList() << env_value;
        return QStringList();
    }

    return QStringList() << this->ExpandVariables(token);
}

bool QMakeParser::ApplyListOperation(QList<QString> *list, QString op, QStringList items)
{
    if (op == "=")
        list->clear();

    if (op == "-=")
    {
        foreach (QString item, items)
            list->removeAll(item);
        return true;
    }

    if (op == "~=")
    {
        this->AddWarning("Regex replacement operator '~=' is not supported at line " + QString::number(this->CurrentLineNumber));
        return true;
    }

    foreach (QString item, items)
    {
        if (op == "*=" && list->contains(item))
            continue;
        if (!list->contains(item))
            list->append(item);
    }
    return true;
}

bool QMakeParser::ParseStandardQMakeList(QList<QString> *list, QString line, QString text)
{
    QString word;
    QString op;
    QString data;
    if (!this->ExtractAssignment(text.isEmpty() ? line : text, &word, &op, &data))
    {
        Logs::ErrorLog("Syntax error: expected assignment operator");
        Logs::ErrorLog("Line: " + line);
        return false;
    }
    Q_UNUSED(word);
    return this->ApplyListOperation(list, op, this->TokenizeValueList(data));
}

bool QMakeParser::ProcessAssignment(QString word, QString op, QString data)
{
    word = word.trimmed();
    QString upper = word.toUpper();
    QStringList items = this->TokenizeValueList(data);

    if (this->RemainingRequiredKeywords.contains(upper))
        this->RemainingRequiredKeywords.removeAll(upper);

    if (upper == "TARGET")
    {
        QList<QString> target;
        this->ApplyListOperation(&target, "=", items);
        if (!target.isEmpty())
        {
            QString target_name = target.join("_").trimmed();
            target_name = target_name.replace(" ", "_");
            this->ProjectName = target_name;
            this->TargetLine = this->CurrentLineNumber;
            this->Variables.insert("TARGET", QStringList() << this->ProjectName);
        }
        return true;
    }

    if (upper == "TEMPLATE")
    {
        if (!items.isEmpty())
        {
            this->TemplateName = items.first();
            this->TargetType = this->TargetTypeFromTemplate(this->TemplateName);
            if (this->TemplateName == "subdirs")
            {
                this->IsSubdirsProject = true;
                this->RequiredKeywords.removeAll("TARGET");
                this->RemainingRequiredKeywords.removeAll("TARGET");
            }
        }
        return true;
    }

    QList<QString> *target_list = nullptr;
    if (upper == "SOURCES")
        target_list = &this->Sources;
    else if (upper == "HEADERS")
        target_list = &this->Headers;
    else if (upper == "QT")
        target_list = &this->Modules;
    else if (upper == "CONFIG")
        target_list = &this->Config;
    else if (upper == "DEFINES")
        target_list = &this->Defines;
    else if (upper == "INCLUDEPATH" || upper == "DEPENDPATH")
        target_list = &this->IncludePaths;
    else if (upper == "LIBS")
        target_list = &this->Libraries;
    else if (upper == "FORMS")
        target_list = &this->UIFiles;
    else if (upper == "RESOURCES")
        target_list = &this->ResourceFiles;
    else if (upper == "TRANSLATIONS")
        target_list = &this->TranslationFiles;
    else if (upper == "SUBDIRS")
        target_list = &this->Subdirectories;
    else if (upper == "INSTALLS")
        target_list = &this->InstallRules;
    else if (upper == "QMAKE_CXXFLAGS")
        target_list = &this->CompileOptions;
    else if (upper == "QMAKE_LFLAGS" || upper == "QMAKE_POST_LINK")
        target_list = &this->LinkOptions;

    if (target_list != nullptr)
    {
        this->ApplyListOperation(target_list, op, items);
        this->Variables.insert(upper, *target_list);
        return true;
    }

    if (upper == "DESTDIR" || upper == "OBJECTS_DIR" || upper == "MOC_DIR" || upper == "RCC_DIR" || upper == "UI_DIR")
    {
        QStringList existing = this->Variables.value(upper);
        this->ApplyListOperation(&existing, op, items);
        this->Variables.insert(upper, existing);
        return true;
    }

    QStringList existing = this->Variables.value(word);
    this->ApplyListOperation(&existing, op, items);
    this->Variables.insert(word, existing);
    return true;
}

bool QMakeParser::ProcessSimpleKeyword(QString word, QString line)
{
    QString parsed_word;
    QString op;
    QString data;
    if (!this->ExtractAssignment(line, &parsed_word, &op, &data))
        return false;
    Q_UNUSED(word);
    return this->ProcessAssignment(parsed_word, op, data);
}

bool QMakeParser::ProcessComplexKeyword(QString word, QString line, QString data_buffer)
{
    QString parsed_word;
    QString op;
    QString data;
    if (!this->ExtractAssignment(data_buffer, &parsed_word, &op, &data))
        return false;
    Q_UNUSED(word);
    Q_UNUSED(line);
    return this->ProcessAssignment(parsed_word, op, data);
}

bool QMakeParser::ProcessInlineScope(QString condition, QString scoped_line, int line_number)
{
    ConditionalBlock block;
    if (condition.trimmed() == "else")
        block.condition = "NOT " + (this->ConditionalBlocks.isEmpty() ? QString("FALSE") : this->ConditionalBlocks.last().condition);
    else
        block.condition = this->NormalizeCondition(condition);
    block.active = this->EvaluateCondition(block.condition);
    block.line = line_number;

    QString word;
    QString op;
    QString data;
    if (!this->ExtractAssignment(scoped_line, &word, &op, &data))
    {
        this->AddWarning("Unsupported scoped qmake statement at line " + QString::number(line_number) + ": " + scoped_line);
        this->ConditionalBlocks.append(block);
        return true;
    }

    QString upper = word.toUpper();
    QStringList items = this->TokenizeValueList(data);
    if (upper == "SOURCES")
        this->ApplyListOperation(&block.Sources, op, items);
    else if (upper == "HEADERS")
        this->ApplyListOperation(&block.Headers, op, items);
    else if (upper == "DEFINES")
        this->ApplyListOperation(&block.Defines, op, items);
    else if (upper == "INCLUDEPATH" || upper == "DEPENDPATH")
        this->ApplyListOperation(&block.IncludePaths, op, items);
    else if (upper == "LIBS")
        this->ApplyListOperation(&block.Libraries, op, items);
    else if (upper == "CONFIG")
        this->ApplyListOperation(&block.Config, op, items);
    else if (upper == "TRANSLATIONS")
        this->ApplyListOperation(&block.TranslationFiles, op, items);
    else if (upper == "QMAKE_CXXFLAGS")
        this->ApplyListOperation(&block.CompileOptions, op, items);
    else if (upper == "QMAKE_LFLAGS" || upper == "QMAKE_POST_LINK")
        this->ApplyListOperation(&block.LinkOptions, op, items);
    else if (upper == "INSTALLS")
        this->ApplyListOperation(&block.InstallRules, op, items);
    else
        this->AddWarning("Unsupported scoped qmake variable at line " + QString::number(line_number) + ": " + word);

    this->ConditionalBlocks.append(block);
    return true;
}

bool QMakeParser::ProcessScope(QString line, QStringList &lines, int &current_line)
{
    ConditionalBlock block;
    QString condition;
    block.line = current_line + 1;

    QString trimmed = line.trimmed();
    int colon = this->FindScopeColon(trimmed);
    if (colon > 0)
    {
        condition = trimmed.left(colon).trimmed();
        QString after_colon = trimmed.mid(colon + 1).trimmed();
        if (!after_colon.isEmpty() && after_colon != "{")
            return this->ProcessInlineScope(condition, after_colon, block.line);
    }
    else if (trimmed.startsWith("if("))
    {
        condition = trimmed.mid(trimmed.indexOf("(") + 1);
        condition = condition.left(condition.indexOf(")"));
    }
    else if (trimmed.contains("{"))
    {
        condition = trimmed.left(trimmed.indexOf("{")).trimmed();
    }
    else if (trimmed.startsWith("else"))
    {
        condition = "NOT " + (this->ConditionalBlocks.isEmpty() ? QString("FALSE") : this->ConditionalBlocks.last().condition);
    }

    block.condition = this->NormalizeCondition(condition);
    block.active = this->EvaluateCondition(block.condition);

    int brace_count = trimmed.contains("{") ? 1 : 0;
    bool in_scope = true;
    while (in_scope && current_line < lines.size() - 1)
    {
        current_line++;
        QString current = this->StripComment(lines[current_line]).trimmed();
        if (current.isEmpty())
            continue;

        if (current.contains("{"))
            brace_count++;
        if (current.startsWith("}"))
        {
            brace_count--;
            if (brace_count <= 0)
                in_scope = false;
            continue;
        }

        QString word;
        QString op;
        QString data;
        if (!this->ExtractAssignment(current, &word, &op, &data))
            continue;

        QString upper = word.toUpper();
        QStringList items = this->TokenizeValueList(data);
        if (upper == "SOURCES")
            this->ApplyListOperation(&block.Sources, op, items);
        else if (upper == "HEADERS")
            this->ApplyListOperation(&block.Headers, op, items);
        else if (upper == "DEFINES")
            this->ApplyListOperation(&block.Defines, op, items);
        else if (upper == "INCLUDEPATH" || upper == "DEPENDPATH")
            this->ApplyListOperation(&block.IncludePaths, op, items);
        else if (upper == "LIBS")
            this->ApplyListOperation(&block.Libraries, op, items);
        else if (upper == "CONFIG")
            this->ApplyListOperation(&block.Config, op, items);
        else if (upper == "TRANSLATIONS")
            this->ApplyListOperation(&block.TranslationFiles, op, items);
        else if (upper == "QMAKE_CXXFLAGS")
            this->ApplyListOperation(&block.CompileOptions, op, items);
        else if (upper == "QMAKE_LFLAGS" || upper == "QMAKE_POST_LINK")
            this->ApplyListOperation(&block.LinkOptions, op, items);
        else if (upper == "INSTALLS")
            this->ApplyListOperation(&block.InstallRules, op, items);
    }

    this->ConditionalBlocks.append(block);
    return true;
}

QString QMakeParser::ParseCondition(QString condition)
{
    condition = condition.trimmed();
    condition = condition.replace("$$QT_MAJOR_VERSION", "QT_VERSION_MAJOR");
    condition = condition.replace(">=", " GREATER_EQUAL ");
    condition = condition.replace("<=", " LESS_EQUAL ");
    condition = condition.replace(">", " GREATER ");
    condition = condition.replace("<", " LESS ");
    condition = condition.replace("==", " EQUAL ");
    condition = condition.replace("&&", " AND ");
    condition = condition.replace("||", " OR ");
    condition = condition.replace("!", " NOT ");

    if (condition.startsWith("contains(") || condition.startsWith("equals(") ||
        condition.startsWith("isEmpty(") || condition.startsWith("exists(") ||
        condition.startsWith("greaterThan(") || condition.startsWith("lessThan("))
    {
        this->AddWarning("Condition function kept as a raw expression at line " + QString::number(this->CurrentLineNumber) + ": " + condition);
    }
    return condition;
}

QString QMakeParser::NormalizeCondition(QString condition)
{
    condition = condition.trimmed();
    if (condition.endsWith("{"))
        condition.chop(1);
    if (condition.endsWith(":"))
        condition.chop(1);
    condition = condition.trimmed();

    if (condition == "win32")
        return "WIN32";
    if (condition == "unix")
        return "UNIX";
    if (condition == "linux")
        return "UNIX AND NOT APPLE";
    if (condition == "macx")
        return "APPLE";
    if (condition == "msvc")
        return "MSVC";
    if (condition == "gcc")
        return "CMAKE_COMPILER_IS_GNUCXX";
    return this->ParseCondition(condition);
}

bool QMakeParser::EvaluateCondition(QString condition)
{
    Q_UNUSED(condition);
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
    QList<QString> warnings = this->Model->Warnings;
    this->Model->Clear();
    this->Model->Warnings = warnings;
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
    target->TranslationFiles = this->TranslationFiles;
    target->QtModules = this->Modules;
    target->Config = this->Config;
    target->Defines = this->Defines;
    target->IncludePaths = this->IncludePaths;
    target->Libraries = this->Libraries;
    target->CompileOptions = this->CompileOptions;
    target->LinkOptions = this->LinkOptions;
    target->InstallRules = this->InstallRules;
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
        scope.TranslationFiles = block.TranslationFiles;
        scope.CompileOptions = block.CompileOptions;
        scope.LinkOptions = block.LinkOptions;
        scope.InstallRules = block.InstallRules;
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

void QMakeParser::AddWarning(QString warning)
{
    if (this->Model != nullptr)
        this->Model->AddWarning(warning);
    Logs::DebugLog(warning);
}
