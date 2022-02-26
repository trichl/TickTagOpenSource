# TickTagOpenSource
 
This repository contains the entire production-ready design (hardware, software, user interface, 3D-printable housing, assembly instructions) of the open-source TickTag GPS logger.

# Hardware Layouts
* See sub folder [TickTagHardware](TickTagHardware)
* Schematics (.sch) and boards (.brd) are designed in Autodesk Eagle 9.5.2
* PCBs were produced and assembled by [PCBWay](https://www.pcbway.com) (production-ready Gerber files and PCBWay settings in [TickTagHardware/GerberProductionFiles](TickTagHardware/GerberProductionFiles))
* Production settings:
   * REV3: 24.9 x 10.5 mm, 2 layers, 0.15 mm thickness (flex), 0.35 mm min hole size, immersion gold (ENIG) surface finish (1U"), 0.06 mm min track spacing, 1 oz Cu finished copper, polyimide flex material
   * REV4: 23.61 x 10.06 mm, 2 layers, 0.2 mm thickness, 0.25 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 5/5 mil min track spacing, 1 oz Cu finished copper, FR-4 TG150 material

# IDE for Software Development (Windows)
* Atmel Studio 7.0: https://www.microchip.com/en-us/tools-resources/develop/microchip-studio

# Using an Arduino Nano for Flashing
* Follow the instructions on https://github.com/ElTangas/jtag2updi
* The 4.7k resistor is already integrated on the user interface board and does not need to be added
* Alternatively software can be flashed with the official Atmel-ICE: https://www.microchip.com/en-us/development-tool/ATATMEL-ICE

# Flashing the Firmware
* Connect the TickTag WITHOUT battery to the user interface board
* Double check the jumper locations on the user interface board !!!PHOTO!!!
* Connect D6 of the Arduino Nano to the UPDI pin of the user interface board (or use an Atmel-ICE instead of Arduino Nano)
* Connect the user interface board to a computer, connect the Arduino Nano to the same computer
* Slide the UPDI button on the user interface board to ON
* Go to [TickTagProgramming/avrdude](TickTagProgramming/avrdude), open ScriptWriteFuse.bat with a text editor and enter the COM port of the Arduino Nano on your computer
* Execute ScriptWriteFuse.bat to write the configuration fuses of the ATTINY
* Open Atmel Studio 7.0 and load the project [TickTagSoftware](TickTagSoftware) for the regular firmware or [TickTagSoftwareBurst](TickTagSoftwareBurst) for the firmware capable of burst-recording GPS fixes
* Configure the ATTINY programming via Arduino Nano under Tools -> External Tools: !!!PHOTO!!!
* Press F7 to compile the firmware
* Press Tools -> jtag2updi ATtiny1626 to flash the firmware

# Tag Assembly
* If the firmware is successfully flashed onto the microcontroller you can solder a battery to the TickTag battery terminals (plus and minus is written on the tag)
* You can use a glass fiber pen to roughen the LiPo pads, which makes soldering more easy (e.g., Laeufer 69119)
* Keep the soldering work on the LiPo very short, otherwise the battery might be damaged
* Keep the area below the TickTag as flat as possible, otherwise you might not be able to click the tag on the user interface board anymore

# Charge battery
* WARNING: never connect the TickTag to the user interface board when the 3-pin jumper is set to "ldo", otherwise the LiPo will be damaged permanently
* The battery can be recharged directly on the breakout board: !!!PHOTO!!!
* Check if the yellow jumper connects "3" and "2" ("lipo") like shown in the photo above
* Gently click the tag on the breakout board (with battery attached to it)
* Connect the USB connector to a computer or power source (red LED on breakout board turns on)
* Turn the charge slide button (red rectangle in photo above) to the left
* A green LED on the breakout board turns on and indicates that the battery is being charged
* In case the tag was activated
   * Wait some minutes until battery is charged a bit
   * Press the white button for 5 seconds to restart the tag
   * Green LED on the tag will blink 5 times to indicate download mode and system restart
   * Wait until the green LED turns off (battery charged)
   * This can take hours (charge current: 15mA)
   * For example: an empty 30 mAh lipo battery needs 2 hours to be fully charged
   * For example an empty 120 mAh lipo battery needs 8 hours to be fully charged
   * The tag can be activated again (see chapter 2 and 3)

# Activation

# Configuration

# Configuration Parameters

# Data Download
 
# Data Compression Algorithm

GPS data is stored on the 128 kByte EEPROM with a lossless compression algorithm. GPS positions are stored with 5 decimal places (accuracy: 1.11 m).

* Latitute (A): stored in 25 Bit (unsigned): compressedLatitude = (latitude * 100000) + 9000000
* Longitude (B): stored in 26 Bit (unsigned): compressedLongitude = (longitude * 100000) + 18000000
* Timestamp (C): stored in 29 Bit  (unsigned): compressedTimestamp = timestamp - 1618428394
* Storage pattern of Bits: AAAAAAAA AAAAAAAA AAAAAAAA ABBBBBBB BBBBBBBB BBBBBBBB BBBCCCCC CCCCCCCC CCCCCCCC CCCCCCCC
* Total length: 10 Byte per GPS fix
