//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2017

#include <QCoreApplication>
#include <iostream>
#include "generic.h"
#include "terminalparser.h"
#include "configuration.h"

static int Parser_Input(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    if (params.isEmpty())
        return TP_RESULT_FAIL;

    Configuration::InputFile = params.at(0);
    return TP_RESULT_OK;
}

static int Parser_Output(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    if (params.isEmpty())
        return TP_RESULT_FAIL;

    Configuration::OutputFile = params.at(0);
    return TP_RESULT_OK;
}

static int Parser_Debug(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::debug = true;
    return TP_RESULT_OK;
}

static int Parser_Qt4(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::only_qt4 = true;
    return TP_RESULT_OK;
}

static int Parser_Qt5(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::only_qt5 = true;
    return TP_RESULT_OK;
}

static int Parser_Qt6(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::only_qt6 = true;
    return TP_RESULT_OK;
}

static int Parser_Force(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::force = true;
    return TP_RESULT_OK;
}

static int Parser_Verbosity(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(parser);
    Q_UNUSED(params);
    Configuration::verbosity_level++;
    return TP_RESULT_OK;
}

static int PrintHelp(TerminalParser *parser, QStringList params)
{
    Q_UNUSED(params);

    std::cout << QCoreApplication::applicationName().toStdString() << " version " << "1.0.0" << std::endl << std::endl
              << "Following options can be used:" << std::endl << std::endl;

    // first we analyse the list of options and get the size of longest one
    int longest = 10;

    foreach (TerminalItem i, parser->GetItems())
    {
        int is = i.GetLong().size() + 4;
        if (is > longest)
            longest = is;
    }

    int longest_line = 20;
    QStringList lines_param, lines_help;

    foreach (TerminalItem i, parser->GetItems())
    {
        QString parameters = i.GetLong();
        if (!parameters.isEmpty())
            parameters = Generic::ExpandedString("--" + parameters, longest, longest + 10);
        QString parameters_short;
        if (i.GetShort() != 0)
            parameters_short = "-" + QString(QChar(i.GetShort()));
        if (!parameters.isEmpty() && !parameters_short.isEmpty())
            parameters += " | " + parameters_short;
        else if (!parameters_short.isEmpty())
            parameters += parameters_short;
        if (i.GetParameters() > 0)
        {
            if (i.GetParameters() == 1)
                parameters += " <required parameter>";
            else
                parameters += " <" + QString::number(i.GetParameters()) + " required parameters>";
        }
        lines_help << i.GetHelp();
        lines_param << parameters;

        int size = parameters.size() + 2;
        if (size > longest_line)
            longest_line = size;
    }

    int item = 0;
    while (item < lines_param.size())
    {
        std::cout << "  " << Generic::ExpandedString(lines_param[item], longest_line, longest_line + 10).toStdString() << ": " << lines_help[item].toStdString() << std::endl;
        // let's go next
        item++;
    }

    std::cout << std::endl;
    std::cout << "This software is open source, contribute at http://github.com/benapetr/q2c" << std::endl;

    return TP_RESULT_SHUT;
}

TerminalParser::TerminalParser()
{
    this->Register('h', "help", "Display help", 0, (TP_Callback)PrintHelp);
    this->Register('v', "verbose", "Increase verbosity level", 0, (TP_Callback)Parser_Verbosity);
    this->Register('d', "debug", "Enable debug output", 0, (TP_Callback)Parser_Debug);
    this->Register('4', "qt4", "Generate Qt4 compatible CMake file", 0, (TP_Callback)Parser_Qt4);
    this->Register('5', "qt5", "Generate Qt5 compatible CMake file", 0, (TP_Callback)Parser_Qt5);
    this->Register('6', "qt6", "Generate Qt6 compatible CMake file", 0, (TP_Callback)Parser_Qt6);
    this->Register('f', "force", "Force overwrite of existing files", 0, (TP_Callback)Parser_Force);
    this->Register('i', "input", "Input file to load", 1, (TP_Callback)Parser_Input);
    this->Register('o', "output", "Output file", 1, (TP_Callback)Parser_Output);
}

bool TerminalParser::Parse(int argc, char **argv)
{
    bool looking_for_input = true;
    bool looking_for_output = true;
    int curr = 1;
    while (curr < argc)
    {
        QString parameter = QString(argv[curr]);
        if (parameter.startsWith("--"))
        {
            QString p = parameter.mid(2);
            TerminalItem *item = this->GetItem(p);
            if (item != nullptr)
            {
                QStringList parameters;
                // If this option requires parameters, gather them
                int required = item->GetParameters();
                while (required > 0 && curr + 1 < argc)
                {
                    curr++;
                    parameters.append(QString(argv[curr]));
                    required--;
                }
                if (required > 0)
                {
                    // Error not enough parameters
                    std::cerr << "Not enough parameters for option: " << p.toStdString() << std::endl;
                    delete item;
                    return false;
                }
                int result = item->Exec(this, parameters);
                delete item;
                if (result != TP_RESULT_OK)
                    return false;
            }
        } else if (parameter.startsWith("-"))
        {
            if (parameter.length() != 2)
            {
                std::cerr << "Invalid parameter: " << parameter.toStdString() << " (multiple characters)" << std::endl;
                return false;
            }
            TerminalItem *item = this->GetItem(parameter[1].toLatin1());
            if (item != nullptr)
            {
                QStringList parameters;
                int required = item->GetParameters();
                while (required > 0 && curr + 1 < argc)
                {
                    curr++;
                    parameters.append(QString(argv[curr]));
                    required--;
                }
                if (required > 0)
                {
                    std::cerr << "Not enough parameters for option: " << parameter.toStdString() << std::endl;
                    delete item;
                    return false;
                }
                int result = item->Exec(this, parameters);
                delete item;
                if (result != TP_RESULT_OK)
                    return false;
            }
        } else if (looking_for_input)
        {
            input_name = parameter;
            looking_for_input = false;
        } else if (looking_for_output)
        {
            output_name = parameter;
            looking_for_output = false;
        }
        curr++;
    }
    return true;
}

int TerminalItem::Exec(TerminalParser *parser, QStringList parameters)
{
    return this->callb(parser, parameters);
}

TerminalItem *TerminalParser::GetItem(QString name)
{
    foreach(TerminalItem x, this->_items)
    {
        if (x.GetLong() == name)
            return new TerminalItem(x);
    }
    return NULL;
}

void TerminalParser::Register(char ch, QString string, QString help, int parameters_required, TP_Callback callb)
{
    this->_items.append(TerminalItem(ch, string, help, parameters_required, callb));
}

QList<TerminalItem> TerminalParser::GetItems()
{
    return this->_items;
}

TerminalItem *TerminalParser::GetItem(char name)
{
    foreach(TerminalItem x, this->_items)
    {
        if (x.GetShort() == name)
            return new TerminalItem(x);
    }
    return NULL;
}

TerminalItem::TerminalItem(char symbol, QString String, QString Help, int ParametersRequired, TP_Callback callback)
{
    this->callb = callback;
    this->parameters_required = ParametersRequired;
    this->ch = symbol;
    this->string = String;
    this->_help = Help;
}

QString TerminalItem::GetHelp()
{
    return this->_help;
}

char TerminalItem::GetShort()
{
    return this->ch;
}

QString TerminalItem::GetLong()
{
    return this->string;
}

int TerminalItem::GetParameters()
{
    return this->parameters_required;
}
