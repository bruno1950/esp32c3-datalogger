This is the result of an old (75) newbie who is struggling to design an ESP32C3_Supermini based datalogger. 
While using I2C sensors and RTC through I2C was relatively straightforward, making the SD card module and E_Paper display work correctly required a long series of attempts because of the interfernce caused by SD Card on the E-Paper refresh.
The reasonably expected working duration of this datalogger with a single 18650 is of several months ( when getting data every thirty minutes between deepsleep periods) also because the MCP1640 DC DC step up converter is powered off during deepsleep
