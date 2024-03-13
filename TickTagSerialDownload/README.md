# TickTag Serial Download Tool Documentation

This tool allows the user to *get the serial data from a TickTag device*, then *arrange the data in csv files*.
One folder is created for each day at which tags have been activated, in these folders two files are created per tag :
1. The **log files** which includes **all the serial output** from the tag
2. The **csv files** containing **GPS data** that have been automatically extracted from the serial data

The tool also supports memory resetting : You can choose at the end of the download process between keeping data on the tag on resetting the tag memory.

## Prerequisites

1. Using this assume you are using TickTag REV6 and the breakout board.
2. You have a Python version > 3.6 ( You can download this from [https://www.python.org/downloads/] )
3. You have all the python dependencies libraries installed : 
   To make sure, go to the Testing root folder and run ```pip install requirements.txt```
   
4. Tags ran and got **at least one fix**, otherwise script will not work.
   

## Usage

1. Make sur the jumpers on the breakout board all for Receiving, Sending and if a battery is attached, make sure the jumper is set to lipo, otherwise set it to ldo. Both *UPDI* and *charge* switches should be *OFF*.
2. Plug the board to an USB port on your computer and find the serial port associated.
3. Open the python script with any text editor and change line 11 : ```PORT="COM12"``` ( modify COM12 to port you found previously ).
4. Gently click the tag on the breakout board
5. Click the button on the breakout board for 2s ( no more than 3s ) and wait : 
   1. If the green LED blinks 7 times and then 1 to 7 times, it means the tag is now active
   2. If nothing blinks, it means the tag was probably already active, leave it this way
6. Launch the python script, for that you can run ```python TickTag_Serial_Download.py``` in the terminal.
7. You can than click the button on the breakout board for 6s until the green LED starts blinking.
8. A progress bar should prompt you with the data downloading status, and the script should ask you if you want to reset the memory => At this point all the data should already be saved in files but make sure to check.
9.  Data downloading is done. 
