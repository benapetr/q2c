//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include "buildmodel.h"
#include "cmakeparser.h"
#include "cmakegenerator.h"
#include "qmakegenerator.h"
#include "qmakeparser.h"

class TestRunner
{
    public:
        TestRunner()
        {
            this->Passed = 0;
            this->Failed = 0;
            this->Skipped = 0;
        }

        void Expect(bool condition, QString message)
        {
            if (condition)
            {
                this->Passed++;
                QTextStream(stdout) << "PASS: " << message << "\n";
            }
            else
            {
                this->Failed++;
                QTextStream(stderr) << "FAIL: " << message << "\n";
            }
        }

        void Skip(QString message)
        {
            this->Skipped++;
            QTextStream(stdout) << "SKIP: " << message << "\n";
        }

        int Finish()
        {
            QTextStream(stdout) << "\n" << this->Passed << " passed, " << this->Failed
                                << " failed, " << this->Skipped << " skipped\n";
            return this->Failed == 0 ? 0 : 1;
        }

    private:
        int Passed;
        int Failed;
        int Skipped;
};

static QString ReadFile(QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return "";
    QString text = QString(file.readAll());
    file.close();
    return text;
}

static QString Fixture(QString relative_path)
{
    return QDir(QString(TEST_FIXTURE_DIR)).absoluteFilePath(relative_path);
}

static bool Contains(const QList<QString> &list, QString value)
{
    return list.contains(value);
}

static bool ContainsWarning(const BuildProject &project, QString needle)
{
    foreach (QString warning, project.Warnings)
    {
        if (warning.contains(needle))
            return true;
    }
    return false;
}

static QString NormalizeGenerated(QString text)
{
    text.replace(QRegularExpression("https://github\\.com/benapetr/q2c at [^\\n]+"), "https://github.com/benapetr/q2c at <timestamp>");
    return text.replace("\r\n", "\n");
}

static void ExpectSnapshot(TestRunner *runner, QString actual, QString snapshot_fixture, QString message)
{
    QString expected = ReadFile(Fixture(snapshot_fixture)).replace("\r\n", "\n");
    runner->Expect(!expected.isEmpty(), message + " snapshot is readable");
    runner->Expect(NormalizeGenerated(actual) == expected, message);
}

static bool HasScopeWithSource(const BuildTarget *target, QString condition, QString source)
{
    if (target == nullptr)
        return false;
    foreach (const BuildConditionalScope &scope, target->ConditionalScopes)
    {
        if (scope.Condition == condition && scope.Sources.contains(source))
            return true;
    }
    return false;
}

static bool HasScopeWithDefine(const BuildTarget *target, QString condition, QString define)
{
    if (target == nullptr)
        return false;
    foreach (const BuildConditionalScope &scope, target->ConditionalScopes)
    {
        if (scope.Condition == condition && scope.Defines.contains(define))
            return true;
    }
    return false;
}

static bool HasScopeWithLibrary(const BuildTarget *target, QString condition, QString library)
{
    if (target == nullptr)
        return false;
    foreach (const BuildConditionalScope &scope, target->ConditionalScopes)
    {
        if (scope.Condition == condition && scope.Libraries.contains(library))
            return true;
    }
    return false;
}

