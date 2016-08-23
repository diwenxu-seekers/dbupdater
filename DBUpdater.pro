QT += core gui sql network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += qt stl c++11 c++14 warn_on
TARGET = DBUpdater
TEMPLATE = app

# use this to compile dbupdater with version 1 library
#DEFINES += DBUPDATER_VERSION_1

# (MP) Build with Multiple Processes
# (O2) Optimze for speed and (favor) Intel64 platform
# (GL) Enables whole program optimization.
# (Gw) Optimize Global Data

CONFIG (release, debug|release) {
  DEFINES += QT_NO_DEBUG_OUTPUT
  QMAKE_CFLAGS += /MP4 /O2 /favor:INTEL64 /GL /Gw
  QMAKE_CXXFLAGS += /MP4 /O2 /favor:INTEL64 /GL /Gw
}

# Memory leak detection for MSVC compilers, DEBUG mode only
win32-msvc* {
    CONFIG(debug, debug|release) {
  INCLUDEPATH += "C:\Program Files (x86)\Visual Leak Detector\include"
  contains(QMAKE_TARGET.arch, x86_64) {
      LIBS += -L"C:\Program Files (x86)\Visual Leak Detector\lib\Win64" -lvld
  } else {
      LIBS += -L"C:\Program Files (x86)\Visual Leak Detector\lib\Win32" -lvld
  }
    }
}

QMAKE_CFLAGS += \
  -DSQLITE_TEMP_STORE=2 \
  -DSQLITE_DEFAULT_PAGE_SIZE=16384 \
  -DSQLITE_DEFAULT_CACHE_SIZE=-32768 \
  -DSQLITE_THREADSAFE=2 \
  -DSQLITE_OMIT_DEPRECATED \
  -DSQLITE_OMIT_EXPLAIN \

# Warning level
# Level 3 recommended for production
# Level 4 recommended to ensure fewest possible hard-to-find code defects
QMAKE_CXXFLAGS += /W3

INCLUDEPATH += C:\boost_1_59_0
win32:LIBS += \
  -LC:/boost_1_59_0/lib64-msvc-12.0/ \
#  -LC:\boost_1_58_0\stage\lib -llibboost_timer-vc120-mt-gd-1_58

HEADERS += \
  indicators/atr.h \
  indicators/distfscross.h \
  ipcserver/commserver.h \
  ipcserver/session.h \
  logwidget/datetimedelegate.h \
  logwidget/logleveldelegate.h \
  logwidget/logmsgmodel.h \
  logwidget/logmsgwidget.h \
  logwidget/tableview_sink.h \
  searchbar/abstractstream.h \
  searchbar/barbuffer.h \
  searchbar/bardatalist.h \
  searchbar/bardataquerystream.h \
  searchbar/dayrangeobserver.h \
  searchbar/globals.h \
  searchbar/resistancehighobserver.h \
  searchbar/resistancequerystream.h \
  searchbar/resistancetagline.h \
  searchbar/resistancevelocityobserver.h \
  searchbar/sqlitehandler.h \
  searchbar/streamprocessor.h \
  searchbar/supportlowobserver.h \
  searchbar/supportquerystream.h \
  searchbar/supporttagline.h \
  searchbar/supportvelocityobserver.h \
  searchbar/xmlconfighandler.h \
  spdlog/details/async_log_helper.h \
  spdlog/details/async_logger_impl.h \
  spdlog/details/file_helper.h \
  spdlog/details/format.h \
  spdlog/details/line_logger.h \
  spdlog/details/log_msg.h \
  spdlog/details/logger_impl.h \
  spdlog/details/mpmc_bounded_q.h \
  spdlog/details/null_mutex.h \
  spdlog/details/os.h \
  spdlog/details/pattern_formatter_impl.h \
  spdlog/details/registry.h \
  spdlog/details/spdlog_impl.h \
  spdlog/sinks/base_sink.h \
  spdlog/sinks/file_sinks.h \
  spdlog/sinks/null_sink.h \
  spdlog/sinks/ostream_sink.h \
  spdlog/sinks/sink.h \
  spdlog/sinks/stdout_sinks.h \
  spdlog/sinks/syslog_sink.h \
  spdlog/async_logger.h \
  spdlog/common.h \
  spdlog/formatter.h \
  spdlog/logger.h \
  spdlog/spdlog.h \
  spdlog/tweakme.h \
  sqlite3/sqlite3.h \
  sqlite3/sqlite3ext.h \
  aboutdlg.h \
  bardatacalculator.h \
  bardatadefinition.h \
  bardatatuple.h \
  common.h \
  datetimehelper.h \
  dataupdater.h \
  dataupdater_v2.h \
  datawatcher.h \
  datawatcher_v2.h \
  fileloader.h \
  fileloader_v2.h \
  infobox.h \
  logdefs.h \
  mainwindow.h \

SOURCES += \
  indicators/atr.cpp \
  indicators/distfscross.cpp \
  ipcserver/commserver.cpp \
  ipcserver/session.cpp \
  logwidget/datetimedelegate.cpp \
  logwidget/logleveldelegate.cpp \
  logwidget/logmsgmodel.cpp \
  logwidget/logmsgwidget.cpp \
  searchbar/barbuffer.cpp \
  searchbar/globals.cpp \
  searchbar/sqlitehandler.cpp \
  searchbar/xmlconfighandler.cpp \
  spdlog/details/format.cc \
  sqlite3/sqlite3.c \
  aboutdlg.cpp \
  bardatacalculator.cpp \
  bardatatuple.cpp \
  common.cpp \
  dataupdater.cpp \
  dataupdater_v2.cpp \
  datawatcher.cpp \
  datawatcher_v2.cpp \
  fileloader.cpp \
  fileloader_v2.cpp \
  infobox.cpp \
  main.cpp \
  mainwindow.cpp \

FORMS += \
  aboutdlg.ui \
  infobox.ui \
  mainwindow.ui \

RESOURCES += \
  DBUpdater.qrc \

win32 {
  # Get build date and time
  DEFINES += BUILD_DATETIME=\\\"$$system('datetime.bat')\\\"

  # Get git tag
  GIT_COMMAND = \"C:/Program Files (x86)/Git/bin/git.exe\" --work-tree \"$$PWD\" describe --always --tags
  DEFINES += GIT_VERSION=\\\"$$system($$GIT_COMMAND)\\\"

  # Application icon
  RC_FILE = resources.rc
}
