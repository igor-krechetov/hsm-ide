# Tech Context

## Technologies Used
- C++17 as primary programming language
- Qt 6 framework for GUI development
- CMake 3.20+ for build management
- PlantUML for diagram generation
- Catch2 for unit testing

## Development Setup
- Requires Qt Creator or compatible IDE
- Build with CMake: `mkdir build && cd build && cmake .. && make`
- Format code with clang-format using provided script
- Documentation generated with Doxygen

## Technical Constraints
- Cross-platform support (Windows, Linux, macOS)
- Real-time performance for state machine simulation
- Memory efficiency for large state machines
- Compatibility with Qt 6.0+

## Dependencies
- Qt6 Core, Gui, Widgets, Svg modules
- hsmcpp library for state machine implementation
- boost::serialization for project file handling

## Tool Usage Patterns
- Qt Creator for development and debugging
- Git for version control with feature branches
- PlantUML for state machine diagram generation
- Clang-format for consistent code styling
- Catch2 for unit testing state machine logic
