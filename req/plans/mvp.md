# MVP Implementation Plan

## Overview

This document outlines the remaining work required for the MVP release of hsm-ide. It excludes items marked as `<<prio_low>>` in the requirements.

Each requirement is scoped for an independent commit/PR.

## MVP Backlog Summary

| Priority | ID | Requirement | Status | Scope |
|----------|-----|-------------|--------|-------|
| P0 | REQ-104e | Multiple Actions per Event | `<<todo>>` | Large |
| P0 | REQ-103f6 | Only one initial state allowed | `<<todo>>` | Small |
| P0 | REQ-204d6 | Warn about unsaved changes on close | `<<todo>>` | Small-Medium |
| P0 | REQ-301a1.3 | Report parse errors with line numbers | `<<todo>>` | Medium |
| P1 | REQ-103f7 | State names must be unique | `<<todo>>` | Small-Medium |
| P1 | REQ-204f | Keyboard shortcuts documentation | `<<todo>>` | Small |

---

## Detailed Requirements

### REQ-103f6: Only One Initial State Allowed in the Model

**Category:** REQ-103f Hierarchy Restrictions

**Problem:** Users can add multiple initial states to the model root, producing invalid SCXML.

**Acceptance Criteria:**
- Only one initial state is allowed at the model root level
- Attempting to add a second initial state shows visual indication (forbidden cursor), consistent with existing REQ-103f restrictions
- Drag, drop, and paste operations respect this constraint

**High-Level Plan:**
1. Extend `StateHierarchyRules` to check if an initial state already exists in the model root
2. Add `hasInitialState()` helper to `ModelRootState` or `StateMachineModel`
3. Update `HsmGraphicsView` drop validation to call the new rule
4. Update paste logic in `ProjectController::pasteScxmlElements` to filter out initial states if one exists
5. Add unit test verifying constraint enforcement

**Files to modify:**
- `src/model/StateHierarchyRules.cpp`
- `src/model/StateHierarchyRules.hpp`
- `src/model/elements/ModelRootState.cpp` (or `StateMachineModel.cpp`)
- `src/view/widgets/HsmGraphicsView.cpp`
- `src/controllers/ProjectController.cpp`

**Estimated scope:** Small (~2-3 files changed)

---

### REQ-103f7: State Names Must Be Unique Across the Model

**Category:** REQ-103f Hierarchy Restrictions

**Problem:** Users can create states with duplicate names, leading to invalid/ambiguous SCXML (transitions reference targets by name).

**Acceptance Criteria:**
- Creating a state with a name that already exists is prevented
- Renaming a state to an existing name is prevented
- Visual indication shown (forbidden cursor or inline error)
- Check is global across the entire model (not just siblings)
- Error message explains the constraint

**High-Level Plan:**
1. Add `hasStateWithName(const QString& name)` to `StateMachineModel` or `ModelRootState`
2. Add validation in state creation flow (`ProjectController::handleViewDropEvent`)
3. Add validation in state rename flow (`State::setName` or view layer)
4. Show visual feedback when name already exists
5. Add unit test for duplicate name rejection

**Files to modify:**
- `src/model/StateMachineModel.cpp`
- `src/model/StateMachineModel.hpp`
- `src/model/elements/ModelRootState.cpp`
- `src/controllers/ProjectController.cpp`
- `src/view/elements/private/HsmStateTextItem.cpp` (for rename validation)

**Estimated scope:** Small-Medium (~3-4 files changed)

---

### REQ-204d6: Warn About Unsaved Changes on Close

**Category:** REQ-204d File menu

**Problem:** Application closes without warning when projects have unsaved changes.

**Acceptance Criteria:**
- Closing the application with unsaved changes prompts the user
- Dialog shows list of unsaved projects
- User can choose: Save All / Discard / Cancel
- Closing individual project tab also prompts if modified
- Works for both "X" button close and File -> Exit

**High-Level Plan:**
1. Override `closeEvent(QCloseEvent*)` in `MainWindow`
2. Iterate all open projects, collect those where `isModified() == true`
3. If any modified, show `QMessageBox` with options
4. Handle "Save All" by iterating and calling `exportModel()` (prompt for path if new file)
5. Handle individual tab close via `projectTabCloseRequested` with same check
6. Add unit/integration test for the flow

**Files to modify:**
- `src/view/MainWindow.cpp`
- `src/view/MainWindow.hpp`

**Estimated scope:** Small-Medium (~2 files changed)

---

### REQ-301a1.3: Report Parse Errors to User With Line Numbers

**Category:** REQ-301a1 Model structure import

**Problem:** SCXML import errors are logged to console only; users see no feedback.

**Acceptance Criteria:**
- Parse errors are displayed in a dialog after import attempt
- Errors include line and column numbers where available
- Multiple errors are listed (not just first one)
- Non-fatal warnings are distinguished from fatal errors
- Dialog is shown even for partial success with warnings

**High-Level Plan:**
1. Create `ParseErrorCollector` class to accumulate errors with line/column info
2. Modify `StateMachineSerializer::handleParseError()` to store errors instead of just logging
3. Capture `mXmlReader->lineNumber()` and `columnNumber()` when error occurs
4. Replace all `// TODO: handleParseError` callsites with actual calls
5. Return error list from `deserializeFromScxml()` (or via out-parameter)
6. Create `ParseErrorDialog` widget showing error list in a table/list view
7. Show dialog in `MainWindow::handleOpenFile()` and `ProjectController::importModel()`
8. Add unit tests for error collection

