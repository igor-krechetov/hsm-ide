find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

set(TEST_OBJECTS
    model/ModelTypesTest.cpp
    model/EntityIdGeneratorTest.cpp
    model/EntityIdGeneratorPropertyTest.cpp
    model/ModelIntegrationPropertyTest.cpp
    model/StateMachineEntityTest.cpp
    
    model/ModelActionsTest.cpp
    model/ModelElementsFactoryTest.cpp
    model/StateMachineModelTest.cpp
    model/StateHierarchyRulesTest.cpp
    model/StateMachineSerializer/StateMachineSerializerSerializationTest.cpp
    model/StateMachineSerializer/StateMachineSerializerDeserializationTest.cpp
    model/StateMachineSerializer/SerializerPropertyTest.cpp
    model/StateMachineSerializer/FormatDetectionTest.cpp
    model/StateMachineSerializer/HsmUidSerializationTest.cpp
    model/StateMachineSerializer/HsmLayoutSerializerTest.cpp
    model/StateMachineSerializer/FormatRoundTripIntegrationTest.cpp
    model/StateMachineSerializer/QtGeometryDeserializationTest.cpp

    model/elements/RegularStateTest.cpp
    model/elements/HistoryStateTest.cpp
    model/elements/InitialStateTest.cpp
    model/elements/FinalStateTest.cpp
    model/elements/EntryPointTest.cpp
    model/elements/ExitPointTest.cpp
    model/elements/TransitionTest.cpp
    model/elements/IncludeEntityTest.cpp
    model/elements/ModelRootStateTest.cpp

    view/HsmGraphicsViewTest.cpp
    view/HsmElementDragTest.cpp
    view/AutoGroupItemCursorTest.cpp
    view/HsmTransitionGripVisibilityTest.cpp
)

foreach(testObject ${TEST_OBJECTS})
    get_filename_component(testName ${testObject} NAME_WE)

    if(COMMAND qt_add_test)
        qt_add_test(${testName}
            SOURCES
                TestMain.cpp
                ${testObject}
        )
    else()
        add_executable(${testName}
            TestMain.cpp
            ${testObject}
        )
        add_test(NAME ${testName} COMMAND ${testName})
    endif()

    target_include_directories(${testName} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/model
        ${CMAKE_SOURCE_DIR}/src/model/elements
        ${CMAKE_SOURCE_DIR}/src/model/private
        ${CMAKE_SOURCE_DIR}/src/model/private/serializer
        ${CMAKE_SOURCE_DIR}/src/view
    )

    target_link_libraries(${testName} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        model
        view
    )

    target_compile_definitions(${testName} PRIVATE
        HSM_IDE_TEST_SCXML_ROOT_DEFAULT="${CMAKE_SOURCE_DIR}/tests/data/scxml"
    )
endforeach()
