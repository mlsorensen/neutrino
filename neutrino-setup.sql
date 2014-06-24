DROP DATABASE IF EXISTS `neutrino`;
CREATE DATABASE `neutrino`;

CREATE TABLE neutrino.sensor (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_address` tinyint(3) unsigned NOT NULL,
  `display_name` varchar(255) DEFAULT NULL,
  `sensor_group` tinyint(3) unsigned DEFAULT NULL,
  `drives_hvac` tinyint(3) unsigned DEFAULT '0',
  `sensor_hub_id` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `sensor_address` (`sensor_address`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.data (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_address` tinyint(3) unsigned NOT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `voltage` float DEFAULT NULL,
  `temperature` float DEFAULT NULL,
  `humidity` float DEFAULT NULL,
  `pressure` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `sensor_address` (`sensor_address`),
  CONSTRAINT `data_ibfk_1` FOREIGN KEY (`sensor_address`) REFERENCES `sensor` (`sensor_address`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.configuration (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) UNIQUE NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.controller (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `display_name` varchar(255) NOT NULL,
  `type` enum('temperature','humidity') NOT NULL,
  `setpoint` float DEFAULT NULL,
  `tolerance` float NOT NULL DEFAULT 3,
  `enabled` tinyint(3) unsigned,
  `status` enum('heating','cooling','fan','idle') NOT NULL DEFAULT 'idle',
  `fan_mode` enum('on','auto') NOT NULL DEFAULT 'auto',
   PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.sensorgroup (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `display_name` varchar(255) NOT NULL,
  `controller_id` int(10) unsigned UNIQUE DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `controller_id` (`controller_id`),
  CONSTRAINT `sensorgroup_ibfk_1` FOREIGN KEY (`controller_id`) REFERENCES `controller` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.sensor_tie (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_id` int(10) unsigned NOT NULL,
  `sensorgroup_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  FOREIGN KEY (`sensor_id`) REFERENCES neutrino.sensor(`id`),
  FOREIGN KEY (`sensorgroup_id`) REFERENCES neutrino.sensorgroup(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

#creates user if not exist. Edit user info to your liking
GRANT USAGE ON neutrino.* TO 'neutrino'@'%';
DROP USER 'neutrino'@'%';
GRANT ALL on neutrino.* to 'neutrino'@'%' IDENTIFIED BY 'electron14';

GRANT USAGE ON neutrino.* TO 'neutrino'@'localhost';
DROP USER 'neutrino'@'localhost';
GRANT ALL on neutrino.* to 'neutrino'@'localhost' IDENTIFIED BY 'electron14';
FLUSH PRIVILEGES;
