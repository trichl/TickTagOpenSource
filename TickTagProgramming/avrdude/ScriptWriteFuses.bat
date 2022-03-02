@ECHO OFF
ECHO Hello! This script configures the fuses of the TickTag. Please connect Arduino Nano with jtag2updi to the programming board (PIN: UPDI).
TIMEOUT 5
cd bin
avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse1:w:0b00011000:m
avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse2:w:0b00000001:m
avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse5:w:0b11110111:m
avrdude -c jtag2updi -P com7 -p t1626 -C ..\etc\avrdude.conf -U fuse6:w:0b00000100:m
PAUSE