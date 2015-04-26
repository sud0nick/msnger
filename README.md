# msnger

A MySQL / PHP / Arduino based messenger


HARDWARE
--

    Arduino Uno R3
    SainSmart LCD Keypad Shield v1.0
    Adafruit CC3000 WiFi Breakout



MySQL
--
Use the following query to create the messages table within your MySQL database.

```
SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- Table structure for table `messages`

CREATE TABLE IF NOT EXISTS `messages` (
  `msg` varchar(39) NOT NULL,
  `author` varchar(15) NOT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`date`),
  KEY `msg` (`msg`,`author`),
  FULLTEXT KEY `author` (`author`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
```
