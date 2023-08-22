create database Weather;

use Weather;

CREATE TABLE IF NOT EXISTS `WeatherReport` (
  `id` INT NOT NULL,
  `Temperature` DECIMAL(5,2) NULL,
  `Humidity` DECIMAL(5,2) NULL,
  `Timestamp` DATETIME NULL,
  PRIMARY KEY (`id`));
  
  #describe WeatherReport;