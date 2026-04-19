find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets LinguistTools)

set(DEBUG_RENDERING True)

if(DEBUG_RENDERING)
    add_definitions(-DDEBUG_RENDERING=1)
endif()

set(TS_FILES view/ts/hsm_ide_en_001.ts)

set(RESOURCE_FILES view/res/resources.qrc)

# Create a model library target
set(MODEL_SOURCES
    model/ModelTypes.cpp
    model/ModelUtils.cpp
    model/EntryPoint.cpp
    model/ExitPoint.cpp
    model/FinalState.cpp
    model/HistoryState.cpp
    model/IncludeEntity.cpp
    model/InitialState.cpp
    model/ModelRootState.cpp
    model/RegularState.cpp
    model/State.cpp
    model/StateMachineModel.cpp
    model/Transition.cpp
    model/ModelElementsFactory.cpp
    model/StateMachineEntity.cpp
    model/StateMachineSerializer.cpp
    model/StateHierarchyRules.cpp
    model/actions/CallbackAction.cpp
    model/actions/NoneAction.cpp
    model/actions/TimerStartAction.cpp
    model/actions/TimerStopAction.cpp
    model/actions/TimerRestartAction.cpp
    model/actions/SendEventAction.cpp
    model/actions/ModelActionFactory.cpp
)

set(PROJECT_SOURCES
    main.cpp
    common/ObjectUtils.cpp

    controllers/MainEditorController.cpp
    controllers/ModificationHistoryController.cpp
    controllers/ProjectController.cpp
    controllers/SettingsController.cpp
    # controllers/RecentFilesManager.cpp

    view/MainWindow.cpp
    view/AboutDialog.cpp

    view/common/ViewUtils.cpp

    view/theme/ThemeManager.cpp

    view/widgets/HsmElementsList.cpp
    view/widgets/HsmGraphicsView.cpp
    view/widgets/HsmTreeView.cpp
    view/widgets/HsmPropertiesTableView.cpp
    view/widgets/WorkspaceView.cpp
    view/widgets/QClickableSlider.cpp
    view/widgets/QImageViewArea.cpp
    view/widgets/private/HsmEntityPropertyDelegate.cpp
    view/widgets/private/HsmElementsListDelegate.cpp

    view/elements/HsmElementsFactory.cpp
    view/elements/HsmEntryPointElement.cpp
    view/elements/HsmExitPointElement.cpp
    view/elements/HsmFinalElement.cpp
    view/elements/HsmHistoryElement.cpp
    view/elements/HsmIncludeElement.cpp
    view/elements/HsmInitialElement.cpp
    view/elements/HsmStateElement.cpp
    view/elements/HsmTransition.cpp

    view/elements/private/ElementBoundaryGripItem.cpp
    view/elements/private/ElementConnectionArrow.cpp
    view/elements/private/ElementGripItem.cpp
    view/elements/private/HsmConnectableElement.cpp
    view/elements/private/HsmElement.cpp
    view/elements/private/HsmResizableElement.cpp
    view/elements/private/HsmStateTextItem.cpp
    view/elements/private/HsmStateBodySection.cpp
    view/elements/private/AutoGroupItem.cpp

    view/elements/private/styles/HsmRectangularElement.cpp
    view/elements/private/styles/HsmRoundElement.cpp

    $<$<BOOL:${DEBUG_RENDERING}>:view/elements/private/DebugRendering.cpp>

    view/models/StateMachineTreeModel.cpp
    view/models/StateMachineEntityViewModel.cpp
    view/models/FilteredFileSystemProxyModel.cpp

    view/ui/main.ui
    view/ui/settings.ui
    view/ui/about.ui
    ${TS_FILES}
)

qt_add_library(model STATIC ${MODEL_SOURCES})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_resources(GENERATED_RESOURCE_FILES ${RESOURCE_FILES})
    list(APPEND PROJECT_SOURCES ${GENERATED_RESOURCE_FILES})
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
    qt5_add_resources(GENERATED_RESOURCE_FILES ${RESOURCE_FILES})
    list(APPEND PROJECT_SOURCES ${GENERATED_RESOURCE_FILES})
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

target_include_directories(model PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt${QT_VERSION_MAJOR}Core_DIR}/../../../../../.
)

target_link_libraries(model PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(hsm_ide PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                           ${CMAKE_SOURCE_DIR}/include
                                           $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/gen>)
target_link_libraries(hsm_ide PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets model)

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
