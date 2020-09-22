# Northern Mechatronics Software Development Kit

## Overview
The NMSDK is a platform library for the Northern Mechatronics NM180100 LoRa BLE module.  It provides support for LoRa direct, LoRa real-time, LoRaWAN, and BLE wireless connectivity as well as a FreeRTOS framework for rapid application development across a wide range of use cases and environments.

The SDK provides a centralized interface between the application layer and other radio stacks.  This allows each feature to evolve independently.

License : [![License](https://img.shields.io/badge/license-BSD_3-blue.svg)](http://gitlab.northernmechatronics.com:50250/nmi/software/nmsdk/blob/master/LICENSE)

Platform Details: [![Hardware](https://img.shields.io/badge/hardware-wiki-green.svg)](https://www.northernmechatronics.com/nm180100)

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

## Build Instructions
1.  Clone the [NMSDK]().
2.  Download and install [AmbiqSuite](https://ambiq.com/apollo3-blue-low-power-mcu-family/#documents).
3.  Clone [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS).  Make sure to clone with the `--recurse-submodules` argument.
4.  If the bundled BLE stack from Ambiq is not used, then clone [Cordio](https://github.com/packetcraft-inc/stacks).  This will require additional porting effort which is currently not supported.  However, this enables BLE mesh networking on the NM180100 which is not available with the bundled stack from Ambiq.
5.  Define the following variables: NM_SDK, AMBIQ_SDK, FREERTOS, CORDIO
6.  Type `make` for release and `make DEBUG=1` for debug.
