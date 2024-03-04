@ECHO OFF
ECHO Hello! This script configures the fuses of the TickTag and flashes the firmware. Please connect Arduino Nano with jtag2updi to the programming board (PIN: UPDI).
mode
set /p COMNUM="Enter COM Port: "
cd bin
:loopie
avrdude -c jtag2updi -P com%COMNUM% -p t1626 -C ..\etc\avrdude.conf -U fuse1:w:0b00011000:m
avrdude -c jtag2updi -P com%COMNUM% -p t1626 -C ..\etc\avrdude.conf -U fuse2:w:0b00000001:m
avrdude -c jtag2updi -P com%COMNUM% -p t1626 -C ..\etc\avrdude.conf -U fuse5:w:0b11110111:m
avrdude -c jtag2updi -P com%COMNUM% -p t1626 -C ..\etc\avrdude.conf -U fuse6:w:0b00000100:m
avrdude -c jtag2updi -P com%COMNUM% -p t1626 -C ..\etc\avrdude.conf -U flash:w:..\..\FinalFirmwareBinaries\TickTagSoftwareBurst_V203_ATTINY1626_BonnetedBats\TickTagSoftwareBurst.hex:i
ECHO Press any key to program again!
PAUSE
goto loopie