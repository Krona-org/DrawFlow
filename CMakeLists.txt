cmake_minimum_required(VERSION 3.16)

project(DrawFlow LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(EXISTS "C:/Qt/6.10.2/mingw_64")
    list(PREPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.2/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
    Xml
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(SOURCES
    src/main.cpp

    src/elements/elementtype.h
    src/elements/shapeelement.h
    src/elements/shapeelement.cpp
    src/elements/rectangleelement.h
    src/elements/rectangleelement.cpp
    src/elements/lineelement.h
    src/elements/lineelement.cpp

    src/canvas/canvasscene.h
    src/canvas/canvasscene.cpp
    src/canvas/canvasview.h
    src/canvas/canvasview.cpp
    src/canvas/canvastabwidget.h
    src/canvas/canvastabwidget.cpp
    src/canvas/orthogonalrouter.h
    src/canvas/orthogonalrouter.cpp
    src/canvas/undocommands.h
    src/canvas/undocommands.cpp

    src/frames/elementslibraryframe.h
    src/frames/elementslibraryframe.cpp
    src/frames/elementslibraryframe.ui
    src/frames/windowelementsframe.h
    src/frames/windowelementsframe.cpp
    src/frames/windowelementsframe.ui
    src/frames/elementpropertyframe.h
    src/frames/elementpropertyframe.cpp
    src/frames/elementpropertyframe.ui

    src/core/projectstorage.h
    src/core/projectstorage.cpp
    src/core/mainwindow.h
    src/core/mainwindow.cpp
    src/core/mainwindow.ui

    resources/resources.qrc
)

add_executable(drawflow ${SOURCES})

target_link_libraries(drawflow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
)

