# Northern Mechatronics Software Development Kit

## Overview
The NMSDK is a platform library for the Northern Mechatronics NM180100 LoRa BLE module.  It provides support for LoRa direct, LoRa real-time, LoRaWAN, and BLE wireless connectivity as well as a FreeRTOS framework for rapid application development across a wide range of use cases and environments.

The SDK provides a centralized interface between the application layer and other radio stacks.  This allows each feature to evolve independently.

License : [![License](https://img.shields.io/badge/license-BSD_3-blue.svg)]
Platform Details: [![Hardware](https://img.shields.io/badge/hardware-wiki-green.svg)]

## Supported Host Platforms

For more details on building the SDK on a host platform, please check the **User Guide** specified below for either the Windows OS or the Linux OS.

## SDK Documentation

From an architectural perspective, the SDK consists of three layers.

## Directory Structure
| Directory | Description |
| --------- | ----------- |
| bsp | Pre-defined board support packages |
| features | SDK bindings and build scripts to other software packages |
| platform | NM platform service code |