static void TestPhase3QMakeFixture(TestRunner *runner)
{
    QString fixture = Fixture("qmake/phase3/phase3.pro");
    QString text = ReadFile(fixture);
    runner->Expect(!text.isEmpty(), "phase3 qmake fixture is readable");

    BuildProject project;
    QMakeParser parser;
    runner->Expect(parser.Parse(text, &project, fixture, "VERSION 3.1.0"), "phase3 qmake fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(target != nullptr, "phase3 fixture has primary target");
    if (target == nullptr)
        return;

    QString phase_dir = QFileInfo(fixture).absoluteDir().absolutePath();
    runner->Expect(project.Name == "phase_three", "quoted TARGET is normalized");
    runner->Expect(target->Type == BuildTarget_Application, "app template maps to application target");
    runner->Expect(Contains(target->Sources, phase_dir + "/included.cpp"), "include() parses .pri source with $$PWD");
    runner->Expect(Contains(target->Sources, "src/main file.cpp"), "quoted source path with spaces is preserved");
    runner->Expect(Contains(target->Sources, "extra.cpp"), "list variable expansion preserves all values");
    runner->Expect(Contains(target->Sources, "generated.cpp"), "multiline SOURCES continuation parses");
    runner->Expect(Contains(target->Headers, "included.h"), "include() parses .pri headers");
    runner->Expect(Contains(target->Headers, "mainwindow.h"), "HEADERS parses");
    runner->Expect(Contains(target->QtModules, "core"), "QT keeps core module");
    runner->Expect(Contains(target->QtModules, "widgets"), "QT keeps widgets module");
    runner->Expect(!Contains(target->QtModules, "gui"), "QT -= removes gui module");
    runner->Expect(Contains(target->Config, "c++17"), "CONFIG *= stores c++17");
    runner->Expect(target->Config.count("c++17") == 1, "CONFIG *= avoids duplicates");
    runner->Expect(Contains(target->Defines, "FROM_PRI"), "DEFINES from .pri parses");
    runner->Expect(Contains(target->IncludePaths, "include dir"), "quoted INCLUDEPATH with spaces parses");
    runner->Expect(Contains(target->IncludePaths, phase_dir + "/include"), "$$PWD expands in INCLUDEPATH");
    runner->Expect(Contains(target->IncludePaths, "deps"), "DEPENDPATH is mapped to include paths");
    runner->Expect(Contains(target->Libraries, "-Llib dir"), "quoted -L path parses");
    runner->Expect(Contains(target->Libraries, "-framework"), "macOS framework flag parses");
    runner->Expect(Contains(target->Libraries, "Cocoa"), "macOS framework name parses");
    runner->Expect(Contains(target->TranslationFiles, "i18n/app_en.ts"), "TRANSLATIONS parses");
    runner->Expect(Contains(target->CompileOptions, "-Wall"), "QMAKE_CXXFLAGS parses");
    runner->Expect(Contains(target->CompileOptions, "-Wextra"), "multiple QMAKE_CXXFLAGS parse");
    runner->Expect(Contains(target->LinkOptions, "-pthread"), "QMAKE_LFLAGS parses");
    runner->Expect(Contains(target->InstallRules, "target"), "INSTALLS parses");
    runner->Expect(HasScopeWithSource(target, "WIN32", "win path/win.cpp"), "inline win32 scope parses source");
    runner->Expect(HasScopeWithDefine(target, "NOT WIN32", "NOT_WIN"), "else scope maps to inverse condition");
    runner->Expect(HasScopeWithSource(target, "APPLE", "mac.mm"), "macx block scope parses source");
    runner->Expect(HasScopeWithDefine(target, "contains(QT, widgets)", "HAS_WIDGETS"), "function condition scope is preserved");
    runner->Expect(ContainsWarning(project, "Condition function kept as a raw expression"), "function condition emits warning");

    CMakeGenerator generator(CMakeQtVersion_All);
    QString cmake = generator.Generate(project, QList<CMakeOption>());
    runner->Expect(cmake.contains("project(phase_three)"), "generated CMake names project");
    runner->Expect(cmake.contains("\"src/main file.cpp\""), "generated CMake quotes source path with spaces");
    runner->Expect(cmake.contains("target_include_directories(phase_three PRIVATE"), "generated CMake uses target include directories");
    runner->Expect(cmake.contains("\"include dir\""), "generated CMake quotes include path with spaces");
    runner->Expect(cmake.contains("target_link_directories(phase_three PRIVATE \"lib dir\")"), "generated CMake quotes link directory with spaces");
    runner->Expect(cmake.contains("target_link_libraries(phase_three PRIVATE \"-framework Cocoa\")"), "generated CMake keeps framework pair");
    runner->Expect(cmake.contains("set(phase_three_TRANSLATIONS \"i18n/app_en.ts\")"), "generated CMake emits translations");
    runner->Expect(cmake.contains("target_compile_options(phase_three PRIVATE -Wall)"), "generated CMake emits compile options");
    runner->Expect(cmake.contains("target_link_options(phase_three PRIVATE -pthread)"), "generated CMake emits link options");
    runner->Expect(cmake.contains("# q2c warning:"), "generated CMake includes parser warnings");
}

static void TestComplexQMakeFixture(TestRunner *runner)
{
    QString fixture = Fixture("qmake/complex/complex.pro");
    BuildProject project;
    QMakeParser parser;
    runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture, "VERSION 3.16"), "complex qmake fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(target != nullptr, "complex qmake fixture has target");
    if (target == nullptr)
        return;

    QString fixture_dir = QFileInfo(fixture).absoluteDir().absolutePath();
    runner->Expect(project.Name == "complex_app", "complex qmake target name parses");
    runner->Expect(target->Type == BuildTarget_Test, "complex qmake testcase config maps target type");
    runner->Expect(Contains(target->QtModules, "core"), "complex qmake keeps core module");
    runner->Expect(Contains(target->QtModules, "widgets"), "complex qmake keeps widgets module");
    runner->Expect(Contains(target->QtModules, "network"), "complex qmake keeps network module");
    runner->Expect(!Contains(target->QtModules, "gui"), "complex qmake removes gui module");
    runner->Expect(Contains(target->Sources, fixture_dir + "/shared/logger.cpp"), "complex qmake include expands shared source path");
    runner->Expect(Contains(target->Sources, "src/file with space.cpp"), "complex qmake keeps source path with spaces");
    runner->Expect(Contains(target->Headers, fixture_dir + "/shared/settings.h"), "complex qmake include expands shared header path");
    runner->Expect(Contains(target->UiFiles, "ui/preferences.ui"), "complex qmake parses multiple forms");
    runner->Expect(Contains(target->ResourceFiles, "resources/icons.qrc"), "complex qmake parses multiple resources");
    runner->Expect(Contains(target->TranslationFiles, "i18n/complex_de.ts"), "complex qmake parses translations");
    runner->Expect(Contains(target->IncludePaths, fixture_dir + "/generated"), "complex qmake expands generated include path");
    runner->Expect(Contains(target->IncludePaths, "third party/include"), "complex qmake preserves include path with spaces");
    runner->Expect(Contains(target->Libraries, "-Lthird party/lib"), "complex qmake preserves quoted library path");
    runner->Expect(Contains(target->Libraries, "-framework"), "complex qmake parses framework flag");
    runner->Expect(Contains(target->CompileOptions, "-Wpedantic"), "complex qmake parses compile option list");
    runner->Expect(Contains(target->InstallRules, "translations"), "complex qmake parses install entries");
    runner->Expect(HasScopeWithSource(target, "WIN32", "platform/win.cpp"), "complex qmake parses win32 source scope");
    runner->Expect(HasScopeWithSource(target, "UNIX", "platform/unix.cpp"), "complex qmake parses unix source scope");
    runner->Expect(HasScopeWithLibrary(target, "APPLE", "Cocoa"), "complex qmake parses mac framework scope");
    runner->Expect(HasScopeWithDefine(target, "contains(QT, network)", "HAS_NETWORK_MODULE"), "complex qmake keeps contains condition");

    CMakeGenerator generator(CMakeQtVersion_Qt6);
    QString cmake = generator.Generate(project, QList<CMakeOption>());
    runner->Expect(cmake.contains("find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)"), "complex qmake generates Qt6 module discovery");
    runner->Expect(cmake.contains("\"src/file with space.cpp\""), "complex qmake generated CMake quotes source with spaces");
    runner->Expect(cmake.contains("target_link_directories(complex_app PRIVATE \"third party/lib\")"), "complex qmake generated CMake keeps shared link directory");
    runner->Expect(cmake.contains("target_link_libraries(complex_app PRIVATE \"-framework Security\")"), "complex qmake generated CMake keeps framework");
    runner->Expect(cmake.contains("if(WIN32)"), "complex qmake generated CMake emits win32 scope");
    runner->Expect(cmake.contains("if(UNIX)"), "complex qmake generated CMake emits unix scope");
    runner->Expect(cmake.contains("set(complex_app_TRANSLATIONS"), "complex qmake generated CMake emits translations");
}

