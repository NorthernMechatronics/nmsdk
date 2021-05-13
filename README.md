# Northern Mechatronics Software Development Kit
License : [![License](https://img.shields.io/badge/license-BSD_3-blue.svg)](http://gitlab.northernmechatronics.com:50250/nmi/software/nmsdk/blob/master/LICENSE)

Platform Details: [![Hardware](https://img.shields.io/badge/hardware-wiki-green.svg)](https://www.northernmechatronics.com/nm180100)

## Attention
This is the master branch.  Please checkout the LoRaWAN_v1.0.3 brnach for the LoRaWAN L2 v1.0.3 specifications.  The corresponding
release tag for the LoRaMac-node is v4.4.7 (final release).  Please make sure that you switch to the proper release tag before building this SDK.

## Overview
The NMSDK is a platform library for the Northern Mechatronics NM180100 LoRa BLE module.
It provides support for LoRa direct, LoRa real-time, LoRaWAN, and BLE wireless connectivity as well
as a FreeRTOS framework for rapid application development across a wide range of use cases and environments.
As of May 2021, NVM context management (a mandatory feature in v1.0.4 but supported in v1.0.3) is enabled by default.
Two flash pages located at the end of the flash memory are used for context storage.  If you wish to disable this feature,
remove the following define in ./makedefs/nm_loramac.mk   

```DEFINES += -DCONTEXT_MANAGEMENT_ENABLED```    

Architecturally speaking, the SDK consists of three layers
![architecture](doc/SDK_architecture.png)


## Supported Host Platforms

For more details on building the SDK on a host platform, please check the **User Guide** specified below for either the Windows OS or the Linux OS.

## Directory Structure
| Directory | Description |
| --------- | ----------- |
| bsp | Pre-defined board support packages |
| doc | Documentation |
| features | SDK bindings and build scripts for other software packages |
| platform | NM platform service code |

## Build Instructions
### Pre-requisites
* git
* make
* sed

### Installation
* Clone the [NMSDK](https://github.com/NorthernMechatronics/nmsdk).

```git clone https://github.com/NorthernMechatronics/nmsdk.git```

```git checkout LoRaWAN_v1.0.3```

* Download and install the [AmbiqSuite](https://ambiq.com/wp-content/uploads/2020/09/AmbiqSuite-R2.5.1.zip).

* Clone [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS).  Make sure to clone with the `--recurse-submodules` argument.

```git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules```  

* If the bundled BLE stack from Ambiq is not used, then clone [Cordio](https://github.com/packetcraft-inc/stacks).  This will require additional porting effort which is currently not supported.  However, this enables BLE mesh networking on the NM180100 which is not available with the bundled stack from Ambiq.

```git clone https://github.com/packetcraft-inc/stacks.git```

* Clone the [LoRaWAN](https://github.com/Lora-net/LoRaMac-node) stack

```git clone https://github.com/Lora-net/LoRaMac-node.git```

* Change to the project directory and switch to the final release of the LoRaWAN
L2 v1.0.3 tag

```cd <LoRaMac-node directory>```

```git checkout v4.4.7```

### Build Setup
* Open the Makefile
* Modify the following variables to point to the location where you have cloned or downloaded the various SDKs:
    - AMBIQ_SDK
    - FREERTOS
    - CORDIO
    - UECC
    - LORAMAC
* If all the SDKs are located within the same parent folder, there is no need to modify the variables as the build
script will automatically search for the correct paths.

### Build
* Type `make` for release and `make DEBUG=1` for debug.
* Note that in the release configuration, the Ambiq's HAL API validation and parameter checking are disabled.
