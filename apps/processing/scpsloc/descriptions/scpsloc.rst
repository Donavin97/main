:orphan:

.. _scpsloc:

=======
scpsloc
=======

**scpsloc** is an advanced automatic earthquake locator that uses both P and S
phases for improved location accuracy, particularly for depth resolution.

Overview
========

scpsloc is based on scautoloc but extends its capabilities by:

* Using both P and S phases for location
* Supporting depth phases (pP, sP, sS) for improved depth resolution
* Computing S/P phase ratios for quality control
* Enhanced phase scoring that gives appropriate weight to S phases
* Better azimuthal gap calculations considering both P and S phases

Key Features
============

P and S Phase Location
----------------------
Unlike traditional locators that primarily use P phases, scpsloc automatically
associates and uses S phases (S, SKS, ScS, etc.) to provide additional constraints
on the hypocenter. This is particularly valuable for:

* Improving depth resolution
* Better constraining the epicenter
* Providing more robust locations with fewer stations

Depth Phase Support
-------------------
scpsloc can automatically detect and use depth phases (pP, sP, sS) when available.
These phases are extremely valuable for constraining the focal depth of earthquakes,
especially for deeper events where depth resolution is typically poor.

Quality Metrics
---------------
scpsloc computes additional quality metrics including:

* P phase count
* S phase count  
* S/P phase ratio

These metrics help users assess the quality and reliability of locations.

Configuration
=============

scpsloc can be configured through the SeisComP configuration system. Key parameters
include:

locator.profile
   The velocity model to use (e.g., iasp91, tab)

autoloc.useSPhases
   Enable/disable S phase usage (default: true)

autoloc.minSPRatio
   Minimum required S/P phase ratio for quality control (default: 0.0)

autoloc.sPhaseWeight
   Weight multiplier for S phases (default: 1.2)

autoloc.useDepthPhases
   Enable/disable depth phase usage (default: true)

autoloc.maxSPResidual
   Maximum S-P time residual for association in seconds (default: 5.0)

Usage
=====

scpsloc runs as a standard SeisComP module::

   $ seiscomp start scpsloc

It will automatically connect to the messaging system and begin processing picks
from scautopick or other pick sources.

See Also
========

* :ref:`scautoloc` - Standard automatic locator
* :ref:`scautopick` - Automatic phase picker
* :ref:`scolv` - Interactive event review and location
