<img src="https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/icon.png" width="100">

# I Repository Introduction
This repository contains the entire production-ready design (hardware, software, user interface, 3D-printable housing, assembly instructions) of the open-source TickTag GPS logger.

# II Hardware Production
* See sub folder [TickTagHardware](TickTagHardware)
* Schematics (.sch) and boards (.brd) are designed in Autodesk Eagle 9.5.2
* PCBs were produced and assembled by [PCBWay](https://www.pcbway.com) (production-ready Gerber files and PCBWay settings in [TickTagHardware/GerberProductionFiles](TickTagHardware/GerberProductionFiles))
* Production settings:
   * **REV3**: 24.9 x 10.5 mm, 2 layers, 0.15 mm thickness (flex), 0.35 mm min hole size, immersion gold (ENIG) surface finish (1U"), 0.06 mm min track spacing, 1 oz Cu finished copper, polyimide flex material

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3.jpg?raw=true)

   * **REV4**: 23.61 x 10.06 mm, 2 layers, 0.2 mm thickness, 0.25 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 5/5 mil min track spacing, 1 oz Cu finished copper, FR-4 TG150 material

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV41.png?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV42.png?raw=true)

   * **User interface board**: 66.4 x 17.8 mm, 2 layers, 1 mm thickness, 0.3 mm min hole size, immersion gold (ENIG) surface finish (1U"), tenting vias, 6/6 mil min track spacing, 1 oz Cu finished copper, FR-4 TG130 material
* Differences between REV3 and REV4:
   * 0.2 mm PCB instead of 0.15 mm (more robust)
   * PCB slots instead of pads for soldering LiPos directly to the PCB
   * New load switch (SiP32431DNP3-T1GE4), as the old load switch has long lead times
   * Reduced the footprint sizes of some of the capacitors and resistors from 0402 to 0201

# III Programming and Assembly

## IDE for Software Development (Windows)
* Atmel Studio 7.0: https://www.microchip.com/en-us/tools-resources/develop/microchip-studio
* Programming language: C / C++

## Android App
* Developed in Android Studio 3.5.1: https://developer.android.com/studio

## Using an Arduino Nano for Flashing
* Follow the instructions on https://github.com/ElTangas/jtag2updi
* The 4.7k resistor is already integrated on the user interface board and does not need to be added
* Alternatively software can be flashed with the official Atmel-ICE: https://www.microchip.com/en-us/development-tool/ATATMEL-ICE

## Flashing the Firmware
Flashing should be done before soldering a battery to the tag.

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/UIBSettings3.png?raw=true)

1. Check if the yellow jumper (E) connects "1" and "2" ("ldo")
2. Gently click the tag on the user interface board (without battery attached to it) (A), mind the correct orientation of the tag
3. Connect D6 of the Arduino Nano to the UPDI pin of the user interface board (or use an Atmel-ICE instead of an Arduino Nano)
4. Connect the user interface board to a computer (red LED turns on) (G), connect the Arduino Nano to the same computer
5. Slide the UPDI button on the user interface board (H) to ON
6. Go to [TickTagProgramming/avrdude](TickTagProgramming/avrdude), open ScriptWriteFuse.bat with a text editor and enter the COM port of the Arduino Nano on your computer
7. Execute ScriptWriteFuse.bat to write the configuration fuses of the ATTINY
8. Open Atmel Studio 7.0 and load the project [TickTagSoftwareBurst](TickTagSoftwareBurst)
9. Configure the ATTINY programming via Arduino Nano under Tools -> External Tools (Arguments: "-P COM7 -c jtag2updi -p t1626 -U flash:w:$(ProjectDir)Debug\$(TargetName).hex:i"):

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/ExternalTools.PNG?raw=true)

10. Press F7 to compile the firmware
11. Press Tools -> jtag2updi ATtiny1626 to flash the firmware

## Tag Assembly

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3soldered.jpg?raw=true)

