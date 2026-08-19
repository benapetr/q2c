COMMON_SOURCES = $$PWD/shared/logger.cpp $$PWD/shared/settings.cpp
COMMON_HEADERS = $$PWD/shared/logger.h $$PWD/shared/settings.h

SOURCES += $$COMMON_SOURCES
HEADERS += $$COMMON_HEADERS
DEFINES += SHARED_PRI_USED
INCLUDEPATH += $$PWD/shared "third party/include"
LIBS += -L"third party/lib" -lsharedsupport

