//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2017

#ifndef TERMINALPARSER_H
#define TERMINALPARSER_H

#define TP_RESULT_OK 0
#define TP_RESULT_SHUT 1

#include <QStringList>
#include <QList>
#include <QString>

class TerminalParser;
class TerminalItem;
typedef int (*TP_Callback) (TerminalParser*, QStringList);

class TerminalItem
{
    public:
        TerminalItem(char symbol, QString String, QString Help, int ParametersRequired, TP_Callback callback);
        QString GetHelp();
        char GetShort();
        QString GetLong();
        int GetParameters();
        int Exec(TerminalParser *parser, QStringList parameters);

    private:
        char ch;
        QString string;
        QString _help;
        int parameters_required;
        TP_Callback callb;
};

/*!
 * \brief The TerminalParser class processes the arguments passed in terminal
 */
class TerminalParser
{
    public:
        TerminalParser();
        bool Parse(int argc, char **argv);
        void Register(char ch, QString string, QString help, int parameters_required, TP_Callback callb);
        QList<TerminalItem> GetItems();
        TerminalItem *GetItem(char name);
        TerminalItem *GetItem(QString name);

    private:
        QList<TerminalItem> _items;
};

#endif // TERMINALPARSER_H
