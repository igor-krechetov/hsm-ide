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




layout for include state is not parsed correctly:
    <state id="NeigborState" hsm:uid="28">
        <onentry>
            <script>callbackName</script>
        </onentry>
        <onexit>
            <script>callbackName</script>
        </onexit>
        <invoke srcexpr="callbackName"/>
        <transition event="NEW_EVENT" type="external" target="FinalState1" hsm:uid="13"/>
        <transition event="NEW_EVENT" type="external" target="HistoryShallow" hsm:uid="14"/>
        <transition event="NEW_EVENT" type="external" target="HistoryDeep" cond="condition1 is true" hsm:uid="26">
            <script>onTransition</script>
        </transition>
        <xi:include href="transitions.scxml" parse="xml"/>
    </state>


double click on Include opens 2 windows.
clicking on include with invalid path doesnt produce an error


Create a joint test with hsmcpp (add it as git submodule)

- load and deserialise tests/data/manual_tests/all_elements.scxml
- serialise model into a temporary file
- generate code using scxml serializer (check that there are no errors or warnings)