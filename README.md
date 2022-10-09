# Arduino plant watering

With remote control via esp8266.

> Regarding the Y2038 problem, DS3231 is ok but the library will need to be updated or some code changed as it returns 32bit unixtime instead of 64.
> The date in the eeprom is already 64bit.
