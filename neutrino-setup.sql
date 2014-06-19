DROP DATABASE IF EXISTS `neutrino`;
CREATE DATABASE `neutrino`;

CREATE TABLE neutrino.sensor (
  `sensor_address` tinyint(3) unsigned NOT NULL,
  `display_name` varchar(255) DEFAULT NULL,
  `sensor_group` tinyint(3) unsigned DEFAULT NULL,
  `drives_hvac` tinyint(3) unsigned DEFAULT '0',
  `sensor_hub_id` tinyint(3) unsigned NOT NULL,
  PRIMARY KEY (`sensor_address`),
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

#creates user if not exist. Edit user info to your liking
GRANT USAGE ON neutrino.* TO 'neutrino'@'%';
DROP USER 'neutrino'@'%';
GRANT ALL on neutrino.* to 'neutrino'@'%' IDENTIFIED BY 'electron14';
FLUSH PRIVILEGES;
