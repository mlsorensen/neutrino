DROP DATABASE IF EXISTS `neutrino`;
CREATE DATABASE `neutrino`;

CREATE TABLE neutrino.sensor (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_address` tinyint(3) unsigned NOT NULL,
  `display_name` varchar(255) DEFAULT NULL,
  `sensor_group` tinyint(3) unsigned DEFAULT NULL,
  `drives_hvac` tinyint(3) unsigned DEFAULT '0',
  `sensor_hub_id` tinyint(3) unsigned NOT NULL,
  `sensor_encryption_key` binary(10) DEFAULT NULL,
  `sensor_signature_key` binary(4) DEFAULT NULL,
  UNIQUE `unique_index`(`sensor_address`,`sensor_hub_id`),
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.data (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `sensor_id` int(10) unsigned NOT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `voltage` float DEFAULT NULL,
  `fahrenheit` float DEFAULT NULL,
  `celsius` float DEFAULT NULL,
  `humidity` float DEFAULT NULL,
  `pascals` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  FOREIGN KEY (`sensor_id`) REFERENCES neutrino.sensor(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.configuration (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) UNIQUE NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.controller (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `display_name` varchar(255) UNIQUE NOT NULL,
  `enabled` enum('on','off') NOT NULL DEFAULT 'off',
  `status` enum('heat','cool','humidify','heat+humidify','fan','idle') NOT NULL DEFAULT 'idle',
  `fan_mode` enum('on','auto') NOT NULL DEFAULT 'auto',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.controller_capabilities (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `controller_id` int(10) unsigned NOT NULL,
  `capability` enum('heat','cool','humidify'),
  `setpoint` float DEFAULT NULL,
  UNIQUE `unique_index`(`controller_id`,`capability`),
  PRIMARY KEY (`id`),
  FOREIGN KEY (`controller_id`) REFERENCES `controller` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE neutrino.sensorgroup (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `display_name` varchar(255) UNIQUE NOT NULL,
  `controller_id` int(10) unsigned UNIQUE DEFAULT NULL,
  PRIMARY KEY (`id`),
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

INSERT INTO neutrino.configuration (name, value) VALUES ('location', 'Orem, UT');
INSERT INTO neutrino.configuration (name, value) VALUES ('tempunits', 'Fahrenheit');
INSERT INTO neutrino.configuration (name, value) VALUES ('graphtime', 1);
INSERT INTO neutrino.configuration (name, value) VALUES ('discovery', 1);
