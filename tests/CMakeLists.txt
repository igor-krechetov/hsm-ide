


# Find Qt components
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Test)

enable_testing()

set(TEST_SOURCES
    StateMachineSerializerTest.cpp
)

add_executable(StateMachineSerializerTests ${TEST_SOURCES})

target_include_directories(StateMachineSerializerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/model
)

target_link_libraries(StateMachineSerializerTests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    model
)

add_test(NAME StateMachineSerializerTests COMMAND StateMachineSerializerTests)


