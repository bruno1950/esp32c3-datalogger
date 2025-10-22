Summary. 
This is the result of an old (75) newbie who is struggling to design an ESP32C3_Supermini based datalogger. 
While using I2C sensors and RTC through I2C was relatively straightforward, making the SD card module and E_Paper display work correctly required a long series of attempts because of the interfernce caused by SD Card on the E-Paper refresh.
The reasonably expected working duration of this datalogger with a single 18650 is of several months ( when getting data every thirty minutes between deepsleep periods) also because the MCP1640 DC DC step up converter is powered off during deepsleep

ESP32-C3 Battery-Powered Datalogger with E-Paper Display
🧠 About This Project
This is a low-power datalogger based on the ESP32-C3 Supermini, designed to run for long periods on a single 18650 Li-Ion battery. It logs environmental data from sensors and displays information on an E-Paper screen, with data stored on an SD card. The system is optimized for deep sleep to maximize battery life.


🔋 Key Features
• 	ESP32-C3 Supermini with deep sleep cycles
• 	MAX17048 battery monitor for voltage and state-of-charge
• 	MCP1640 DC-DC boost converter (3.7V → 5V)
• 	BME280 and SHT31 sensors for temperature, humidity, and pressure
• 	DS3231 RTC for accurate timekeeping
• 	E-Paper display for ultra-low-power visual output
• 	SD card logging of sensor data
• 	GPIO-controlled power to disable boost converter during sleep

🧰 Hardware Used


🧪 How It Works
1. 	On wake-up, the ESP32:
• 	Powers the boost converter via GPIO
• 	Reads battery voltage and SOC from MAX17048
• 	Reads sensors (BME280, SHT31)
• 	Logs data to SD card
• 	Updates the E-Paper display
2. 	Then it:
• 	Powers down the boost converter
• 	Enters deep sleep for 1800 seconds (30 minutes)

⚡ Power Notes
• 	The datalogger works reliably when powered via:
• 	The USB port of the ESP32-C3 Supermini
• 	An accessory USB port, such as a USB-to-DIP adapter mounted on the PCB
• 	The ESP32-C3 Supermini is a compact variant of the ESP32-C3, ideal for space-constrained designs. Its small footprint makes it perfect for portable, battery-powered applications.


🧠 What I Learned
• 	Designing a custom PCB in KiCad 9.0
• 	Managing I2C and SPI bus conflicts
• 	Optimizing for low power consumption
• 	Using GitHub to share and document my work

🤝 Acknowledgments
This project was made possible thanks to the support and insights from:
• 	Microsoft Copilot
• 	Gemini
• 	Perplexity
• 	The wonderful open-source and maker communities

📂 Files
• 	: Final Arduino sketch with corrected sleep interval
• 	More files and schematics coming soon!
