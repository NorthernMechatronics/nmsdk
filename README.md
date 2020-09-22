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
### Pre-requisites
* git
* make

### Installation
* Clone the [NMSDK](http://gitlab.northernmechatronics.com:50250/nmi/software/nmsdk).

```git clone ssh://git@gitlab.northernmechatronics.com:50251/nmi/software/nmsdk.git```

* Download and install [AmbiqSuite](https://ambiq.com/apollo3-blue-low-power-mcu-family/#documents).

* Clone [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS).  Make sure to clone with the `--recurse-submodules` argument.

```git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules```  

* If the bundled BLE stack from Ambiq is not used, then clone [Cordio](https://github.com/packetcraft-inc/stacks).  This will require additional porting effort which is currently not supported.  However, this enables BLE mesh networking on the NM180100 which is not available with the bundled stack from Ambiq.

```git clone https://github.com/packetcraft-inc/stacks.git```

### Build
* Define the following variables: NM_SDK, AMBIQ_SDK, FREERTOS, CORDIO

* Type `make` for release and `make DEBUG=1` for debug.
