cmake_minimum_required(VERSION 3.16)

project(DrawFlow LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frames
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frames/ePropertyFrame/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frames/eLibraryFrame/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frames/wElementsFrame/ui
)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements/shapeElement
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements/connectorElement
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements/masterBlockElement
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements/maskElement
)

set(SOURCES
    src/main.cpp

    src/elements/common/elementtype.h
    src/elements/common/portposition.h
    src/elements/masterBlockElement/k1804microcode.h
    src/elements/masterBlockElement/k1804microcode.cpp
    src/elements/shapeElement/shapeelement.h
    src/elements/shapeElement/shapeelement.cpp
    src/elements/masterBlockElement/masterblockelement.h
    src/elements/masterBlockElement/masterblockelement.cpp
    src/elements/connectorElement/connectorelement.h
    src/elements/connectorElement/connectorelement.cpp
    src/elements/maskElement/maskelement.h
    src/elements/maskElement/maskelement.cpp

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

    src/widgets/smoothcombobox.h
    src/widgets/smoothscrollarea.h
    src/widgets/smoothscrollarea.cpp
    src/themes/thememanager.h
    src/frames/bParamsDialog/data/blockparamsdialog.h
    src/frames/bParamsDialog/data/blockparamsdialog.cpp
    src/frames/bParamsDialog/data/helpdialog.h
    src/frames/bParamsDialog/data/helpdialog.cpp
    src/frames/eLibraryFrame/data/elementslibraryframe.h
    src/frames/eLibraryFrame/data/elementslibraryframe.cpp
    src/frames/eLibraryFrame/ui/elementslibraryframe.ui
    src/frames/wElementsFrame/data/windowelementsframe.h
    src/frames/wElementsFrame/data/windowelementsframe.cpp
    src/frames/wElementsFrame/ui/windowelementsframe.ui
    src/frames/ePropertyFrame/data/elementpropertyframe.h
    src/frames/ePropertyFrame/data/elementpropertyframe.cpp
    src/frames/ePropertyFrame/ui/elementpropertyframe.ui

    src/core/projectstorage.h
    src/core/projectstorage.cpp
    src/core/mainwindow.h
    src/core/mainwindow.cpp
    src/core/mainwindow.ui

    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES resources/app.rc)
    add_executable(drawflow WIN32 ${SOURCES})
else()
    add_executable(drawflow ${SOURCES})
endif()

target_link_libraries(drawflow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
)

