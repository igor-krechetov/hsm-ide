.. _code-generation-hsm-ide:

######################
HSM-IDE
######################


.. contents::
   :local:


.. note:: HSM-IDE is under active development. Not all features are implemented yet, and
          the UI may change between releases.


Overview
========

**HSM-IDE** is a desktop application for visually designing, editing, and validating
hierarchical state machines (HSMs) through a graphical interface. You can create, manage,
and simulate state machines with SCXML support, and export your work for use with the
hsmcpp library.

Compared with the general-purpose editors described in :ref:`code-generation-editors`,
HSM-IDE is built specifically around hsmcpp: it understands hsmcpp-specific concepts
(state callbacks, timer actions, conditional transitions, conditional entry points) and
produces SCXML that the :ref:`code-generation` toolchain consumes directly.


Key Features
============

-  Graphical editor for hierarchical state machines (SCXML).
-  Import and export of SCXML files.
-  Designed for both generic SCXML and hsmcpp-specific features.
-  Multi-document design with support for includes.
-  Integrated validation and simulation tools.
-  Export to PlantUML, PNG, and SVG.
-  Cross-platform: Ubuntu and Windows.


Getting HSM-IDE
===============

HSM-IDE is developed in its own repository. Sources, build instructions, and release
binaries are available on GitHub:

-  `HSM-IDE repository <https://github.com/igor-krechetov/hsm-ide>`__
-  `Releases <https://github.com/igor-krechetov/hsm-ide/releases>`__


Limitations
===========

-  HSM-IDE does not yet provide full SCXML format support. For the subset of SCXML used by
   hsmcpp and its constraints, see :ref:`code-generation-scxml`.


.. toctree::
   :caption: Content
   :maxdepth: 2

   manual/manual
