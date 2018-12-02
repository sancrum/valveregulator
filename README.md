# valveregulator with dual thermocouple sensors
Arduino project to control valve regulator with bipolar stepper motor drive.

Dual thermocouple temperature control

ModbusRTU compatible over serial port.

Manual control by encoder.

Dispalay LCD1602 with I2C driver PCF8574.

**Project schema**
Attention! Present schematic is full of simplifying. Sorry for that.
1. LCD connection does not present it's actual soldering. I've used LCD1602 module with I2C already soldered together. So it has specific 4-wire connection GND-VCC-SDA-SCL.

2. MAX6755 modules drawing is not real-like. It's actual pinout is: 1-GND, 2-VCC, 3-SCK, 4-CS, 5-MISO

3. NEMAmotor + SSR equals to valveregulator Atmix VK43##. It contains bipolar stepper motor (4 coils) on valve and SSR-drived opener. SSR is 3..32DC control and 220AC operation.



**Used libraries are:**
ModbusRTU by  	Samuel Marco i Armengol
                https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

Newliquidcrystal_1.3.5 by Francisco Malpartido
                https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/

Rotary by Brian Low
                https://github.com/brianlow/Rotary

MAX31855 by Enjoyneering
		https://github.com/enjoyneering/MAX31855

Modbus slave ID 1. Modbus registers are:

3 - first thermocouple data *100 multiplier

4 - second thermocouple data *100 multiplier

6 - current power percentage (view changeog version 1.1)


CHANGELOG

version 1.2 || New thermocouple-to-digital converter MAX31855k

version 1.1 || maximum allowed power (100%) equals 85% actual valve regulator opening