* If the firmware is successfully flashed onto the microcontroller you can solder a battery to the TickTag battery terminals (plus and minus is written on the tag)
* No battery protection circuit is needed, the battery is protected by software
* You can use a glass fiber pen to roughen the LiPo tabs, which makes soldering of the aluminium tabs more easy (e.g., with a Laeufer 69119 pen)
* Keep the soldering work on the LiPo very short, otherwise the battery might be damaged due to high temperatures
* Keep the area below the TickTag as flat as possible, otherwise you might not be able to click the tag on the user interface board anymore

# IV Tag Manual

## Charging the Battery
**WARNING**: never connect the TickTag to the user interface board when the 3-pin jumper is set to "ldo", otherwise the LiPo will be damaged permanently. The battery can be recharged directly on the breakout board:

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/uib2.jpg?raw=true)

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/UIBSettings1.png?raw=true)

1. Check if the yellow jumper connects (E) "3" and "2" ("lipo")
2. Gently click the tag on the user interface board (with battery attached to it) (A), mind the correct orientation of the tag
3. Connect the USB connector to a computer or powerbank (red LED turns on) (G)
4. Slide the charge button on the user interface board (F) to the left (ON)
5. A green LED on the user interface board turns on and indicates that the battery is being charged
6. In case the tag was previously activated, it first needs to be deactivated:
   * Wait some minutes until battery is charged a bit
   * Press the white button (D) for 5 seconds to restart the tag and enter menue
   * Green LED on the tag will blink 5 times to indicate download mode and system restart
7. Wait until the green LED on the user interface board turns off (battery charged)
   * This can take hours (default charge current: 15 mA)
   * For example: an empty 30 mAh lipo battery needs 2 hours to be fully charged
   * For example an empty 120 mAh lipo battery needs 8 hours to be fully charged
8. The tag can be activated again (see chapter "Activation")
   * Warning: If the memory is full the tag can not be reactivated (data download necessary, see chapter "Data Download, Configuration and Memory Reset")

## Activation
* Prerequisites
   * The tags need to be connected to a charged lithium polymer battery (plus and minus pads on the tag are soldered to the battery)
   * If the battery voltage is too low, the tag won’t start (please charge the battery), see chapter "Charge Battery"
   * If the data memory is full, the tag won't start (please download the data and reset the memory), see chapter "Data Download, Configuration and Memory Reset"
   * Check the location of the green LED on the tag (it will give you visual feedback):

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3led.jpg?raw=true)

### Activation option 1: by a wire

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3activation1.jpg?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/Wire.PNG?raw=true)

1. Gently touch with one end of a conducting wire the ground connection where the battery minus is soldered to
2. Gently touch with the other end of the wire the hole marked with "A" (or "ST")
3. Once the tag starts blinking green, remove the wire immediately (should not take longer than 2 seconds)
4. The tag blinks 7 times to indicate that it’s activated, then waits for 700 ms and is blinking again to indicate battery status (1 time = battery low, 7 times = battery is full)
5. The green LED is located on the back side of the tag, so it's best to hold the breakout board sideways to see the blinking LED under the tag
6. If it doesn't blink at all: battery voltage might be low or the tag is already activated
7. If the tag blinks 5 times it entered download mode (was already activated), please wait a minute and start again from the beginning
8. The tag is activated and will start sampling GPS data after 10 seconds (default configuration, can be changed, see chapter "Data Download, Configuration and Memory Reset")

### Activation option 2: on breakout board

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3click.jpg?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/UIBSettings1.png?raw=true)

1. Locate the click connector on the tag (see picture above)
2. Take a look at the breakout board, do not connect the USB power connector (G) to the computer or phone (no external power needed for activation)
3. Gently click the tag on the user interface board (with battery attached to it) (A), mind the correct orientation of the tag
      * Take care of the correct orientation of the tag as shown in the photo, otherwise a short circuit might permanently damage the tag
