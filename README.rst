**************************
HABOSC Command Module (SW)
**************************

.. image:: https://img.shields.io/github/tag/johnmlarkin/whitworth-commandmodule.svg?label=version&style=plastic   :alt: GitHub tag (latest SemVer)
.. image:: https://img.shields.io/github/license/johnmlarkin/whitworth-commandmodule.svg?style=plastic   :alt: GitHub license
.. image:: https://img.shields.io/badge/platform-mbed-lightgrey.svg?style=plastic   :alt: Platform
.. image:: https://img.shields.io/badge/microcontroller-lpc1768-lightgrey.svg?style=plastic   :alt: Microcontroller

Overview
========

The High Altitude Balloon Open Source Collection (HABOSC) is a set of interrelated tools for transmitting telemetry from a high altitude balloon, processing and storing that data, and providing user access to the stored data both during and after the flight. The system is designed to be flexible so it can adapt to the needs of various users.

One of the core components of the HABOSC is the **Command Module**. This is the essential hardware system carried by the balloon. Tracking data is relayed to the ground during flight using a GPS receiver and an Iridium satellite network modem. The operations of the Command Module are coordinated by a microcontroller. The microcontroller's software is written in C++ using the Arm\ :sup:`®` Mbed\ :sup:`™` OS.

This repository contains the *software* portion of the Command Module and supporting documentation.