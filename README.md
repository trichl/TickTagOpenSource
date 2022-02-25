# TickTagOpenSource
 
This repository contains the entire production-ready design (hardware, software, user interface, 3D-printable housing, assembly instructions) of the open-source TickTag GPS logger.

# Hardware Layouts
* See sub folder [TickTagHardware](TickTagHardware)
* Schematics (.sch) and boards (.brd) are designed in Autodesk Eagle 9.5.2
* PCBs were produced and assembled by [PCBWay](https://www.pcbway.com) (production-ready Gerber files and PCBWay settings in [TickTagHardware/GerberProductionFiles](TickTagHardware/GerberProductionFiles))
* Production settings:
   * REV3: 24.9 x 10.5 mm, 2 layers, 0.15 mm thickness (flex), 0.35 mm min hole size, immersion gold (ENIG) surface finish (1U"), 0.06 mm min track spacing, 1 oz Cu finished copper, polyimide flex material
   * REV4: 23.61 x 10.06 mm, 2 layers, 0.2 mm thickness, 0.25 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 5/5 mil min track spacing, 1 oz Cu finished copper, FR-4 TG150 material

# Software development

# Programming

# Configuration
 
# Data compression algorithm

GPS data is stored on the 128 kByte EEPROM with a lossless compression algorithm. GPS positions are stored with 5 decimal places (accuracy: 1.11 m).

* Latitute (A): stored in 25 Bit (unsigned): compressedLatitude = (latitude * 100000) + 9000000
* Longitude (B): stored in 26 Bit (unsigned): compressedLongitude = (longitude * 100000) + 18000000
* Timestamp (C): stored in 29 Bit  (unsigned): compressedTimestamp = timestamp - 1618428394
* Storage pattern of Bits: AAAAAAAA AAAAAAAA AAAAAAAA ABBBBBBB BBBBBBBB BBBBBBBB BBBCCCCC CCCCCCCC CCCCCCCC CCCCCCCC
* Total length: 10 Byte per GPS fix