static void TestPhase5CMakeGeneration(TestRunner *runner)
{
    QString fixture = Fixture("qmake/phase3/phase3.pro");
    BuildProject project;
    QMakeParser parser;
    runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture, "VERSION 3.1.0"), "phase5 fixture parses");

    CMakeGenerator qt6_generator(CMakeQtVersion_Qt6);
    QString qt6 = qt6_generator.Generate(project, QList<CMakeOption>());
    runner->Expect(qt6.contains("find_package(Qt6 COMPONENTS Core Widgets REQUIRED)"), "Qt6 generator finds selected modules");
    runner->Expect(qt6.contains("set(CMAKE_AUTOMOC ON)"), "Qt6 generator enables automoc");
    runner->Expect(qt6.contains("set(CMAKE_AUTOUIC ON)"), "Qt6 generator enables autouic");
    runner->Expect(qt6.contains("set(CMAKE_AUTORCC ON)"), "Qt6 generator enables autorcc");
    runner->Expect(qt6.contains("add_executable(phase_three ${phase_three_SOURCES} ${phase_three_HEADERS} ${phase_three_UI_FILES} ${phase_three_RESOURCE_FILES})"), "Qt6 generator attaches all source classes to target");
    runner->Expect(qt6.contains("target_compile_definitions(phase_three PRIVATE"), "Qt6 generator uses target compile definitions");
    runner->Expect(qt6.contains("target_include_directories(phase_three PRIVATE"), "Qt6 generator uses target include directories");
    runner->Expect(qt6.contains("target_link_libraries(phase_three PRIVATE Qt6::Core Qt6::Widgets)"), "Qt6 generator links imported Qt targets");
    runner->Expect(!qt6.contains("qt6_wrap_cpp"), "Qt6 generator relies on automoc instead of wrap_cpp");

    CMakeGenerator qt5_generator(CMakeQtVersion_Qt5);
    QString qt5 = qt5_generator.Generate(project, QList<CMakeOption>());
    runner->Expect(qt5.contains("find_package(Qt5 COMPONENTS Core Widgets REQUIRED)"), "Qt5 generator finds selected modules");
    runner->Expect(qt5.contains("target_link_libraries(phase_three PRIVATE Qt5::Core Qt5::Widgets)"), "Qt5 generator links imported Qt targets");
    runner->Expect(qt5.contains("set(CMAKE_AUTOMOC ON)"), "Qt5 generator enables automoc");

    CMakeGenerator qt4_generator(CMakeQtVersion_Qt4);
    QString qt4 = qt4_generator.Generate(project, QList<CMakeOption>());
    runner->Expect(qt4.contains("find_package(Qt4 REQUIRED)"), "Qt4 generator finds Qt4");
    runner->Expect(qt4.contains("include(${QT_USE_FILE})"), "Qt4 generator includes Qt use file");
    runner->Expect(qt4.contains("qt4_wrap_cpp(phase_three_HEADERS_MOC ${phase_three_HEADERS})"), "Qt4 generator wraps moc headers");
    runner->Expect(qt4.contains("qt4_wrap_ui(phase_three_UI_HEADERS ${phase_three_UI_FILES})"), "Qt4 generator wraps ui files");
    runner->Expect(qt4.contains("qt4_add_resources(phase_three_RESOURCES ${phase_three_RESOURCE_FILES})"), "Qt4 generator wraps resources");
    runner->Expect(!qt4.contains("CMAKE_AUTOMOC ON"), "Qt4 generator does not emit automoc");
}

