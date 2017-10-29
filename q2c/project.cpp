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
    QString source = "#-------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-------------------------------------------------\n";
    source += "TARGET = " + ProjectName;
    return source;
}

QString Project::ToCmake()
{
    QString source = "#-------------------------------------------------\n";
    source += "# Project converted from qmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-------------------------------------------------\n";
    source += "cmake_minimum_required (VERSION 2.6)\n";
    source += "project(" + ProjectName + ")\n";
    //! \todo Somewhere here we should generate options for CMake based on Qt version preference
    source += generateCMakeOptions(&this->CMakeOptions);
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
        if (!line.contains("="))
        {
            Logs::ErrorLog("Syntax error: expected '=' or '+=', neither of these 2 found");
            Logs::ErrorLog("Line: " + line);
            return false;
        }
        if (!line.contains("+="))
        {
            // Wipe current buffer
            this->Sources.clear();
        }
        data_buffer = data_buffer.mid(data_buffer.indexOf("=") + 1);
        data_buffer = data_buffer.replace("\n", " ");
        data_buffer = data_buffer.replace("\\", " ");
        this->Sources << data_buffer.split(" ", QString::SkipEmptyParts);
    } else if (word == "HEADERS")
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
            this->Headers.clear();
        }
        data_buffer = data_buffer.mid(data_buffer.indexOf("=") + 1);
        data_buffer = data_buffer.replace("\n", " ");
        data_buffer = data_buffer.replace("\\", " ");
        this->Headers << data_buffer.split(" ", QString::SkipEmptyParts);
    }
    return true;
}
