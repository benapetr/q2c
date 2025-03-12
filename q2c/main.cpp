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
        if (filename.toLower().endsWith(".pro"))
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

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // Parse options
    TerminalParser parser;

    if (!parser.Parse(argc, argv))
    {
        return TP_RESULT_SHUT;
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
    if (Configuration::OutputFile == "")
    {
        // Let's try to resolve it from input file
        if (!Configuration::InputFile.contains("."))
        {
            Logs::ErrorLog("Unable to resolve output file name from: " + Configuration::InputFile);
            return TP_RESULT_SHUT;
        }
        Configuration::OutputFile = Configuration::InputFile.mid(0, Configuration::InputFile.indexOf("."));
        if (Configuration::q2c)
        {
            Configuration::OutputFile = "CMakeLists.txt";
        } else
        {
            Configuration::OutputFile += ".pro";
        }
        Logs::DebugLog("Resolved output name to " + Configuration::OutputFile);
    }

    // Load the file
    QFile *file = new QFile(Configuration::InputFile);
    if (!file->open(QIODevice::ReadOnly))
    {
        Logs::ErrorLog("Unable to read: " + Configuration::InputFile);
        delete file;
        return TP_RESULT_FAIL;
    }
    QString input_text = QString(file->readAll());
    file->close();
    delete file;

    Project *project = new Project();
    if (!project->Load(input_text))
    {
        Logs::ErrorLog("Unable to parse: " + Configuration::InputFile);
        delete project;
        return TP_RESULT_FAIL;
    }
    file = new QFile(Configuration::OutputFile);
    if ((!Configuration::force) && file->exists())
    {
        Logs::ErrorLog("File " + Configuration::OutputFile + " already exist! I will not overwrite it unless you provide parameter -f for it");
        delete file;
        delete project;
        return TP_RESULT_FAIL;
    }
    if (!file->open(QIODevice::WriteOnly))
    {
        Logs::ErrorLog("Unable to open for writing: " + Configuration::OutputFile);
        delete file;
        delete project;
        return TP_RESULT_FAIL;
    }

    QString result;
    if (Configuration::q2c)
    {
        result = project->ToCmake();
    } else
    {
        result = project->ToQmake();
    }

    file->write(result.toUtf8());
    file->close();
    delete file;
    delete project;

    return 0;
}
