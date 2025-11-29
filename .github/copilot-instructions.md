# Copilot Coding Agent Onboarding Instructions for hsm-ide

## Repository Summary
- **Project**: hsm-ide — Desktop IDE for designing hierarchical state machines (HSMs) with graphical editing, validation, simulation, and code/documentation export.
- **Primary Language**: C++17
- **Frameworks/Tools**: Qt 6, CMake 3.20+, PlantUML, Doxygen, Catch2 (for tests), hsmcpp
- **Target Platforms**: Linux, Windows, macOS (cross-platform)
- **Architecture**: Model-View-Controller (MVC) with Qt-based UI and graphics view for state machine diagrams.
- **Repository Size**: Medium (multiple subdirectories for src, tests, docs, memory-bank, etc.)

## Build, Test, and Validation Instructions

### Environment Setup
- **ALWAYS** run `setup.sh` before building for the first time or after environment changes. This script:
  - Installs all required system dependencies (Qt6, CMake, Boost, Doxygen, PlantUML, clang-format, etc.)
  - Prepares the build directory and runs CMake configuration.
  - Usage:
    ```bash
    ./setup.sh
    ```
- **Qt**: Requires Qt 6 (qt6-base-dev, qt6-tools-dev, qt6-tools-dev-tools, libqt6svg6-dev). Ensure these are installed (setup.sh does this).
- **CMake**: Minimum version 3.5 (3.20+ recommended).
- **Compiler**: GCC or Clang with C++17 support.

### Building
- **Standard build** (after setup):
  ```bash
  ./build.sh
  ```
  - This script sources environment variables, creates the build directory, and runs the build using custom functions (`qt6_env`, `normal_build`).
  - If `build.sh` fails due to missing shell functions, use the simple build:
    ```bash
    ./build_simple.sh
    ```
    - This runs `cmake ..` and `make` in the build directory.
- **Manual build**:
  ```bash
  mkdir -p build
  cd build
  cmake ..
  make
  ```
- **Build output**: Main binary is at `build/src/hsm_ide`.

### Running
- **Run the application**:
  ```bash
  ./build/src/hsm_ide
  ```

### Testing
- **Unit tests** are in `tests/` and built to `build/tests/StateMachineSerializerTests`.
- **Run tests**:
  ```bash
  ./build/tests/StateMachineSerializerTests
  ```
- **Test framework**: Catch2 (integrated via QtTest in test sources).

### Formatting
- **Code style**: Enforced by `.clang-format` (Google style, customizations).
- Always use a single point of return per function when writing code
- Use ifdef guards instead of pragma once for headers.
- **Format all code**:
  ```bash
  ./format.sh
  ```
  - Requires `clang-format-14`.

### Documentation
- **Doxygen** and **PlantUML** are used for documentation and diagrams.
- **Docs**: See `doc/` for diagrams and documentation sources.

### Cleaning/Rebuilding
- To clean, remove the `build/` directory and rebuild from scratch.
- If you encounter build errors, always try re-running `setup.sh` and then a clean build.

## Project Layout and Key Files
- **Root**: `CMakeLists.txt`, `build.sh`, `build_simple.sh`, `setup.sh`, `format.sh`, `.clang-format`, `README.md`, `LICENSE`, `doc/`, `include/`, `src/`, `tests/`, `memory-bank/`, `.vscode/` (VSCode configs)
- **Source**: `src/` (main code), `src/main.cpp` (entry point), `src/model/`, `src/view/`, `src/controllers/`
- **Tests**: `tests/` (unit tests)
- **Headers**: `include/`
- **Docs**: `doc/`
- **Build output**: `build/`
- **VSCode**: `.vscode/` (tasks, launch configs, include paths)

## Validation and CI
- **No explicit GitHub Actions or CI workflows found**. Validation is manual: build, run, test, and format as above.
- **Pre-checkin**: Always ensure code builds, passes all tests, and is formatted.
- **Memory Bank**: For project context, always consult `memory-bank/` files for requirements, architecture, and current focus.

## Additional Notes
- **MVC**: Strict separation between model, view, and controller. See `src/model/`, `src/view/`, `src/controllers/`.
- **Naming**: Classes for state machine elements use `Hsm` prefix. Consistent naming and smart pointers are preferred.
- **Undo/Redo**: Command pattern is used for undo/redo (see systemPatterns.md).
- **Export/Import**: State machines can be serialized/deserialized (XML format).
- **If in doubt**: Trust these instructions. Only search the codebase if information here is incomplete or appears incorrect.

---

## Root Directory File List
- .clang-format
- .github/
- .gitignore
- .vscode/
- CMakeLists.txt
- LICENSE
- README.md
- build.sh
- format.sh
- setup.sh
- src/
- include/
- tests/
- doc/
- req/

## README.md
```
# hsm-ide
IDE for designing and working with hsmcpp library.
```

## main.cpp (snippet)
```
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    // ...
    MainEditorController controller;
    return controller.start();
}
```

---

**End of Copilot onboarding instructions.**
