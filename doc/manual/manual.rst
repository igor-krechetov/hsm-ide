.. _code-generation-hsm-ide-manual:

######################
HSM-IDE User Manual
######################


.. contents::
   :local:


.. note:: HSM-IDE is under active development. Menus, shortcuts, and panels described here
          reflect the current release and may change. Screenshots marked *(screenshot
          pending)* are placeholders to be captured for a future documentation update.


Interface overview
===================

The HSM-IDE window is organized around a central editing canvas, a side panel that can be
switched between several views, and a menu bar and toolbar for actions.

.. figure:: ../_images/hsm-ide/overview.png
   :align: center
   :alt: HSM-IDE main window overview

   *(screenshot pending)* Main window: menu bar, toolbar, editing canvas, and side panel.

The side panel can be switched between three views:

-  **Workspace** (``Ctrl+Shift+B``) -- browse the files and HSMs of the current workspace.
-  **Design** / HSM Elements (``Ctrl+Shift+E``) -- inspect and edit the elements of the
   HSM currently open on the canvas.
-  **Debug** (``Ctrl+Shift+D``) -- tools for inspecting HSM execution.

Multiple HSM documents can be open at once, each in its own tab above the canvas.


Working with workspaces and files
=================================

A *workspace* groups the HSM files you are working on. Workspace and file operations live
in the **File** menu.

Workspaces
----------

-  **Open Workspace** -- open an existing workspace folder.
-  **Close Workspace** -- close the current workspace.
-  **Recent Workspaces** -- reopen a recently used workspace.

HSM files
---------

-  **New HSM** (``Ctrl+N``) -- create a new, empty state machine.
-  **Open HSM** (``Ctrl+O``) -- open an existing SCXML file.
-  **Recent Files** -- reopen a recently used file.
-  **Save** (``Ctrl+S``) / **Save As...** -- write the current HSM to disk.
-  **Close Current** (``Ctrl+W``) / **Close All** -- close the active document or all
   open documents.

.. figure:: ../_images/hsm-ide/file-menu.png
   :align: center
   :alt: HSM-IDE File menu

   *(screenshot pending)* The File menu with workspace and HSM file operations.


Creating and editing a state machine
=====================================

Add states to the canvas from the **Design** panel and connect them with transitions.
Editing operations are available from the **Edit** menu, the toolbar, and the canvas
context menu.

-  **Edit** (``F2``) -- rename or edit the selected element.
-  **Duplicate** (``Ctrl+D``) -- duplicate the selected element(s).
-  **Delete** (``Del``) -- remove the selected element(s).
-  **Cut** (``Ctrl+X``) / **Copy** (``Ctrl+C``) / **Paste** (``Ctrl+V``) -- standard
   clipboard operations on elements.
-  **Select All** (``Ctrl+A``) -- select every element on the canvas.
-  **Undo** (``Ctrl+Z``) / **Redo** (``Ctrl+Shift+Z``) -- step backward and forward
   through your edits.
-  **Find** (``Ctrl+F``) -- locate an element by name.

.. figure:: ../_images/hsm-ide/editing.png
   :align: center
   :alt: Editing a state machine in HSM-IDE

   *(screenshot pending)* Editing states and transitions on the canvas.

For details on how hsmcpp-specific behavior (state callbacks, timer actions, conditional
transitions, conditional entry points) is expressed, see :ref:`code-generation-editors`.


Navigating the canvas
======================

The **View** menu controls the canvas display:

-  **Show Grid** / **Snap to Grid** -- toggle the background grid and grid snapping.
-  **Zoom In** (``Ctrl+=``) / **Zoom Out** (``Ctrl+-``) -- change the zoom level.
-  **Reset Zoom** (``Ctrl+0``) -- return to 100%.
-  **Fit to View** (``Ctrl+Shift+0``) -- zoom so the whole diagram is visible.

.. figure:: ../_images/hsm-ide/view-navigation.png
   :align: center
   :alt: Canvas navigation controls

   *(screenshot pending)* Grid and zoom controls in the View menu and toolbar.


Keyboard shortcuts
===================

.. list-table::
   :header-rows: 1
   :widths: 40 20

   * - Action
     - Shortcut
   * - New HSM
     - ``Ctrl+N``
   * - Open HSM
     - ``Ctrl+O``
   * - Save
     - ``Ctrl+S``
   * - Close Current
     - ``Ctrl+W``
   * - Undo
     - ``Ctrl+Z``
   * - Redo
     - ``Ctrl+Shift+Z``
   * - Cut
     - ``Ctrl+X``
   * - Copy
     - ``Ctrl+C``
   * - Paste
     - ``Ctrl+V``
   * - Delete
     - ``Del``
   * - Duplicate
     - ``Ctrl+D``
   * - Edit / Rename
     - ``F2``
   * - Find
     - ``Ctrl+F``
   * - Select All
     - ``Ctrl+A``
   * - Zoom In
     - ``Ctrl+=``
   * - Zoom Out
     - ``Ctrl+-``
   * - Reset Zoom
     - ``Ctrl+0``
   * - Fit to View
     - ``Ctrl+Shift+0``
   * - Switch to Design panel
     - ``Ctrl+Shift+E``
   * - Switch to Workspace panel
     - ``Ctrl+Shift+B``
   * - Switch to Debug panel
     - ``Ctrl+Shift+D``


Getting help
============

The **Help** menu provides:

-  **Project Wiki** -- open the online documentation and project wiki.
-  **About** -- version and license information.
