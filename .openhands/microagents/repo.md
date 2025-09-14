
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
The repository follows a structured approach with clear project rules and requirements. To build the project, you need to run the `./build.sh` script, which sets up the environment and compiles the code. The project uses a model-view-controller architecture for separation of concerns.

## 3. Repository Structure
The repository has the following key directories and files:

- **root directory**: Contains configuration files like `.gitignore`, `CMakeLists.txt`, and build scripts.
- **.clinerules**: Contains project rules and guidelines.
- **doc**: Documentation files.
- **include**: Header files for the project.
- **memory-bank**: Documentation and context files about the project.
- **req**: Requirements files, including `requirements.pu` which outlines the project's requirements.
- **src**: Source code organized into subdirectories:
  - **common**: Common utilities and components.
  - **controllers**: Controller logic.
  - **model**: Data models.
  - **view**: User interface components, including:
    - **common**: Common UI elements.
    - **elements**: State machine elements.
    - **res**: Resources like images.
    - **ts**: TypeScript files for UI.
    - **ui**: UI definitions.
    - **widgets**: Custom widgets.

The project is designed to be extensible and follows a modular approach, making it easier to maintain and develop new features.

## 4. Development Guidelines
- Before committing changes, code must be formatted with clang-format
- Commit description must be made according to the description in `.clinerules/.projectrules.md`