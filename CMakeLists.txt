cmake_minimum_required(VERSION 3.5)
project(hsm_ide VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_VERSION "0.10.0")

# Get build date
string(TIMESTAMP HSM_IDE_BUILD_DATE "%Y-%m-%d")

# Get git commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE HSM_IDE_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Generate version.hpp from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in
    ${CMAKE_BINARY_DIR}/gen/version.hpp
    @ONLY
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
