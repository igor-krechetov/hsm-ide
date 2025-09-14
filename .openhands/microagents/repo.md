
---
name: hsm-ide
type: knowledge
version: 1.0.0
agent: CodeActAgent
---

# hsm-ide Repository Overview

## 1. Purpose of the Repository
The hsm-ide repository is an Integrated Development Environment (IDE) designed for creating and working with hierarchical state machines (HSMs) using the hsmcpp library. The primary goal is to provide a graphical interface for designing state machines with support for all standard HSM elements, including states, transitions, initial states, final states, and history states.

## 2. General Setup
The repository follows a structured approach with clear project rules and requirements. The project uses the following technologies:
- C++17 as the primary programming language
- Qt 6 framework for GUI development
- CMake 3.20+ for build management
- PlantUML for diagram generation
- Catch2 for unit testing

To build the project, you need to run:
```bash
./build.sh
```

The project uses a model-view-controller architecture for separation of concerns.

## 3. Repository Structure
The repository has the following key directories and files:

- **root directory**: Contains configuration files like `.gitignore`, `CMakeLists.txt`, and build scripts.
- **.clinerules**: Contains project rules and guidelines.
- **memory-bank**: Documentation and context files about the project.
- **req**: Requirements files, including `requirements.pu` which outlines the project's requirements.
- **src**: Source code organized into subdirectories:
  - **common**: Common utilities and components.
  - **controllers**: Controller logic.
  - **model**: Data models.
  - **view**: User interface components, including:
    - **common**: Common UI elements.
    - **elements**: State machine elements.
    - **widgets**: Custom widgets.

## 4. Key Components
- **Model**: State machine model with elements like states, transitions, and hierarchical relationships
- **View**: Qt-based UI components and graphics view for displaying state machines
- **Controller**: Mediates between model and view components

## 5. Current Status
- Core model and view components are implemented (70% complete)
- Basic state machine editing capabilities (50% complete)
- Project estimated at 45% completion overall

## 6. Next Steps
1. Implement transition connection logic between state elements
2. Develop serialization/deserialization for saving/loading state machines
3. Add property editors for state machine elements
4. Implement undo/redo functionality using command pattern
5. Create state machine validation and simulation features

## 7. Development Guidelines
- Before committing changes, code must be formatted with clang-format
- Commit description must be made according to the description in `.clinerules/.projectrules.md`