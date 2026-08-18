//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "buildmodel.h"

BuildSourceLocation::BuildSourceLocation()
{
    this->LineNumber = 0;
}

BuildSourceLocation::BuildSourceLocation(QString file_name, int line_number)
{
    this->FileName = file_name;
    this->LineNumber = line_number;
}

bool BuildSourceLocation::IsValid() const
{
    return !this->FileName.isEmpty() && this->LineNumber > 0;
}

BuildConditionalScope::BuildConditionalScope()
{
}

BuildTarget::BuildTarget()
{
    this->Type = BuildTarget_Unknown;
}

QString BuildTarget::TypeName() const
{
    switch (this->Type)
    {
        case BuildTarget_Application:
            return "application";
        case BuildTarget_Library:
            return "library";
        case BuildTarget_Plugin:
            return "plugin";
        case BuildTarget_Test:
            return "test";
        case BuildTarget_Subdirs:
            return "subdirs";
        case BuildTarget_Unknown:
        default:
            return "unknown";
    }
}

BuildProject::BuildProject()
{
    this->Clear();
}

void BuildProject::Clear()
{
    this->Name = "";
    this->CMakeMinimumVersion = "";
    this->GlobalConfig.clear();
    this->GlobalQtModules.clear();
    this->Warnings.clear();
    this->Targets.clear();
}

BuildTarget *BuildProject::EnsurePrimaryTarget()
{
    if (this->Targets.isEmpty())
        this->Targets.append(BuildTarget());
    return &this->Targets[0];
}

BuildTarget *BuildProject::PrimaryTarget()
{
    if (this->Targets.isEmpty())
        return nullptr;
    return &this->Targets[0];
}

const BuildTarget *BuildProject::PrimaryTarget() const
{
    if (this->Targets.isEmpty())
        return nullptr;
    return &this->Targets[0];
}

void BuildProject::AddWarning(QString warning)
{
    if (!this->Warnings.contains(warning))
        this->Warnings.append(warning);
}
