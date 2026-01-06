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

find_package(Python3 COMPONENTS Interpreter)

if (Python3_FOUND)
  add_custom_command(
    OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mrim/mrimprotocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mrimprotocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mrim/messagehandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/messagehandler.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen.py ${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating MRIM protocol code from protocol.xml"
  )

  add_custom_target(mrimcodegen
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/mrim/mrimprotocol.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/mrimprotocol.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/mrim/messagehandler.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/messagehandler.cpp
  )
else()
  message(WARNING "Python3 not found. Code generation will not be available. Please run codegen.py manually.")
endif()

add_library(mrim SHARED
  include/mrim/libmrim_global.h

  include/mrim/redirector.h src/redirector.cpp
  include/mrim/mrimclient.h src/mrimclient.cpp
  include/mrim/mrimmessage.h src/mrimmessage.cpp
  include/mrim/mrimconnection.h src/mrimconnection.cpp
  include/mrim/messagehandler.h src/messagehandler.cpp
  include/mrim/mrimprotocol.h src/mrimprotocol.cpp
)

if(Python3_FOUND)
    add_dependencies(mrim mrimcodegen)
endif()

target_link_libraries(mrim
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Core5Compat
)

target_include_directories(mrim PUBLIC include)

target_compile_definitions(mrim PRIVATE LIBMRIM_LIBRARY)
