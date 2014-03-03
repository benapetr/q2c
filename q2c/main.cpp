//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <QCoreApplication>
#include <iostream>
#include <QDir>
#include "configuration.h"
#include "project.h"
#include "logs.h"
#include "terminalparser.h"

using namespace std;

//! Scan for input file and return true if it finds some
bool DetectInput()
{
    QDir d(QDir::currentPath());
    QStringList files = d.entryList();
    int x = 0;
    bool found = false;
    while (x < files.count())
    {
        QString filename = files.at(x);
        filename.toLower();
        if (filename.endsWith(".pro"))
        {
            if (found)
            {
                // more pro files exist, let's print error and quit
                Logs::ErrorLog("There are multiple .pro files in this directory, you need to explicitly provide");
                Logs::ErrorLog("the project you want to convert, see --help for more");
                return false;
            }
            found = true;
            Configuration::InputFile = files.at(x);
        }
        x++;
    }
    return found;
}

int main(int argc, char *argv[])
{
    int c = 0;
    QStringList args;
    while (c < argc)
    {
        args.append(QString(argv[c]));
        c++;
    }
    TerminalParser *tp = new TerminalParser(argc, args);
    if (tp->Parse())
    {
        // Parameter require to exit (--help) etc
        return 0;
    }
    Logs::DebugLog("Verbosity: " + QString::number(Configuration::Verbosity));
    if (Configuration::InputFile == "")
    {
        // user didn't provide any input file
        if (!DetectInput())
        {
            return 2;
        }
        Logs::DebugLog("Resolved input name to " + Configuration::InputFile);
    }
    if (Configuration::OutputFile == "")
    {
        // user didn't provide output file name
        // we can simply reuse the original name
        if (!Configuration::InputFile.contains("."))
        {
            Logs::ErrorLog("The input file can't be converted to output file, please provide output file name");
            return 3;
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
    // Load the project file
    QFile *file = new QFile(Configuration::InputFile);
    if (!file->open(QIODevice::ReadOnly))
    {
        Logs::ErrorLog("Unable to read: " + Configuration::InputFile);
        delete file;
        return 4;
    }
    QString source = QString(file->readAll());
    delete file;
    Project *input = new Project();
    if (!input->Load(source))
    {
        Logs::ErrorLog("Unable to parse: " + Configuration::InputFile);
        return 5;
    }
    file = new QFile(Configuration::OutputFile);
    if ((!Configuration::Forcing) && file->exists())
    {
        Logs::ErrorLog("File " + Configuration::OutputFile + " already exist! I will not overwrite it unless you provide parameter -f for it");
        delete file;
        return 6;
    }
    if (!file->open(QIODevice::ReadWrite))
    {
        Logs::ErrorLog("Unable to open for writing: " + Configuration::OutputFile);
        delete file;
        return 7;
    }
    if (Configuration::q2c)
    {
        file->write(input->ToCmake().toUtf8());
    } else
    {
        file->write(input->ToQmake().toUtf8());
    }
    delete file;
    delete input;
    return 0;
}