**Files to modify:**
- `src/model/StateMachineSerializer.cpp`
- `src/model/StateMachineSerializer.hpp`
- `src/view/MainWindow.cpp`
- `src/controllers/ProjectController.cpp`

**Files to create:**
- `src/model/ParseErrorCollector.hpp`
- `src/model/ParseErrorCollector.cpp`
- `src/view/ParseErrorDialog.hpp` (optional, could use QMessageBox)
- `src/view/ParseErrorDialog.cpp` (optional)

**Estimated scope:** Medium (~5-6 files changed, 1-2 new files)

---

### REQ-204f: Keyboard Shortcuts Documentation

**Category:** REQ-204 Menu Features

**Problem:** Some keyboard shortcuts work but aren't visible in menus.

**Acceptance Criteria:**
- All functional shortcuts are documented in menus or tooltips
- F2 (rename), Delete, Ctrl+D (duplicate) visible somewhere
- Arrow key navigation documented if supported
- Consider adding Help -> Keyboard Shortcuts menu item (optional)

**High-Level Plan:**
1. Audit all `keyPressEvent` handlers for undocumented shortcuts
2. Add shortcuts to corresponding menu `QAction` items where possible
3. Add tooltips with shortcut hints where menu item doesn't exist
4. (Optional) Create simple shortcuts reference dialog

**Files to modify:**
- `src/view/ui/main.ui` (add shortcuts to QAction items)
- `src/view/MainWindow.cpp` (tooltips if needed)

**Estimated scope:** Small (~2-3 files changed)

---

### REQ-104e: Multiple Actions per Event

**Category:** REQ-104 State Behavior Configuration

**Problem:** States and transitions support only a single action, but real HSMs often need multiple entry/exit/transition actions.

**Acceptance Criteria:**
- States can have multiple onentry and onexit actions
- Transitions can have multiple actions
- Properties editor allows adding, removing, and reordering actions
- SCXML serialization handles multiple `<script>` elements within `<onentry>`/`<onexit>`
- Existing single-action HSMs continue to work (backward compatible)

**High-Level Plan:**
1. Create `CompositeAction` class implementing `IModelAction`, containing `QList<QSharedPointer<IModelAction>>`
2. Update `RegularState`, `ExitPoint`, `FinalState`, `Transition` to use composite actions internally
3. Modify serialization to write multiple `<script>` elements
4. Modify deserialization to read multiple actions into composite
5. Update `StateMachineEntityViewModel` to expose actions as expandable tree nodes
6. Update Properties editor delegate to support add/remove buttons for action list
7. Add unit tests for composite action serialization round-trip
8. Add UI test for adding multiple actions

**Files to modify:**
- `src/model/elements/RegularState.cpp`
- `src/model/elements/RegularState.hpp`
- `src/model/elements/ExitPoint.cpp`
- `src/model/elements/ExitPoint.hpp`
- `src/model/elements/FinalState.cpp`
- `src/model/elements/FinalState.hpp`
- `src/model/elements/Transition.cpp`
- `src/model/elements/Transition.hpp`
- `src/model/StateMachineSerializer.cpp`
- `src/view/models/StateMachineEntityViewModel.cpp`
- `src/view/widgets/private/HsmEntityPropertyDelegate.cpp`

**Files to create:**
- `src/model/actions/CompositeAction.hpp`
- `src/model/actions/CompositeAction.cpp`

**Estimated scope:** Large (~8-10 files changed, 1-2 new files)

---

## Suggested Implementation Order

```
Phase 1 - Quick Wins (small scope):
  1. REQ-103f6:  Only one initial state allowed
  2. REQ-204f:   Keyboard shortcuts documentation

Phase 2 - Core Safety (P0):
  3. REQ-204d6:  Warn about unsaved changes on close
  4. REQ-103f7:  State names must be unique

Phase 3 - Error Handling (P0):
  5. REQ-301a1.3: Report parse errors with line numbers

Phase 4 - Feature Completion (P0):
  6. REQ-104e:   Multiple actions per event
```

---

## Housekeeping

### TODO.md Cleanup

The TODO.md file contains outdated items that were fixed in v0.13.0. Should be updated to reflect current state:

**Items to remove (already fixed):**
- "Final state serialisation with event needs fixing"
- "SCXML is not serialised correctly"

**Items to keep/update:**
- Localisation support (post-MVP)
- Indication of currently opened workspace (post-MVP)
- UX improvements for grip nodes (post-MVP)

---

## Code TODOs Summary

The following code TODOs are addressed by this MVP plan:

| TODO Location | Addressed By |
|---------------|--------------|
| `HsmGraphicsView.cpp:440` - check if element allowed top level | REQ-103f6 |
| `StateMachineSerializer.cpp:244` - handleParseError | REQ-301a1.3 |
| Multiple serializer TODOs about error handling | REQ-301a1.3 |

Note: The `// TODO: block outgoing connections` comments in `HsmFinalElement.cpp`
and `HsmExitPointElement.cpp` are stale. That functionality is already implemented
via `isConnectable()` returning `false`. These comments should be removed as part
of housekeeping.

Remaining code TODOs are technical debt items deferred to post-MVP.