4. Now press the white button (D) for two seconds until the tag starts to blink
      * Do not press the button longer than some seconds
      * The tag blinks 7 times to indicate that it’s activated, then waits for 700 ms and is blinking again to indicate battery status (1 time = battery low, 7 times = battery is full)
      * The green LED is located on the back side of the tag, so it's best to hold the breakout board sideways to see the blinking LED under the tag
      * If it doesn't blink at all: battery voltage might be low or the tag is already activated
      * If the tag blinks 5 times it entered download mode (was already activated), please wait a minute and start again from the beginning
5. The tag is activated and will start sampling GPS data after 10 seconds (default configuration, can be changed, see chapter "Data Download, Configuration and Memory Reset")

## After the Activation (Data Sampling)
* After the activation delay, the tag starts blinking green every second and tries to get the current time via GPS satellites. If it can’t obtain the time within 120 seconds, it will sleep for 15 minutes and will try again.
   * **IMPORTANT**: If blinking configuration is set to false the tag will completely stop blinking at all after getting the current UTC time
   * If you have configured the tag to only sample GPS at certain times:
       * **IMPORTANT**: Before attaching the tag to the animal (e.g., the day before): go outside, activate the tag, wait until it starts blinking, wait until the green LED stays on for 2 seconds (tag got the time and went to sleep mode).
* The tag samples GPS data (continuously or from time to time, depending on the configuration)
* The module deactivates itself if the battery voltage is low (default configuration: 3.3V) or if the memory is full (maximum 13,100 fixes)

## On-Animal Mounting
* **IMPORTANT**: The small antenna of the TickTag is disturbed by any electrically conducting material nearby, so keep the antenna away from other tags, VHF/UHF tags, batteries, screws or similar materials (at least 5 cm).
* **IMPORTANT**: The antenna can easily break if bent too strong. Try to protect the antenna with a 3D-printed housing (for example with ASA material).
* **IMPORTANT**: The battery needs to be located behind the tag (see picture below), never under the tag, as it will disturb the antenna.
* **IMPORTANT**: Mount the tag on the flat side (connector facing down, top side with antenna facing the sky). The photo shows the top side that should face the sky:

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/mounting.PNG?raw=true)

* **IMPORTANT**: Do not glue anything on the connector that can't be removed, otherwise data cannot be downloaded (Bostik Blu Tack works fine):

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/REV3click.jpg?raw=true)

## Data Download, Configuration and Memory Reset

![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/UIBSettings1.png?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/termite.PNG?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/moba.PNG?raw=true)
![Image](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/androidapp.PNG?raw=true)

1. Choose a serial software
   * **Option 1**: Download a serial program for your computer
       * For example (Windows 10): Termite: https://www.compuphase.com/software_termite.htm
       * For example (Windows 10): MobaXTerm: https://mobaxterm.mobatek.net/
       * The Arduino Serial Monitor also works fine
   * **Option 2**: Use the TickTag Android app [TickTagAndroidApp](TickTagAndroidApp) and an USB OTG adapter to connect the breakout board with the phone
2. The battery needs to be connected (soldered) to the tag
3. Check if the yellow jumper connects (E) "3" and "2" ("lipo")
4. Gently click the tag on the user interface board (with battery attached to it) (A), mind the correct orientation of the tag
5. Connect the USB connector to a computer or Android phone (red LED turns on) (G)
6. Slide the charge button on the user interface board (F) to the left (ON) to charge the battery while downloading data (not mandatory, but recommended)
7. **For option 1**: Open the serial program on your computer with following settings:
   * COM port: select the CP2104N COM port from the list
   * Baud rate: 9600
   * Default serial settings (8 data bits, 1 stop bit, no parity)
