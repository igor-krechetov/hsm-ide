find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets LinguistTools)

add_definitions(-DDEBUG_RENDERING=1)

set(TS_FILES view/ts/hsm_ide_en_001.ts)

set(PROJECT_SOURCES
        main.cpp
        common/ObjectUtils.cpp

        controllers/MainEditorController.cpp
        controllers/ProjectController.cpp
        # controllers/RecentFilesManager.cpp

        model/ModelTypes.cpp
        model/EntryPoint.cpp
        model/ExitPoint.cpp
        model/FinalState.cpp
        model/HistoryState.cpp
        model/InitialState.cpp
        model/RegularState.cpp
        model/State.cpp
        model/StateMachineModel.cpp
        model/Transition.cpp
        model/ModelElementsFactory.cpp
        model/StateMachineEntity.cpp


        view/MainWindow.cpp

        view/common/ViewUtils.cpp

        view/widgets/HsmElementsList.cpp
        view/widgets/HsmGraphicsView.cpp
        view/widgets/HsmTreeView.cpp
        view/widgets/QClickableSlider.cpp
        view/widgets/QImageViewArea.cpp
        view/widgets/HsmEntityPropertyDelegate.cpp

        view/elements/HsmElementsFactory.cpp
        view/elements/HsmEntryPointElement.cpp
        view/elements/HsmExitPointElement.cpp
        view/elements/HsmFinalElement.cpp
        view/elements/HsmHistoryElement.cpp
        view/elements/HsmInitialElement.cpp
        view/elements/HsmStateElement.cpp
        view/elements/HsmTransition.cpp

        view/elements/private/ElementBoundaryGripItem.cpp
        view/elements/private/ElementConnectionArrow.cpp
        view/elements/private/ElementGripItem.cpp
        view/elements/private/HsmConnectableElement.cpp
        view/elements/private/HsmElement.cpp
        view/elements/private/HsmResizableElement.cpp

        view/elements/private/styles/HsmRectangularElement.cpp
        view/elements/private/styles/HsmRoundElement.cpp

        view/models/StateMachineTreeModel.cpp
        view/models/StateMachineEntityViewModel.cpp

        view/ui/main.ui
        view/ui/settings.ui
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

target_include_directories(hsm_ide PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                           ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hsm_ide PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

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