static void TestVersionAndConsoleFixtures(TestRunner *runner)
{
    struct VersionCase
    {
        QString FixturePath;
        CMakeQtVersion Version;
        QString ExpectedFindPackage;
        QString ExpectedTargetLink;
        QString Message;
    };

    QList<VersionCase> cases;
    cases << VersionCase{"qmake/qt4/qt4_widgets.pro", CMakeQtVersion_Qt4, "find_package(Qt4 REQUIRED)", "include(${QT_USE_FILE})", "Qt4 fixture"};
    cases << VersionCase{"qmake/qt5/qt5_widgets.pro", CMakeQtVersion_Qt5, "find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)", "target_link_libraries(qt5_widgets PRIVATE Qt5::Core Qt5::Widgets Qt5::Network)", "Qt5 fixture"};
    cases << VersionCase{"qmake/qt6/qt6_widgets.pro", CMakeQtVersion_Qt6, "find_package(Qt6 COMPONENTS Core Widgets REQUIRED)", "target_link_libraries(qt6_widgets PRIVATE Qt6::Core Qt6::Widgets)", "Qt6 fixture"};

    foreach (const VersionCase &item, cases)
    {
        BuildProject project;
        QMakeParser parser;
        QString fixture = Fixture(item.FixturePath);
        runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture, "VERSION 3.16"), item.Message + " parses");
        CMakeGenerator generator(item.Version);
        QString cmake = generator.Generate(project, QList<CMakeOption>());
        runner->Expect(cmake.contains(item.ExpectedFindPackage), item.Message + " emits expected Qt discovery");
        runner->Expect(cmake.contains(item.ExpectedTargetLink), item.Message + " emits expected Qt linkage");
    }

    BuildProject console_project;
    QMakeParser console_parser;
    QString console_fixture = Fixture("qmake/console/console.pro");
    runner->Expect(console_parser.Parse(ReadFile(console_fixture), &console_project, console_fixture, "VERSION 3.16"), "console fixture parses");
    const BuildTarget *console_target = console_project.PrimaryTarget();
    runner->Expect(console_target != nullptr && console_target->Type == BuildTarget_Application, "console fixture maps to app target");
    runner->Expect(console_target != nullptr && Contains(console_target->Config, "console"), "console fixture preserves console config");
    CMakeGenerator console_generator(CMakeQtVersion_Qt6);
    QString console_cmake = console_generator.Generate(console_project, QList<CMakeOption>());
    runner->Expect(console_cmake.contains("add_executable(console_tool ${console_tool_SOURCES})"), "console fixture generates executable");
}

