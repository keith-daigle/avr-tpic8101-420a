# avr-tpic8101-420a
TPIC 8108 based knock monitor for AVR using Chrysler 420a trigger wheel

This code is about 5 years old (as of July 2015) but I was asked to post it.
It's mostly functional, however it's got a fatal bug in the calculation of RPM.  When the RPMS get high
the tick counts get small and there is a great loss of resolution.  I had plans on rewriting it for a SMT32F4 based uC
but never got around to it.  The TPIC 8101 interface is pretty fully fleshed out and the menu is quite functional.

I'd love to release a working version for avr or stm32 (or other arm uC), so pull requests are most welcome.

It was written under avrstudio but between the upload.bat and the Makefile it should be able to be built and uploaded
to any bare 328P AVR.