8. Click on CONNECT in your serial program or the Android app
9. Press the download button (D) for 5 seconds (not longer) until the green LED on the tag blinks for 5 times
10. Data download is shown in your serial program (in CSV format)
11. After the download the tag shows a little menu for configuration
       * Data download might take some minutes (if memory is full)
       * Data download can be interrupted by pressing the download button once (short), data will not be deleted
12. In the command line of your serial program enter "1" and press return to reset the memory (when using the Android app: press the "Reset memory" button)
13. The tag will confirm that the memory is now empty
14. The tag will restart into the activation state after some time (waiting on tag activation, see chapter "Activation")

Example data output of the serial program (or Android app):

```
---TICK-TAG---
*START MEMORY*
UVs: 0, TOs: 0/0, ErrorsOrGF: 0, TTFF: 0
Fixes: 10, Avg. TTF: 11 s, Avg. HDOP (x10): 16
count,timestamp,lat,lon
1,1635235981,47.74340,8.99910
2,1635236130,47.74346,8.99922
3,1635236352,47.74340,8.99889
4,1635236506,47.74327,8.99894
5,1635236682,47.74336,8.99879
6,1635238092,47.74340,8.99824
7,1635238313,47.74304,8.99896
8,1635238411,47.74329,8.99883
9,1635238511,47.74332,8.99878
10,1635238610,47.74325,8.99881
*END MEMORY*
V201, ID: 30-2509, 3533mV, 01b
SETTINGS:
- Min voltage: 3300 mV
- Frequency: 90 s
- Min HDOP (x10): 30
- Activation delay: 60 s
- Geofencing: 0
- Burst duration: 0 s
- Time: 8:00 - 14:00 UTC
-------------------
0 Read memory
1 Reset memory
2 Set min. voltage
3 Set frequency
4 Set accuracy
5 Set activation delay
6 Set times
7 Toggle geofencing ON/OFF
8 Set burst duration
9 Exit
-------------------
```

## Configuration Parameters
* **Read memory**: printing all stored GPS fixes as CSV-compatible list (count, timestamp in UTC, latitude, longitude)
* **Reset memory**: deleting all stored GPS fixes
* **Min. voltage (3000 - 4250, in mV)**: recording is stopped when battery voltage (open-circuit, no load) drops below that threshold
* **Frequency (1 - 16382, in s)**: GPS fix attempt frequency, between 1 and 5 s the TickTag keeps the GPS module constantly powered and puts the device in fitness low power mode
* **Accuracy (1 - 250, HDOP x 10)**: HDOP value (times 10, 30 = 3.0) that tries to be achieved within 9 seconds after getting the first positional estimate
* **Activation delay (10 - 16382, in s)**: delay after activation before the tag starts recording data
* **Sampling times (0000 - 2359, time within day)**: daily recording time window
* **Geo-fencing (true/false)**: if set to true the first GPS fix becomes the home location and only fixes outside a 300 m radius are stored (10 min hibernation afterwards)
* **Blinking (true/false)**: if set to true the tag blinks every second when GPS is active

## State Machine
![StateMachine](https://github.com/trichl/TickTagOpenSource/blob/main/TickTagImages/StateMachineTickTagV3.png?raw=true)

# V Data Compression Algorithm
GPS data is stored on the 128 kByte EEPROM with a lossless compression algorithm. GPS positions are stored with 5 decimal places (accuracy: 1.11 m).

* **Latitute (A)**: stored in 25 Bit (unsigned): compressedLatitude = (latitude * 100000) + 9000000
* **Longitude (B)**: stored in 26 Bit (unsigned): compressedLongitude = (longitude * 100000) + 18000000
* **Timestamp (C)**: stored in 29 Bit  (unsigned): compressedTimestamp = timestamp - 1618428394
* Storage pattern of Bits: **AAAAAAAA AAAAAAAA AAAAAAAA ABBBBBBB BBBBBBBB BBBBBBBB BBBCCCCC CCCCCCCC CCCCCCCC CCCCCCCC**
* Total length: 10 Byte per GPS fix