static void TestLibraryFixture(TestRunner *runner)
{
    QString fixture = Fixture("qmake/library/library.pro");
    BuildProject project;
    QMakeParser parser;
    runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture, "VERSION 3.16.0"), "library fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(target != nullptr && target->Type == BuildTarget_Library, "TEMPLATE = lib maps to library");

    CMakeGenerator generator(CMakeQtVersion_Qt6);
    QString cmake = generator.Generate(project, QList<CMakeOption>());
    runner->Expect(cmake.contains("add_library(libdemo ${libdemo_SOURCES} ${libdemo_HEADERS})"), "library fixture generates add_library");
    ExpectSnapshot(runner, cmake, "snapshots/library_qt6.cmake.expected", "library Qt6 generated CMake snapshot matches");
}

static void TestSubdirsFixture(TestRunner *runner)
{
    QString fixture = Fixture("qmake/subdirs/subdirs.pro");
    BuildProject project;
    QMakeParser parser;
    runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture, "VERSION 3.1.0"), "subdirs fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(target != nullptr && target->Type == BuildTarget_Subdirs, "TEMPLATE = subdirs maps to subdirs target");
    runner->Expect(target != nullptr && target->Subdirectories.contains("corelib"), "subdirs fixture contains corelib");
    runner->Expect(target != nullptr && target->Subdirectories.contains("guiapp"), "subdirs fixture contains guiapp");

    CMakeGenerator generator(CMakeQtVersion_All);
    QString cmake = generator.Generate(project, QList<CMakeOption>());
    runner->Expect(cmake.contains("project(MainProject)"), "subdirs fixture generates fallback project name");
    runner->Expect(cmake.contains("add_subdirectory(corelib)"), "subdirs fixture generates corelib");
    runner->Expect(cmake.contains("add_subdirectory(guiapp)"), "subdirs fixture generates guiapp");
}

