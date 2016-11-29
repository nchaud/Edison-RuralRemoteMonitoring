# RuralRemoteMonitoring
Multi-channel remote monitoring end-to-end system for rural areas. See my Instructable for how to use this.

This sketch is designed to be loaded in the Arduino IDE and run on an Edison-Mounted SIM808 module.

Installed on a local solar-system site, retrieves data about electrical characteristics for that solar system and environmental conditions and transmits this back to a central location. Via Internet if GPRS connectivity is available, otherwise a compact encoded form via SMS.

Uses a SIM808 ([Adafruit breakout](https://www.adafruit.com/products/2636)) mounted on an [Intel Edison](https://software.intel.com/en-us/iot/hardware/edison) along with various components of a [Grove Starter Kit](https://www.seeedstudio.com/Grove-Starter-Kit-p-709.html)
