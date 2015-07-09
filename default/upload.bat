@echo off
set avrutils_base=C:\arduino-0018\arduino-0018\hardware\tools\avr
set comport=com5
set file_to_load=.\knock_watch.hex
set avrdude=%avrutils_base%\bin\avrdude.exe
set avrdude_cfg=%avrutils_base%\etc\avrdude.conf
set avrdude_args=-pm328p -cstk500v1 -b 57600 -P %comport% -D -F -Uflash:w:%file_to_load%
@echo on

%avrdude% -C %avrdude_cfg% %avrdude_args%
