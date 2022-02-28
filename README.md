# TickTagOpenSource
 
This repository contains the entire production-ready design (hardware, software, user interface, 3D-printable housing, assembly instructions) of the open-source TickTag GPS logger.

# Hardware Production
* See sub folder [TickTagHardware](TickTagHardware)
* Schematics (.sch) and boards (.brd) are designed in Autodesk Eagle 9.5.2
* PCBs were produced and assembled by [PCBWay](https://www.pcbway.com) (production-ready Gerber files and PCBWay settings in [TickTagHardware/GerberProductionFiles](TickTagHardware/GerberProductionFiles))
* Production settings:
   * REV3: 24.9 x 10.5 mm, 2 layers, 0.15 mm thickness (flex), 0.35 mm min hole size, immersion gold (ENIG) surface finish (1U"), 0.06 mm min track spacing, 1 oz Cu finished copper, polyimide flex material
   * REV4: 23.61 x 10.06 mm, 2 layers, 0.2 mm thickness, 0.25 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 5/5 mil min track spacing, 1 oz Cu finished copper, FR-4 TG150 material
   * User interface board: 66.4 x 17.8 mm, 2 layers, 1 mm thickness, 0.3 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 6/6 mil min track spacing, 1 oz Cu finished copper, FR-4 TG130 material
* Differences between REV3 and REV4:
   * 0.2 mm PCB instead of 0.15 mm (more robust)
   * PCB slots instead of pads for soldering LiPos
   * New load switch (SiP32431DNP3-T1GE4), as the old load switch has long lead times
   * Reduced the footprints of some of the capacitors and resistors from 0402 to 0201

# IDE for Software Development (Windows)
* Atmel Studio 7.0: https://www.microchip.com/en-us/tools-resources/develop/microchip-studio

# Using an Arduino Nano for Flashing
* Follow the instructions on https://github.com/ElTangas/jtag2updi
* The 4.7k resistor is already integrated on the user interface board and does not need to be added
* Alternatively software can be flashed with the official Atmel-ICE: https://www.microchip.com/en-us/development-tool/ATATMEL-ICE

# Flashing the Firmware
* Flashing should be done before soldering a battery to the tag
* Gently click the tag on the user interface board (with battery attached to it), mind the correct orientation of the tag
* Double check the jumper locations on the user interface board !!!PHOTO!!!
* Connect D6 of the Arduino Nano to the UPDI pin of the user interface board (or use an Atmel-ICE instead of an Arduino Nano)
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
* You can use a glass fiber pen to roughen the LiPo tabs, which makes soldering of the aluminium tabs more easy (e.g., with a Laeufer 69119 pen)
* Keep the soldering work on the LiPo very short, otherwise the battery might be damaged due to high temperatures
* Keep the area below the TickTag as flat as possible, otherwise you might not be able to click the tag on the user interface board anymore

# Charging the Battery
* WARNING: never connect the TickTag to the user interface board when the 3-pin jumper is set to "ldo", otherwise the LiPo will be damaged permanently
* The battery can be recharged directly on the breakout board: !!!PHOTO!!!
* Check if the yellow jumper connects "3" and "2" ("lipo") like shown in the photo above
* Gently click the tag on the user interface board (with battery attached to it), mind the correct orientation of the tag
* Connect the USB connector to a computer or power source (red LED on breakout board turns on)
* Turn the charge slide button (red rectangle in photo above) to the left
* A green LED on the breakout board turns on and indicates that the battery is being charged
* In case the tag was previously activated, it first needs to be deactivated:
   * Wait some minutes until battery is charged a bit
   * Press the white button for 5 seconds to restart the tag and enter menue
   * Green LED on the tag will blink 5 times to indicate download mode and system restart
* Wait until the green LED turns off (battery charged)
   * This can take hours (default charge current: 15 mA)
   * For example: an empty 30 mAh lipo battery needs 2 hours to be fully charged
   * For example an empty 120 mAh lipo battery needs 8 hours to be fully charged
* The tag can be activated again (see chapter "Activation")
* Warning: If the memory is full the tag can not be reactivated (data download necessary, see next chapter)

# Activation
* Prerequisites
   * The tags need to be connected to a charged lithium polymer battery (plus and minus pads on the tag are soldered to the battery)
   * If the battery voltage is too low, the tag won’t start (please charge the battery), see chapter "Charge Battery"
   * If the data memory is full, the tag won't start (please download the data and reset the memory), see chapter "Data Download and Memory Reset"
   * No battery protection circuit is needed, the battery is protected by software
   * Check the location of the green LED on the tag (it will give you feedback): !!!PHOTO!!!
* Activation option 1: by a wire: !!!PHOTO!!!
   * Gently touch with one end of a conducting wire the ground connection where the battery minus is soldered to (red circle on the left)
   * Gently touch with the other end of the wire the hole marked with "A" (or "ST") (red circle on the right)
   * Once the tag starts blinking green, remove the wire immediately (should not take longer than 2 seconds)
   * The tag blinks 7 times to indicate that it’s activated, then waits for 700 ms and is blinking again to indicate battery status (1 time = battery low, 7 times = battery is full)
   * If it doesn’t blink at all: battery voltage might be low or the tag is already activated
   * If the tag blinks 5 times it entered download mode (was already activated), please wait a minute and start again from 1
   * The tag is activated and will start sampling GPS data after 10 seconds (default configuration, can be changed, see chapter "Configuration")
* Activation option 2: on breakout board: !!!PHOTO!!!
   * Locate the click connector on the tag (red circle): !!!PHOTO!!!
   * Take a look at the breakout board, do not connect the USB power connector (no external power needed for activation): !!!PHOTO!!!
   * Gently and carefully click the tag on the breakout board where you see the connector counterpart (red tag outline in the photo above)
      * Take care of the correct orientation of the tag as shown in the photo, otherwise a short circuit might permanently damage the tag
   * Now press the white button (red circle in photo above) for 2 seconds until the tag starts to blink
      * Do not press the button longer than some seconds
      * The tag blinks 7 times to indicate that it’s activated, then waits for 700 ms and is blinking again to indicate battery status (1 time = battery low, 7 times = battery is full)
      * The green LED is located on the back side of the tag, so it’s best to hold the breakout board sideways to see the blinking LED under the tag
      * If the tag blinks 5 times it entered download mode (was already activated), please wait a minute and start again from 1
   * The tag is activated and will start sampling GPS data after 30 seconds (default configuration, can be changed)

# After the Activation (Data Sampling)
* After the activation delay, the tag starts blinking green every second and tries to get the current time via GPS satellites. If it can’t obtain the time within 120 seconds, it will sleep for 15 minutes and will try again.
   * IMPORTANT: If blinking configuration is set to false the tag will completely stop blinking at all after getting the current UTC time
   * If you have configured the tag to only sample GPS at certain times:
       * IMPORTANT: Before attaching the tag to the animal (e.g., the day before): go outside, activate the tag, wait until it starts blinking, wait until the green LED stays on for 2 seconds (tag got the time and went to sleep mode).
* The tag samples GPS data (continuously or from time to time, depending on the configuration)
* The module deactivates itself if the battery voltage is low (default configuration: 3.3V) or if the memory is full (maximum 13,100 fixes)

# Mounting the Tag on an Animal
* IMPORTANT: The small antenna of the TickTag is disturbed by any electrically conducting material nearby, so keep the antenna away from other tags, VHF/UHF tags, batteries, screws or similar materials (at least 5 cm).
* IMPORTANT: The antenna can easily break if bent too strong. Try to protect the antenna with a 3D-printed housing (for example with ASA material).
* IMPORTANT: The battery needs to be located behind the tag (see picture below), never under the tag, as it will disturb the antenna.
* IMPORTANT: Mount the tag on the flat side (connector facing down, top side with antenna facing the sky). The photo shows the top side that should face the sky: !!!PHOTO!!!
* IMPORTANT: Do not glue anything on the connector that can't be removed, otherwise data cannot be downloaded (blu-tack works fine): !!!PHOTO!!!

# Data Download, Configuration and Memory Reset
1. Choose a serial software
* Option 1: Download a serial program for your computer
i. For example (Windows 10): Termite
https://www.compuphase.com/software_termite.htm
ii. For example (Windows 10): MobaXTerm
https://mobaxterm.mobatek.net/
iii. The Arduino Serial Monitor also works fine
b. Option 2: Use the TickTag Android app and an USB OTG adapter to connect
the breakout board with the phone
2. The battery needs to be connected to the tag
3. Check if the jumper settings are like in the picture above (B, C, E)
4. Gently and very carefully click the tag on the breakout board (A)
5. Connect the USB connector (G) to a computer or Android phone (red LED on
breakout board turns on)
6. Turn the charge slide button (F) to the left to charge the battery while downloading
data (not mandatory, but recommended)
7. For Option 1: Open the serial program on your computer with following settings
a. COM port: select the CP2104N COM port from the list
b. Baud rate: 9600
c. Default serial settings (8 data bits, 1 stop bit, no parity)

# Configuration Parameters
* Read memory: printing all stored GPS fixes as CSV-compatible list (timestamp in UTC, latitude, longitude)
* Reset memory: all stored GPS fixes are deleted
* Shutdown voltage (3000 - 4250, in mV): Open-circuit voltage (no load) when the TickTag shall stop recording GPS data
* Frequency (1 - 16382, in s): GPS fix attempt frequency, between 1 and 5 s the TickTag keeps the GPS module constantly powered and puts the device in fitness low power mode
* Accuracy (1 - 255, HDOP x 10): HDOP value that tries to be achieved within 9 seconds after getting the first positional estimate
* Activation delay (10 - 16382, in s): Delay after activation before the tag starts recording data
* On time (0000 - 2359, in min within day): Daily recording time window
* Geo-fencing (true/false): When activated the first GPS fix becomes the home location and only fixes outside a 300 m radius are stored (10 min hibernation afterwards)
* Blinking (true/false): When activated the tag blinks every second when GPS is active
 
# Android App

# State Machine

# Data Compression Algorithm

GPS data is stored on the 128 kByte EEPROM with a lossless compression algorithm. GPS positions are stored with 5 decimal places (accuracy: 1.11 m).

* Latitute (A): stored in 25 Bit (unsigned): compressedLatitude = (latitude * 100000) + 9000000
* Longitude (B): stored in 26 Bit (unsigned): compressedLongitude = (longitude * 100000) + 18000000
* Timestamp (C): stored in 29 Bit  (unsigned): compressedTimestamp = timestamp - 1618428394
* Storage pattern of Bits: AAAAAAAA AAAAAAAA AAAAAAAA ABBBBBBB BBBBBBBB BBBBBBBB BBBCCCCC CCCCCCCC CCCCCCCC CCCCCCCC
* Total length: 10 Byte per GPS fix
