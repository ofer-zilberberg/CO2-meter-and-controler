
# CO2 monitor & control

This project is based on the amassing work of Stephan Schulz that can be found on:
https://github.com/stephanschulz/co2-monitor
Please start by reading his work for better understanding.

I have added to his work:
1) Full menu to setup parameters.
2) More information and date added to IDLE screen.
3) option to activate a relay when CO2 levels above co2_bad parameter and DE-activate relay with hyst_co2 hysteresis value.
   Both can be setup by menu.
4) option to setup RTC date and time from menu.
5) Tons of explanations inside SW.
6) Warning before CO2 calibration.
7) Add correct file "creation date" into SD-card FAT system.
8) Auto check if RTC need initialize after startup
9) Define SW FLAGS to manage information printed on serial port.
10) Low battery voltage indication on IDLE screen.

additional work that can be done:
1) When SD-Card is full - delete OLD log-files to free space. (LIFO)
2) Store SETUP threshold parameters, updated by user, on flash for correct wakeup after power failure.
3) use roll menu to enable more options per menu. (more lines of menu although only 4 lines in display)
4) ADD relay operation per temperature threshold. (i.e. FAN will vent when temperature rise above threshold)
5) ADD relay operation per humidity threshold. (i.e. FAN will vent when humidity rise above threshold)
6) Menu option to chart last week CO2 graph on display.
7) Adding WiFi to remotely handle the data. (move to ESP8266 instead of M4 ?)
8) WIFI for "remote F/W upgrade" (using QSPI Flash as SW F/W bank ?)
9) SW upgrade from file on SD-CARD.
10) Ethernet connection on RJ45.
11) Add barometric presure sensor for automatic adjust CO2 sensor parameter.

Pins used:

Power:
GND - this is the common ground for all power and logic
BAT - this is the positive voltage to/from the JST jack for the optional Li-Poly battery
USB - this is the 5V positive voltage to/from the micro USB jack if connected
3V - this is the output from the 3.3V regulator, it can supply 500mA peak

Display:
SDA - the I2C (Wire) data pin. Also GPIO #21. 	\
SCL - the I2C (Wire) clock pin. Also GPIO #22.	|
GPIO #5 - KeyC = DOWN							|---> used for display
GPIO #6 - KeyB = ENTER							|
GPIO #9 - KeyA = UP								/

RTC:
SDA - The I2C (Wire) data pin. Also GPIO #21.	\____> Same I2C bus as for display.
SCL - The I2C (Wire) clock pin. Also GPIO #22.	/

SD-card:
SCK - SPI Clock (SCK) - output from feather to wing								\
MOSI - SPI Micro-controller Out Sensor In (MOSI) - output from feather to wing	|--> SPI bus
MISO - SPI Micro-controller In Sensor Out (MISO) - input from wing to feather	|
GPIO #10 - Chip Select.															/


GPIO #13 - Connected to the red LED next to the USB jack

GPIO #12 - Used for relay operation.

GPIO #8 - Used for neo-pixel.

SCD-30, CO2 meter:
SDA - the I2C (Wire) data pin. Also GPIO #21.	\___> Same I2C bus as for display.
SCL - the I2C (Wire) clock pin. Also GPIO #22.	/


