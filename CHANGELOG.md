# Changelog
All notable changes to project will be documented in this file.

## [0.9.0] - 2026-04-05
### New
- Duplicate state elements through menu or Ctrl+D

## [0.8.1] - 2026-04-05
### Fixed
- Sceene was loosing selection when interacting with main menu

## [0.8.0] - 2026-03-29
### New
- [REQ-204a6] Added Edit -> Select All action handling in MainWindow to select all HSM elements in the active canvas.

## [0.7.0] - 2026-03-29
### New
- Code restructuring to properly support element actions
- [REQ-205c6] UI editor for HSM element actions

## [0.6.0] - 2026-03-28
### New
- Workspace tree now lists nested subfolders recursively and shows SCXML files from all levels.
- Added toggle button to show/hide empty folders in workspace tree (default: hidden).
- Added workspace context menu actions for files/folders: New File, New Folder, Rename, Delete.
- Added collapse workspace tree button
- Enabled drag-and-drop move for files and folders inside the workspace tree.

## [0.5.1] - 2026-03-28
### Fix
- [REQ-103c3] Resolve paste parent correctly when multiple sibling states are selected. If there is no selected ancestor that contains all selected states, paste now targets their common direct parent (or root if parents differ).
- [REQ-103c3] Keeps existing cursor-anchor placement behavior while preserving relative positions for multi-element paste groups.
- [REQ-103c3] Recalculate transition grip geometry metadata during paste by applying paste offset to GEOMETRY points.
- [REQ-103c3] Ensures pasted transitions keep correct shape/location relative to moved pasted states.


## [0.5.0] - 2026-03-16
### New
- [REQ-103c1, REQ-103c2, REQ-103c3] Copy, cut, paste HSM elements through SCXML clipboard serialization/deserialization

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
