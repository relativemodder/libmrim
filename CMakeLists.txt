cmake_minimum_required(VERSION 3.16)

project(mrim LANGUAGES CXX VERSION 1.0)

add_compile_definitions(MRIM_LIB_VERSION="${PROJECT_VERSION}")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Core5Compat)

add_library(mrim SHARED
  libmrim_global.h

  redirector.h redirector.cpp
  mrimclient.h mrimclient.cpp
  mrimmessage.h mrimmessage.cpp
)

target_link_libraries(mrim
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Core5Compat
)

target_compile_definitions(mrim PRIVATE LIBMRIM_LIBRARY)
