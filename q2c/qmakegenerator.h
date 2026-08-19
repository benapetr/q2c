//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef QMAKEGENERATOR_H
#define QMAKEGENERATOR_H

#include <QString>
#include <QStringList>
#include "buildmodel.h"

class QMakeGenerator
{
    public:
        QString Generate(const BuildProject &project);

    private:
        QString GenerateTarget(const BuildProject &project, const BuildTarget &target);
        QString GenerateAssignments(const BuildTarget &target);
        QString GenerateConditionalScopes(const BuildTarget &target);
        QString GenerateAdditionalTargetNotes(const BuildProject &project, const BuildTarget &primary);
        QString Assignment(QString variable, const QList<QString> &items) const;
        QString ScopedAssignment(QString variable, const QList<QString> &items) const;
        QString ConfigForTarget(const BuildTarget &target) const;
        QStringList LibrariesForQmake(const QList<QString> &libraries) const;
        QStringList CompileOptionsForQmake(const QList<QString> &options) const;
        QStringList LinkOptionsForQmake(const QList<QString> &options) const;
        QString MapCondition(QString condition, bool *supported) const;
        QString Quote(QString value) const;
        bool HasUnsupportedGeneratorExpression(const QList<QString> &items) const;
};

#endif // QMAKEGENERATOR_H
