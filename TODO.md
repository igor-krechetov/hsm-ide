- Add localisation support
- Add indication of the currently opened workspace

- Hierarchy rules
-- only one initial element is allowed (consider)
- hsm element selection and highlight have same effect
- App version in about dlg
- Create consts for all model element keys



UX improvements:



Add a way to grip nodes when creating a transition (keep adding nodes until user clicks on a conectable element). ESC or loose of focus cancels it.
Transition should be deleted if target state is deleted.
Resize grip and transition connection grips overlap.
Start changing title when user starts typing on a selected node/transition
Labels for self transitions are incorrectly aligned.
Transitions have a gap with substates.

Design decision:
- what will transition with event do if defined for initial state?