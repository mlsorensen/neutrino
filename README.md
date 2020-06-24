neutrino
================

Neutrino environmental monitoring for home HVAC DIY projects

#### WHY?

Neutrino is designed to be an alternative to a zoned HVAC system, or a supplement to a zoned HVAC system that is relatively easy to implement for people who don't want to retrofit to a fully zoned system. In some ways it may even be better than a zoned system, and more cost effective. It also allows the ability to get into the system and have more control over what's going on than commercial systems offer.

Zoned systems don't always work well. Ideally a zoned system heats and cools more efficiently, but unfortunately limitations in the ability for ductwork to supply airflow often cause the system to open a bypass to relieve pressure. This essentially connects the output to the input, short circuiting the system. While the zone that needs air is getting it, capacity is being wasted. In a traditional single zone system this air would go to other rooms that might not need it at the moment, but probably would eventually be called for. This pressure issue is mitigated somewhat by different blower speeds in the furnace, which means a more expensive zoning system. Further, sometimes zone conflicts cause the system to act non-intuitively, like waiting for the blower to stop for one zone before kicking on a request for another zone.

Sometimes a zoned system is overkill for a structure, or needs a boost for higher accuracy, or someone just wants data. There are many reasons, but in the end we do it because we can!

Neutrino gives the DIY user an ability to go in and change how it works. First, by giving an average user the ability to fine-tune their system. Just having detailed information on each room is a big bonus, it allows the use to adjust their vent covers appropriately, which can sometimes make all the difference. On top of that, users can choose which sensors for the HVAC system to pay attention to, whether it be one particular room or an average of multiple. In 2014 it doesn't make much sense to drive the HVAC off of the hallway temperature, unless of course you enjoy hanging out in your hallway. It also gives advanced users the ability to go in and change how the system works, by providing an open system.

#### HOW?

It all starts with sensors. Tiny, inexpensive, battery powered sensors are placed wherever it makes sense to sense the surroundings. This can be on a wall with putty or double-sided tape, on a shelf, etc. Next, sensors are grouped together by the user in software, however they see fit. These groups represent zones, but they don't have to match (for example, a sensor in a room driven by HVAC system A could be included in a sensor group that controls HVAC system B).  Finally, users choose the set hot/cold set points for the group, and toggle which sensors in the group 'count'. The system will average the values of the sensors that count, whether it be a single sensor or ten sensors.

#### WHAT?

Sensors consist of a microcontroller, a temperature/humidity/pressure sensor, a 2.4GHz radio, and a battery. They use radio, but not WiFi, as it is not power efficient or inexpensive to implement. Sensors report to sensor hubs via radio. Up to six sensors can be assigned to a single hub. Sensors are assigned an ID (0 through 5) via jumpers.

Sensor hubs are responsible for getting the sensor info into a database, but don't necessarily correspond to sensor groups (a group can span hubs). The hubs currently consist of a Raspberry Pi with a 2.4Ghz radio attached, and software (sensor-listener) that listens on the radio and plugs the output into a mysql database. Up to eight sensor hubs are supported per system, each sensor-listener is assigned a hub id (0-7) via config file, and the sensors themselves are told which hub to report to via a second set of jumpers.

Then there's the controller. Controllers are a computer and some relays, which control the actual HVAC furnace, AC, humidifier, and blower. The controller software looks into the database for its sensor group's readings, checks it's set points via the controller table, and adjusts the HVAC accordingly. 
 
Finally, there's a web UI for the user to easily see sensor data, create sensor groups, and set the heating/cooling points.

#### CODE?
rpi/sensor-listener: A C++ application for Raspberry Pi. It requires you make/make install the RF24 library (https://github.com/mlsorensen/RF24/tree/master/librf24-rpi/librf24), as well as apt-get install mysql and libconfig++ libs. It will start up, read the config file you supply via '-c' flag (see example config), and start listening for the six radios assigned to it. When it gets a message, it puts it into mysql, and optionally publishes to Zabbix via "Zabbix sender", under the keys 'neutrino.(sensorid).(hubid).temperature', etc..

arduino: This is the code that runs on the sensor microcontrollers. It also contains schematics for the controllers. It requires you have the arduino IDE and import the arduino-version of the RF24 library (https://github.com/mlsorensen/RF24), the BMP180 library (https://github.com/mlsorensen/BMP180.git), and the RocketScream low power library (https://github.com/rocketscream/Low-Power).

controller: This will be the software that controls the controller

web: This will be the UI code

#### DEPLOYMENT

Deployment is currently ad-hoc, but the following may help.

Running the sensor hub:

    $ sudo apt-get install daemon
    $ cd ~/neutrino/hub
    $ sudo daemon --name sensor-listener --respawn --output=daemon.err -- /home/pi/neutrino/hub/sensor-listener -c /etc/sensor-listener.cfg

Stopping the sensor hub:

    $ sudo daemon --name sensor-listener --stop

Running the controller:

    $ cd ~/neutrino/controller
    $ screen -dmS controller sudo ./controller -c /etc/controller.conf

Running the web UI:

    $ cd ~/neutrino/web
    $ sudo hypnotoad neutrino-webapp

Stopping the web UI:

    $ sudo hypnotoad neutrino-webapp -stop
