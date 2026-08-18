//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include "configuration.h"
#include "project.h"
#include "terminalparser.h"
#include "logs.h"

using namespace std;

static bool DetectInput()
{
    QStringList files;
    foreach (QString filename, QDir(".").entryList())
    {
        QString lower = filename.toLower();
        if (lower.endsWith(".pro") || lower.endsWith(".pri") || filename == "CMakeLists.txt" || lower.endsWith(".cmake"))
        {
            files.append(filename);
        }
    }

    if (files.count() == 0)
    {
        return false;
    }

    if (files.count() == 1)
    {
        Configuration::InputFile = files.at(0);
        return true;
    }

    cout << endl << "Following project files were found in current directory:" << endl;
    int x = 0;
    while (x < files.count())
    {
        cout << files.at(x).toStdString() << endl;
        x++;
    }

    return false;
}

static bool DetectDirection()
{
    if (Configuration::direction_explicit)
        return true;

    QFileInfo file_info(Configuration::InputFile);
    QString filename = file_info.fileName();
    QString lower = filename.toLower();

    if (lower.endsWith(".pro") || lower.endsWith(".pri"))
    {
        Configuration::q2c = true;
        return true;
    }
    if (filename == "CMakeLists.txt" || lower.endsWith(".cmake"))
    {
        Configuration::q2c = false;
        return true;
    }

    return false;
}

static bool ResolveOutputFile()
{
    if (!Configuration::OutputFile.isEmpty())
        return true;

    if (Configuration::q2c)
    {
        Configuration::OutputFile = "CMakeLists.txt";
        Logs::DebugLog("Resolved output name to " + Configuration::OutputFile);
        return true;
    }

    QFileInfo file_info(Configuration::InputFile);
    QString base_name = file_info.completeBaseName();
    if (base_name.isEmpty())
    {
        Logs::ErrorLog("Unable to resolve output file name from: " + Configuration::InputFile);
        return false;
    }

    Configuration::OutputFile = base_name + ".pro";
    Logs::DebugLog("Resolved output name to " + Configuration::OutputFile);
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("q2c");

    // Parse options
    TerminalParser parser;

    if (!parser.Parse(argc, argv))
    {
        return TP_RESULT_FAIL;
    }

    if (Configuration::exit_after_parse)
    {
        return Configuration::exit_code;
    }

    // Verbosity
    Logs::DebugLog("Verbosity: " + QString::number(Configuration::verbosity_level));

    if (Configuration::InputFile == "")
    {
        if (!DetectInput())
        {
            Logs::ErrorLog("No input file was provided");
            return TP_RESULT_SHUT;
        }
        Logs::DebugLog("Resolved input name to " + Configuration::InputFile);
    }

    if (!DetectDirection())
    {
        Logs::ErrorLog("Unable to detect conversion direction from input file: " + Configuration::InputFile);
        return TP_RESULT_FAIL;
    }

    // Load the file
    QFile file(Configuration::InputFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        Logs::ErrorLog("Unable to read: " + Configuration::InputFile);
        return TP_RESULT_FAIL;
    }
    QString input_text = QString(file.readAll());
    file.close();

    Project *project = new Project();
    if (!project->Load(input_text))
    {
        Logs::ErrorLog("Unable to parse: " + Configuration::InputFile);
        delete project;
        return TP_RESULT_FAIL;
    }
    foreach (QString warning, project->GetModel().Warnings)
    {
        Logs::Log("Warning: " + warning);
    }

    if (Configuration::check_only)
    {
        Logs::Log("Input parsed successfully: " + Configuration::InputFile);
        delete project;
        return TP_RESULT_OK;
    }

    QString result;
    if (Configuration::q2c)
    {
        result = project->ToCmake();
    } else
    {
        result = project->ToQmake();
    }

    if (Configuration::dry_run)
    {
        cout << result.toStdString();
        delete project;
        return TP_RESULT_OK;
    }

    if (!ResolveOutputFile())
    {
        delete project;
        return TP_RESULT_FAIL;
    }

    QFile output_file(Configuration::OutputFile);
    if ((!Configuration::force) && output_file.exists())
    {
        Logs::ErrorLog("File " + Configuration::OutputFile + " already exists. Use -f or --force to overwrite it");
        delete project;
        return TP_RESULT_FAIL;
    }
    if (!output_file.open(QIODevice::WriteOnly))
    {
        Logs::ErrorLog("Unable to open for writing: " + Configuration::OutputFile);
        delete project;
        return TP_RESULT_FAIL;
    }

    output_file.write(result.toUtf8());
    output_file.close();
    delete project;

    return 0;
}
