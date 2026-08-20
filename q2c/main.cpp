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
#include <QRegularExpression>
#include <iostream>
#include "configuration.h"
#include "project.h"
#include "terminalparser.h"
#include "logs.h"

using namespace std;

static QString JsonEscape(QString value)
{
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    value.replace("\t", "\\t");
    return value;
}

static void PrintWarning(QString warning)
{
    int line = -1;
    QRegularExpression line_regex("\\bline\\s+(\\d+)\\b");
    QRegularExpressionMatch match = line_regex.match(warning);
    if (match.hasMatch())
        line = match.captured(1).toInt();

    QString file = Configuration::InputFile;
    if (Configuration::WarningFormat == "json")
    {
        cerr << "{\"type\":\"warning\",\"file\":\"" << JsonEscape(file).toStdString()
             << "\",\"line\":" << line
             << ",\"message\":\"" << JsonEscape(warning).toStdString() << "\"}" << endl;
        return;
    }

    if (line >= 0)
        cerr << file.toStdString() << ":" << line << ": warning: " << warning.toStdString() << endl;
    else
        cerr << file.toStdString() << ": warning: " << warning.toStdString() << endl;
}

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
    if (!Configuration::OutputFile.isEmpty() && Configuration::OutputDirectory.isEmpty())
        return true;

    QString output_name = Configuration::OutputFile;
    if (output_name.isEmpty() && Configuration::q2c)
    {
        output_name = "CMakeLists.txt";
    }
    else if (output_name.isEmpty())
    {
        QFileInfo file_info(Configuration::InputFile);
        QString base_name = file_info.completeBaseName();
        if (base_name.isEmpty())
        {
            Logs::ErrorLog("Unable to resolve output file name from: " + Configuration::InputFile);
            return false;
        }
        output_name = base_name + ".pro";
    }

    if (!Configuration::OutputDirectory.isEmpty())
    {
        QDir output_dir(Configuration::OutputDirectory);
        if (!output_dir.exists() && !output_dir.mkpath("."))
        {
            Logs::ErrorLog("Unable to create output directory: " + Configuration::OutputDirectory);
            return false;
        }
        output_name = output_dir.filePath(QFileInfo(output_name).fileName());
    }

    Configuration::OutputFile = output_name;
    Logs::DebugLog("Resolved output name to " + Configuration::OutputFile);
    return true;
}

static bool BackupExistingOutput(QString output_path)
{
    QFile output_file(output_path);
    if (!output_file.exists())
        return true;

    QString backup_path = output_path + ".bak";
    int suffix = 1;
    while (QFile::exists(backup_path))
    {
        backup_path = output_path + ".bak." + QString::number(suffix);
        suffix++;
    }

    if (!QFile::copy(output_path, backup_path))
    {
        Logs::ErrorLog("Unable to create backup file: " + backup_path);
        return false;
    }
    Logs::Log("Backed up existing output to " + backup_path);
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
    Logs::DebugLog(QString("Conversion direction: ") + (Configuration::q2c ? "qmake to CMake" : "CMake to qmake"));
    Logs::DebugLog("Input file: " + Configuration::InputFile, 2);
    if (!Configuration::OutputDirectory.isEmpty())
        Logs::DebugLog("Output directory: " + Configuration::OutputDirectory, 2);

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
        PrintWarning(warning);

    if (Configuration::strict && !project->GetModel().Warnings.isEmpty())
    {
        Logs::ErrorLog("Strict mode failed because conversion warnings were emitted");
        delete project;
        return TP_RESULT_FAIL;
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
    if (output_file.exists() && Configuration::backup)
    {
        if (!BackupExistingOutput(Configuration::OutputFile))
        {
            delete project;
            return TP_RESULT_FAIL;
        }
    }
    if ((!Configuration::force) && (!Configuration::backup) && output_file.exists())
    {
        Logs::ErrorLog("File " + Configuration::OutputFile + " already exists. Use -f/--force or --backup to overwrite it");
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
