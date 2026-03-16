# Changelog
All notable changes to project will be documented in this file.

## [0.5.4] - 2026-03-16
### Fixed
- [REQ-205a] Removed redundant view element overrides for hierarchy checks that duplicated base `HsmElement` behavior

## [0.5.3] - 2026-03-16
### Fixed
- [REQ-205a] Updated view HSM elements to reuse shared model hierarchy rules for top-level and child-acceptance checks
- [REQ-205a] Added shared transition-parent hierarchy rule and applied it in view element validation

### New
- [REQ-205a] Extended hierarchy rule tests with transition parent compatibility checks

## [0.5.2] - 2026-03-16
### Fixed
- [REQ-205a] Unified HSM hierarchy validation rules between clipboard paste and SCXML parsing
- [REQ-205a] Paste hierarchy checks now reuse shared model rules instead of controller-local logic

### New
- [REQ-205a] Added model-level hierarchy rules utilities and unit tests
- [REQ-205a] Added serializer test for ignoring invalid hierarchy during deserialization

## [0.5.1] - 2026-03-16
### Fixed
- [REQ-205a] Copy/paste implementation updated to handle unordered multi-selection (states before transitions), context-aware paste parent selection, and element placement restrictions
- [REQ-205a] SCXML serialization now supports exporting fragments without wrapping <scxml> tag

## [0.5.0] - 2026-03-15
### New
- [REQ-205a] Copy/cut/paste HSM elements through SCXML clipboard serialization/deserialization

## [0.4.0] - 2026-03-14
### New
- [REQ-204d4] List of recent files
- [REQ-204d5] List of recent workspaces

## [0.3.0] - 2026-03-14
### New
- [REQ-204c] Undo/Redo support

## [0.2.1] - 2026-02-22
### New
- [REQ-103b] Selection rendering for HSM elements

### Fixed
- Loading of nested elements from SCXML resulted in visual duplicates
- [REQ-103b] Incorrect cursor shapes when dragging/reparenting HSM elements
- [REQ-103b] Improved showing/hiding connection arrows for HSM elements
- [REQ-103b] Rendering and scaling of grip items

## [0.2.0] - 2026-02-22
### New
- [REQ-103f1] History state must have a parent state
- [REQ-103f2] Entry point must have a parent state
- [REQ-103f3] Exit point must have a parent state
- [REQ-103f4] Initial and final states shall be top level only
- [REQ-103f5] Visual indication of restricted move action
- [REQ-103f] All hierarchy restrictions implemented

## [0.1.0] - 2025-11-30
### New
- Rendering of transition callback and condition

### Fixed
- Improved label positioning for transitions
- Fixed HsmGraphicsView not giving some key presses to child elements (couldn't type spaces in labels)

## [0.0.1] - 2025-11-29
### New
- Start of the project versioning
- CI/CD configured