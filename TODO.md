- Add localisation support
- Add indication of the currently opened workspace

- Hierarchy rules
-- only one initial element is allowed (consider)
- Create consts for all model element keys


- Final state serialisation with event needs fixing
- test with large HSM


ONGOING:

UX improvements:
Add a way to grip nodes when creating a transition (keep adding nodes until user clicks on a conectable element). ESC or loose of focus cancels it.
Resize grip and transition connection grips overlap.

Design decision:
- what will transition with event do if defined for initial state?

In some specific case moving transition grips starts to render incorrectly.


