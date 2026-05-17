find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

set(TEST_OBJECTS
    model/ModelTypesTest.cpp
    model/StateMachineEntityTest.cpp
    model/RegularStateTest.cpp
    model/HistoryStateTest.cpp
    model/InitialStateTest.cpp
    model/FinalStateTest.cpp
    model/EntryPointTest.cpp
    model/ExitPointTest.cpp
    model/TransitionTest.cpp
    model/ModelActionsTest.cpp
    model/IncludeEntityTest.cpp
    model/ModelRootStateTest.cpp
    model/ModelElementsFactoryTest.cpp
    model/StateMachineModelTest.cpp
    model/StateHierarchyRulesTest.cpp
    model/StateMachineSerializer/StateMachineSerializerSerializationTest.cpp
    model/StateMachineSerializer/StateMachineSerializerDeserializationTest.cpp
    view/HsmGraphicsViewTest.cpp
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
