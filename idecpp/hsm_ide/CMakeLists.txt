cmake_minimum_required(VERSION 3.5)

project(hsm_ide VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES ts/hsm_ide_en_001.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        widgets/HsmElementsList.hpp
        widgets/HsmElementsList.cpp
        widgets/HsmGraphicsView.hpp
        widgets/HsmGraphicsView.cpp

        # elements/HsmElementsFactory.cpp
        # elements/HsmElementsFactory.hpp
        elements/HsmEntryPointElement.cpp
        elements/HsmEntryPointElement.hpp
        elements/HsmExitPointElement.cpp
        elements/HsmExitPointElement.hpp
        elements/HsmFinalElement.cpp
        elements/HsmFinalElement.hpp
        elements/HsmHistoryElement.cpp
        elements/HsmHistoryElement.hpp
        elements/HsmStartElement.cpp
        elements/HsmStartElement.hpp
        elements/HsmStateElement.cpp
        elements/HsmStateElement.hpp
        elements/HsmTransition.cpp
        elements/HsmTransition.hpp

        # elements/private/ElementBoundaryGripItem.cpp
        # elements/private/ElementBoundaryGripItem.hpp
        elements/private/ElementConnectionArrow.cpp
        elements/private/ElementConnectionArrow.hpp
        elements/private/HsmConnectableElement.cpp
        elements/private/HsmConnectableElement.hpp
        elements/private/HsmElement.cpp
        elements/private/HsmElement.hpp
        elements/private/HsmResizableElement.cpp
        elements/private/HsmResizableElement.hpp

        ui/main.ui
        ui/settings.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(hsm_ide
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET hsm_ide APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(hsm_ide SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(hsm_ide
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_include_directories(hsm_ide PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hsm_ide PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(hsm_ide PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS hsm_ide
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(hsm_ide)
endif()
