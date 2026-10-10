#ifndef UNSAVEDCHANGESCHOICE_HPP
#define UNSAVEDCHANGESCHOICE_HPP

// Public outcome of the unsaved-changes prompt, shared between the view (which presents
// the prompt) and MainEditorController (which orchestrates the close). This keeps the
// view decoupled from the controller's private UnsavedChangesGuard implementation.
enum class UnsavedChangesChoice { SaveAll, Discard, Cancel };

#endif  // UNSAVEDCHANGESCHOICE_HPP