static void TestCMakeFixtureParses(TestRunner *runner)
{
    QString fixture = Fixture("cmake/basic/CMakeLists.txt");
    QString text = ReadFile(fixture);
    runner->Expect(!text.isEmpty(), "cmake fixture is readable");
    runner->Expect(text.contains("add_executable"), "cmake fixture contains a real target");

    BuildProject project;
    CMakeParser parser;
    runner->Expect(parser.Parse(text, &project, fixture), "cmake fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(project.Name == "cmake_fixture", "CMake project() parses project name");
    runner->Expect(project.CMakeMinimumVersion == "VERSION 3.16", "cmake_minimum_required parses version");
    runner->Expect(target != nullptr, "cmake fixture has primary target");
    if (target == nullptr)
        return;

    runner->Expect(target->Name == "cmake_fixture", "add_executable parses target name");
    runner->Expect(target->Type == BuildTarget_Application, "add_executable maps to application target");
    runner->Expect(Contains(target->Sources, "main.cpp"), "add_executable parses source");
    runner->Expect(Contains(target->Sources, "mainwindow.cpp"), "add_executable parses second source");
    runner->Expect(Contains(target->Sources, "extra.cpp"), "target_sources parses source");
    runner->Expect(Contains(target->Headers, "mainwindow.h"), "add_executable classifies header");
    runner->Expect(Contains(target->UiFiles, "mainwindow.ui"), "add_executable classifies ui file");
    runner->Expect(Contains(target->ResourceFiles, "resources.qrc"), "add_executable classifies resource file");
    runner->Expect(Contains(target->Defines, "HAS_CMAKE_FIXTURE"), "target_compile_definitions parses");
    runner->Expect(Contains(target->IncludePaths, "include"), "target_include_directories parses");
    runner->Expect(Contains(target->CompileOptions, "-Wall"), "target_compile_options parses");
    runner->Expect(Contains(target->CompileOptions, "$<$<CONFIG:Debug>:-DDEBUG_BUILD>"), "target_compile_options keeps generator expression for warning");
    runner->Expect(Contains(target->Libraries, "-Llib dir"), "target_link_directories parses");
    runner->Expect(Contains(target->Libraries, "customlib"), "target_link_libraries parses non-Qt library");
    runner->Expect(Contains(target->LinkOptions, "-pthread"), "target_link_options parses");
    runner->Expect(Contains(target->TranslationFiles, "i18n/cmake_fixture.ts"), "qt_add_translations parses");
    runner->Expect(Contains(target->Subdirectories, "plugin"), "add_subdirectory is attached to primary target");
    runner->Expect(HasScopeWithSource(target, "WIN32", "win.cpp"), "if target_sources parses scoped source");
    runner->Expect(HasScopeWithDefine(target, "WIN32", "WIN_ONLY"), "if target_compile_definitions parses scoped define");
    runner->Expect(Contains(target->QtModules, "core"), "Qt imported Core target maps to qt module");
    runner->Expect(Contains(target->QtModules, "widgets"), "Qt imported Widgets target maps to qt module");
    runner->Expect(ContainsWarning(project, "set_target_properties"), "set_target_properties emits warning");

    QMakeGenerator generator;
    QString qmake = generator.Generate(project);
    runner->Expect(qmake.contains("TARGET = cmake_fixture"), "generated qmake contains target");
    runner->Expect(qmake.contains("QT += \\\n    core \\\n    widgets"), "generated qmake contains Qt modules");
    runner->Expect(qmake.contains("SOURCES += \\\n    main.cpp \\\n    mainwindow.cpp \\\n    extra.cpp"), "generated qmake contains sources");
    runner->Expect(qmake.contains("HEADERS += mainwindow.h"), "generated qmake contains headers");
    runner->Expect(qmake.contains("FORMS += mainwindow.ui"), "generated qmake contains forms");
    runner->Expect(qmake.contains("RESOURCES += resources.qrc"), "generated qmake contains resources");
    runner->Expect(qmake.contains("TRANSLATIONS += i18n/cmake_fixture.ts"), "generated qmake contains translations");
    runner->Expect(qmake.contains("LIBS += \\\n    \"-Llib dir\" \\\n    customlib"), "generated qmake contains link directories and libraries");
    runner->Expect(qmake.contains("QMAKE_CXXFLAGS += -Wall"), "generated qmake contains compile options");
    runner->Expect(qmake.contains("QMAKE_LFLAGS += -pthread"), "generated qmake contains link options");
    runner->Expect(qmake.contains("win32 {\n    SOURCES += win.cpp\n}"), "generated qmake maps WIN32 source scope");
    runner->Expect(qmake.contains("win32 {\n    DEFINES += WIN_ONLY\n}"), "generated qmake maps WIN32 define scope");
    runner->Expect(qmake.contains("CMake generator expressions require manual qmake review"), "generated qmake warns on generator expressions");
    runner->Expect(qmake.contains("Additional CMake target 'plugin_module'"), "generated qmake warns on additional target");
}

static void TestComplexCMakeFixture(TestRunner *runner)
{
    QString fixture = Fixture("cmake/complex/CMakeLists.txt");
    BuildProject project;
    CMakeParser parser;
    runner->Expect(parser.Parse(ReadFile(fixture), &project, fixture), "complex CMake fixture parses");

    const BuildTarget *target = project.PrimaryTarget();
    runner->Expect(target != nullptr, "complex CMake fixture has primary target");
    if (target == nullptr)
        return;

    runner->Expect(project.Name == "complex_cmake", "complex CMake project name parses");
    runner->Expect(project.CMakeMinimumVersion == "VERSION 3.20", "complex CMake minimum version parses");
    runner->Expect(target->Type == BuildTarget_Application, "complex CMake qt_add_executable maps app target");
    runner->Expect(Contains(target->Sources, "src/main.cpp"), "complex CMake expands source variable");
    runner->Expect(Contains(target->Sources, "src/file with space.cpp"), "complex CMake parses quoted source");
    runner->Expect(Contains(target->Headers, "include/appconfig.h"), "complex CMake expands header variable");
    runner->Expect(Contains(target->UiFiles, "ui/mainwindow.ui"), "complex CMake classifies ui file");
    runner->Expect(Contains(target->ResourceFiles, "resources/app.qrc"), "complex CMake classifies qrc file");
    runner->Expect(Contains(target->Defines, "VERSION=\\\"2.0\\\""), "complex CMake keeps version define");
    runner->Expect(Contains(target->IncludePaths, "third party/include"), "complex CMake parses include path with spaces");
    runner->Expect(Contains(target->Libraries, "-Lthird party/lib"), "complex CMake parses link directory");
    runner->Expect(Contains(target->Libraries, "ssl"), "complex CMake parses ssl library");
    runner->Expect(Contains(target->CompileOptions, "$<$<CONFIG:Debug>:-DDEBUG_ONLY>"), "complex CMake keeps generator expression option");
    runner->Expect(Contains(target->TranslationFiles, "i18n/complex_de.ts"), "complex CMake parses translations");
    runner->Expect(Contains(target->Subdirectories, "plugins"), "complex CMake parses subdirectory");
    runner->Expect(HasScopeWithSource(target, "WIN32", "platform/win.cpp"), "complex CMake parses WIN32 source scope");
    runner->Expect(HasScopeWithSource(target, "APPLE", "platform/mac.mm"), "complex CMake parses APPLE source scope");
    runner->Expect(HasScopeWithLibrary(target, "APPLE", "-framework Cocoa"), "complex CMake parses scoped framework");
    runner->Expect(HasScopeWithDefine(target, "UNIX", "PLATFORM_UNIX"), "complex CMake parses UNIX define scope");

    QMakeGenerator generator;
    QString qmake = generator.Generate(project);
    runner->Expect(qmake.contains("TARGET = complex_cmake"), "complex CMake generated qmake has target");
    runner->Expect(qmake.contains("QT += \\\n    core \\\n    widgets \\\n    network"), "complex CMake generated qmake has Qt modules");
    runner->Expect(qmake.contains("\"src/file with space.cpp\""), "complex CMake generated qmake quotes source with spaces");
    runner->Expect(qmake.contains("LIBS += \\\n    \"-Lthird party/lib\" \\\n    ssl \\\n    crypto"), "complex CMake generated qmake maps libraries");
    runner->Expect(qmake.contains("macx {\n    SOURCES += platform/mac.mm"), "complex CMake generated qmake maps APPLE scope");
    runner->Expect(qmake.contains("unix {\n    SOURCES += platform/unix.cpp"), "complex CMake generated qmake maps UNIX scope");
    runner->Expect(qmake.contains("CMake generator expressions require manual qmake review"), "complex CMake generated qmake warns about generator expression");
    runner->Expect(qmake.contains("Additional CMake target 'extra_plugin'"), "complex CMake generated qmake warns about module target");
    ExpectSnapshot(runner, qmake, "snapshots/complex_cmake.pro.expected", "complex CMake generated qmake snapshot matches");
}

static void TestRoundTrips(TestRunner *runner)
{
    QString qmake_fixture = Fixture("qmake/complex/complex.pro");
    BuildProject qmake_project;
    QMakeParser qmake_parser;
    runner->Expect(qmake_parser.Parse(ReadFile(qmake_fixture), &qmake_project, qmake_fixture, "VERSION 3.16"), "round-trip qmake source parses");
    CMakeGenerator cmake_generator(CMakeQtVersion_Qt6);
    QString generated_cmake = cmake_generator.Generate(qmake_project, QList<CMakeOption>());

    BuildProject reparsed_cmake;
    CMakeParser cmake_parser;
    runner->Expect(cmake_parser.Parse(generated_cmake, &reparsed_cmake, "generated/CMakeLists.txt"), "round-trip generated CMake parses");
    const BuildTarget *cmake_target = reparsed_cmake.PrimaryTarget();
    runner->Expect(cmake_target != nullptr && cmake_target->Name == "complex_app", "round-trip CMake keeps target name");
    runner->Expect(cmake_target != nullptr && Contains(cmake_target->QtModules, "network"), "round-trip CMake keeps Qt module");
    runner->Expect(cmake_target != nullptr && Contains(cmake_target->Sources, "src/window.cpp"), "round-trip CMake keeps source");
    runner->Expect(cmake_target != nullptr && Contains(cmake_target->TranslationFiles, "i18n/complex_de.ts"), "round-trip CMake keeps translation");

    QString cmake_fixture = Fixture("cmake/complex/CMakeLists.txt");
    BuildProject cmake_project;
    CMakeParser original_cmake_parser;
    runner->Expect(original_cmake_parser.Parse(ReadFile(cmake_fixture), &cmake_project, cmake_fixture), "round-trip CMake source parses");
    QMakeGenerator qmake_generator;
    QString generated_qmake = qmake_generator.Generate(cmake_project);

    BuildProject reparsed_qmake;
    QMakeParser reparsing_qmake_parser;
    runner->Expect(reparsing_qmake_parser.Parse(generated_qmake, &reparsed_qmake, "generated/project.pro", "VERSION 3.16"), "round-trip generated qmake parses");
    const BuildTarget *qmake_target = reparsed_qmake.PrimaryTarget();
    runner->Expect(qmake_target != nullptr && qmake_target->Name == "complex_cmake", "round-trip qmake keeps target name");
    runner->Expect(qmake_target != nullptr && Contains(qmake_target->QtModules, "network"), "round-trip qmake keeps Qt module");
    runner->Expect(qmake_target != nullptr && Contains(qmake_target->Sources, "src/file with space.cpp"), "round-trip qmake keeps quoted source");
    runner->Expect(qmake_target != nullptr && Contains(qmake_target->Libraries, "-Lthird party/lib"), "round-trip qmake keeps link directory");
}

static void TestNegativeAndUnsupportedInputs(TestRunner *runner)
{
    BuildProject missing_target;
    QMakeParser qmake_parser;
    QString missing_target_fixture = Fixture("qmake/negative/missing_target.pro");
    runner->Expect(!qmake_parser.Parse(ReadFile(missing_target_fixture), &missing_target, missing_target_fixture, "VERSION 3.16"), "negative qmake fixture without target fails");

    BuildProject unsupported_cmake;
    CMakeParser cmake_parser;
    QString unsupported_fixture = Fixture("cmake/negative/unsupported.cmake");
    runner->Expect(cmake_parser.Parse(ReadFile(unsupported_fixture), &unsupported_cmake, unsupported_fixture), "unsupported CMake fixture still parses");
    runner->Expect(ContainsWarning(unsupported_cmake, "Unsupported CMake command"), "unsupported CMake command emits warning");
    runner->Expect(ContainsWarning(unsupported_cmake, "set_target_properties"), "unsupported CMake property emits warning");

    QMakeGenerator qmake_generator;
    QString qmake = qmake_generator.Generate(unsupported_cmake);
    runner->Expect(qmake.contains("CMake generator expressions require manual qmake review"), "unsupported generator expression emits qmake warning");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    TestRunner runner;
    TestPhase3QMakeFixture(&runner);
    TestComplexQMakeFixture(&runner);
    TestPhase5CMakeGeneration(&runner);
    TestVersionAndConsoleFixtures(&runner);
    TestLibraryFixture(&runner);
    TestSubdirsFixture(&runner);
    TestCMakeFixtureParses(&runner);
    TestComplexCMakeFixture(&runner);
    TestRoundTrips(&runner);
    TestNegativeAndUnsupportedInputs(&runner);
    return runner.Finish();
}
