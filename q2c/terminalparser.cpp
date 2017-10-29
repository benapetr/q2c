//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "terminalparser.h"
using namespace std;

TerminalParser::TerminalParser(int argc_, QStringList argv)
{
    this->argc = argc_;
    this->Silent = false;
    this->args = argv;
}

bool TerminalParser::Parse()
{
    int x = 1;
    while (x < this->args.count())
    {
        bool valid = false;
        QString text = this->args.at(x);
        if (text == "-h" || text == "--help")
        {
            DisplayHelp();
            return true;
        }
        if (!text.startsWith("--") && text.startsWith("-"))
        {
            text = text.mid(1);
            while (text.length())
            {
                if (this->ParseChar(text.at(0)))
                {
                    return true;
                }
                text = text.mid(1);
            }
            valid = true;
        }
        if (!valid)
        {
            if (!this->Silent)
            {
                cout << (QString("This parameter isn't valid: ") + text).toStdString() << endl;
            }
            return true;
        }
        x++;
    }
    return false;
}

bool TerminalParser::ParseChar(QChar x)
{
    switch (x.toLatin1())
    {
        case 'v':
            Configuration::Verbosity++;
            return false;
        case 'h':
            this->DisplayHelp();
            return true;
        case 'f':
            Configuration::Forcing = true;
            return false;
        case '4':
            Configuration::only_qt4 = true;
            return false;
        case '5':
            Configuration::only_qt5 = true;
            return false;
    }
    return false;
}

void TerminalParser::DisplayHelp()
{
    if (this->Silent)
    {
        return;
    }
    cout << "q2c" << endl << endl;
    cout << "By default q2c checks the current folder for any qmake (.pro) file," << endl;
    cout << "it fails if it find 0 or more files" << endl << endl;
    cout << "Parameters:" << endl;
    cout << "  -v:              Increases verbosity" << endl;
    cout << "  -o|--out <file>: Defines a name of output file" << endl;
    cout << "  -i|--in <file>:  Defines a name of input file" << endl;
    cout << "  -h|--help:       Display this help" << endl;
    cout << "  -4|--qt4:        Set qt4 as only supported version" << endl;
    cout << "  -5|--qt5:        Set qt5 as only supported version" << endl << endl;
    cout << "q2c is an open source, contribute at https://github.com/benapetr/q2c" << endl;
}
