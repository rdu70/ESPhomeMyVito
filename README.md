# MyVito for ESPHome


This ESPHome config use custom components to handle communication with a Vitotronic 200 KW regulation through Optical Link interface.  ESPHome config done in YAML and a specific custom component in C++.

Features :

* Get all availlable data (temperatures, pump status, burner status & error, time, schedules, ...)
* Set a few ones (set point for normal, reduced mode, mode selection, water temperature set point, ...).
* Connection to the Viessmann regulation through optolink port (serial over InfraRed)

Hardware :
* The interface can be easily build using an Infrared LED and an Infrared phototransistor.  More info here : https://github.com/openv/openv/wiki/Bauanleitung-ESP8266
!(https://github.com/rdu70/ESPhomeMyVito/blob/master/docs/esp_proto.png)
!(https://github.com/rdu70/ESPhomeMyVito/blob/master/docs/Optolink_front.png)
!(https://github.com/rdu70/ESPhomeMyVito/blob/master/docs/Optolink_bottom.png)
