find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

enable_testing()

set(TEST_SOURCES
    TestMain.cpp
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
)

add_executable(ModelTests ${TEST_SOURCES})

target_include_directories(ModelTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/model
)

target_link_libraries(ModelTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    model
)

target_compile_definitions(ModelTests PRIVATE
    HSM_IDE_TEST_SCXML_ROOT_DEFAULT="${CMAKE_SOURCE_DIR}/tests/data/scxml"
)

add_test(NAME ModelTests COMMAND ModelTests)
