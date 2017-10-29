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
}

bool TerminalParser::Parse(int argc, char **argv)
{
    int x = 0;
    int expected_parameters = 0;
    QStringList parameter_buffer;
    TerminalItem *item = NULL;
    while (x < argc)
    {
        QString parameter = QString(argv[x++]);
        if (expected_parameters > 0)
        {
            // the last argument we processed has some parameters, so we are now processing them into a buffer
            parameter_buffer << parameter;
            expected_parameters--;
            if (expected_parameters == 0)
            {
                // execute
                if (item->Exec(this, parameter_buffer) == TP_RESULT_SHUT)
                {
                    delete item;
                    return false;
                }
                parameter_buffer.clear();
            }
            continue;
        }
        if (parameter.length() < 2)
        {
            std::cerr << "ERROR: unrecognized parameter: " << parameter.toStdString() << std::endl;
            return false;
        }
        if (parameter.startsWith("--"))
        {
            // It's a word parameter
            delete item;
            item = this->GetItem(parameter.mid(2));
            if (item == NULL)
            {
                std::cerr << "ERROR: unrecognized parameter: " << parameter.toStdString() << std::endl;
                delete item;
                return false;
            }
            expected_parameters = item->GetParameters();
            if (expected_parameters)
                continue;
            // let's process this item
            if (item->Exec(this, parameter_buffer) == TP_RESULT_SHUT)
            {
                delete item;
                return false;
            }
        }
        else if (parameter.startsWith("-"))
        {
            // It's a single character(s), let's process them recursively
            int symbol_px = 0;
            while (parameter.size() > ++symbol_px)
            {
                char sx = parameter[symbol_px].toLatin1();
                delete item;
                item = this->GetItem(sx);
                if (item == NULL)
                {
                    std::cerr << "ERROR: unrecognized parameter: -" << sx << std::endl;
                    delete item;
                    return false;
                }
                expected_parameters = item->GetParameters();
                if (expected_parameters && (symbol_px+1) < parameter.size())
                {
                    std::cerr << "ERROR: not enough parameters provided for -" << sx << std::endl;
                    delete item;
                    return false;
                }
                if (expected_parameters)
                    continue;
                // let's process this item
                if (item->Exec(this, parameter_buffer) == TP_RESULT_SHUT)
                {
                    delete item;
                    return false;
                }
            }
        }
    }
    delete item;
    if (expected_parameters)
    {
        std::cerr << "ERROR: missing parameter" << std::endl;
        return false;
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
