//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "project.h"
#include <QStringList>

Project::Project()
{
    this->ProjectName = "";
    this->CMakeMinumumVersion = "VERSION 2.6";
    this->KnownSimpleKeywords << "TARGET";
    this->RequiredKeywords << "TARGET";
    this->KnownComplexKeywords << "SOURCES" << "HEADERS";
    this->RemainingRequiredKeywords = this->RequiredKeywords;
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
    this->ParseQmake(text);
    return true;
}

bool Project::ParseQmake(QString text)
{
    ParserState state = ParserState_LookingForKeyword;
    // Process the file line by line
    QStringList lines = text.split("\n");
    QString data_buffer;
    QString current_word;
    QString current_line;
    foreach (QString line, lines)
    {
        // Trim leading spaces
        while (line.startsWith(" "))
            line = line.mid(1);
        // If line starts with hash we can ignore it
        if (line.startsWith("#") || line.isEmpty())
            continue;
        if (state == ParserState_LookingForKeyword)
        {
            // we are now looking for a keyword
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
                    this->ProcessComplexKeyword(keyword, current_line, data_buffer);
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
                this->ProcessComplexKeyword(current_word, current_line, data_buffer);
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
    return true;
}

QString Project::ToQmake()
{
    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "TARGET = " + ProjectName;
    return source;
}

QString Project::ToCmake()
{
    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from qmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "cmake_minimum_required (" + this->CMakeMinumumVersion + ")\n";
    source += "project(" + ProjectName + ")\n";
    //! \todo Somewhere here we should generate options for CMake based on Qt version preference
    source += generateCMakeOptions(&this->CMakeOptions);

    // Sources, headers and so on
    if (!this->Sources.isEmpty())
    {
        source += "set(" + this->ProjectName + "_SOURCES";
        foreach (QString src, this->Sources)
        {
            source += " \"" + src + "\"";
        }
        source += ")\n";
    }
    if (!this->Headers.isEmpty())
    {
        source += "set(" + this->ProjectName + "_HEADERS";
        foreach (QString src, this->Headers)
        {
            source += " \"" + src + "\"";
        }
        source += ")\n";
    }
    source += "add_executable(" + this->ProjectName;
    if (!this->Sources.isEmpty())
        source += " ${" + this->ProjectName + "_SOURCES}";
    if (!this->Headers.isEmpty())
        source += " ${" + this->ProjectName + "_HEADERS}";
    source += ")\n";
    return source;
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
    if (!line.contains("+="))
    {
        // Wipe current buffer
        list->clear();
    }
    text = text.mid(text.indexOf("=") + 1);
    text = text.replace("\n", " ");
    text = text.replace("\\", " ");
    list->append(text.split(" ", QString::SkipEmptyParts));
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
    }
    return true;
}
