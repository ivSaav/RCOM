# RCOM - 1st PROJECT
## 3MIEIC05 - G3

## setup serial port connection
socat -d -d pty,raw,echo=0 pty,raw,echo=0

## application - filename, serialport
#### application transmitter
./app  /dev/pts/0 commands.txt

#### application receiver
./app /dev/pts/1

