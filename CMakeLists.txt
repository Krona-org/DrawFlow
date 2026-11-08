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

# Опциональный фолбэк для CMAKE_PREFIX_PATH, если он не передан через CMakePresets или окружение
if(NOT DEFINED CMAKE_PREFIX_PATH AND EXISTS "C:/Qt/6.10.2/mingw_64")
    set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
    Xml
    Test
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
    src/frames/exportDialog/exportbinarydialog.h
    src/frames/exportDialog/exportbinarydialog.cpp

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

enable_testing()

set(TEST_SOURCES
    tests/test_k1804.cpp

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
    src/canvas/orthogonalrouter.h
    src/canvas/orthogonalrouter.cpp
    src/canvas/undocommands.h
    src/canvas/undocommands.cpp

    src/frames/bParamsDialog/data/blockparamsdialog.h
    src/frames/bParamsDialog/data/blockparamsdialog.cpp
    src/frames/bParamsDialog/data/helpdialog.h
    src/frames/bParamsDialog/data/helpdialog.cpp
    src/widgets/smoothcombobox.h
    src/widgets/smoothscrollarea.h
    src/widgets/smoothscrollarea.cpp

    src/core/projectstorage.h
    src/core/projectstorage.cpp
)

add_executable(test_drawflow ${TEST_SOURCES})
target_link_libraries(test_drawflow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Xml
    Qt6::Test
)
add_test(NAME test_drawflow COMMAND test_drawflow)

set(CANVAS_TEST_SOURCES ${TEST_SOURCES})
list(REMOVE_ITEM CANVAS_TEST_SOURCES tests/test_k1804.cpp)
add_executable(canvas_regression_tests tests/canvas_regression_tests.cpp ${CANVAS_TEST_SOURCES})
target_link_libraries(canvas_regression_tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Xml Qt6::Test)
add_test(NAME canvas_regression_tests COMMAND canvas_regression_tests)
set_tests_properties(canvas_regression_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(projectstorage_test tests/projectstorage_test.cpp ${CANVAS_TEST_SOURCES})
target_link_libraries(projectstorage_test PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Xml)
add_test(NAME projectstorage COMMAND projectstorage_test)
set_tests_properties(projectstorage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(drawflow_regression_tests tests/regression.cpp ${CANVAS_TEST_SOURCES}
    src/frames/wElementsFrame/data/windowelementsframe.h
    src/frames/wElementsFrame/data/windowelementsframe.cpp
    src/frames/wElementsFrame/ui/windowelementsframe.ui)
target_link_libraries(drawflow_regression_tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Xml)
add_test(NAME drawflow_regression_tests COMMAND drawflow_regression_tests)
set_tests_properties(drawflow_regression_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
