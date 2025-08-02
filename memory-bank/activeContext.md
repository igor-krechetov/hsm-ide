# Active Context

## Current Work Focus
- Implementing the graphical editor interface using Qt Graphics View framework
- Developing state machine element classes (states, transitions, history markers)
- Connecting model and view components via controller classes
- Implementing drag-and-drop functionality for state machine elements

## Recent Changes
- Added HsmElementsFactory for creating state machine elements
- Implemented HsmGraphicsView for displaying state machine diagrams
- Created base classes for state machine elements (HsmElement, HsmConnectableElement)
- Added resource files for element icons

## Next Steps
1. Implement transition connection logic between state elements
2. Develop serialization/deserialization for saving/loading state machines
3. Add property editors for state machine elements
4. Implement undo/redo functionality using command pattern
5. Create state machine validation and simulation features

## Active Decisions and Considerations
- Choosing between QGraphicsScene and custom rendering for performance
- Deciding on state machine persistence format (XML vs custom binary)
- Balancing flexibility and performance in state machine simulation
- Determining optimal abstraction level for state machine elements

## Important Patterns and Preferences
- Strict adherence to MVC architecture
- Use of smart pointers for memory management
- Qt signal-slot connections for component communication
- RAII pattern for resource management
- Consistent naming conventions: Hsm prefix for state machine classes

## Learnings and Project Insights
- Qt Graphics View provides powerful capabilities but has performance limitations
- Hierarchical state management requires careful parent-child relationships
- Element selection and manipulation is more complex than anticipated
- Undo/redo implementation benefits from command pattern encapsulation
