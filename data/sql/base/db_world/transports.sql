/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19  Distrib 10.11.14-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: acore_world_build
-- ------------------------------------------------------
-- Server version	10.11.14-MariaDB-0ubuntu0.24.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `transports`
--

DROP TABLE IF EXISTS `transports`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `transports` (
  `guid` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `entry` int(10) unsigned NOT NULL DEFAULT 0,
  `name` text DEFAULT NULL,
  `ScriptName` char(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`guid`),
  UNIQUE KEY `idx_entry` (`entry`)
) ENGINE=InnoDB AUTO_INCREMENT=21 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Transports';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `transports`
--

LOCK TABLES `transports` WRITE;
/*!40000 ALTER TABLE `transports` DISABLE KEYS */;
INSERT INTO `transports` VALUES
(1,176495,'Grom\'gol Base Camp, Stranglethorn Vale and Undercity, Tirisfal Glades (Zeppelin, Horde (\"The Purple Princess\"))',''),
(2,176310,'Auberdine, Darkshore and Stormwind Harbor (Boat, Alliance (\"The Bravery\"))',''),
(3,176244,'Auberdine, Darkshore and Rut\'theran Village, Teldrassil (Boat, Alliance (\"The Moonspray\"))',''),
(4,176231,'Menethil Harbor, Wetlands and Theramore Isle, Dustwallow Marsh (Boat, Alliance (\"The Lady Mehley\"))',''),
(5,175080,'Grom\'gol Base Camp, Stranglethorn Vale and Orgrimmar, Durotar (Zeppelin, Horde (\"The Iron Eagle\"))',''),
(6,164871,'Undercity, Tirisfal Glades and Orgrimmar, Durotar (Zeppelin, Horde (\"The Thundercaller\"))',''),
(7,20808,'Booty Bay, Stranglethorn Vale and Ratchet, The Barrens (Boat, Neutral (\"The Maiden\'s Fancy\"))',''),
(8,177233,'The Forgotten Coast, Feralas and Feathermoon Stronghold, Feralas (Boat, Alliance (\"Feathermoon Ferry\"))',''),
(9,181646,'Auberdine, Darkshore and Valaar\'s Berth, Azuremyst Isle (Boat, Alliance (\"Elune\'s Blessing\"))',''),
(10,181688,'Menethil Harbor, Wetlands and Valgarde, Howling Fjord (Boat, Alliance (\"Northspear\"))',''),
(11,181689,'Undercity, Tirisfal Glades and Vengeance Landing, Howling Fjord (Zeppelin, Horde (\"Cloudkisser\"))',''),
(12,186238,'Orgrimmar, Durotar and Warsong Hold, Borean Tundra (Zeppelin, Horde (\"The Mighty Wind\"))',''),
(13,186371,'Westguard Keep, Howling Fjord and Shattered Straits, Howling Fjord (Zeppelin, Alliance)',''),
(14,187038,'Garvan\'s Reef (Boat, Neutral (\"Sister Mercy\"))',''),
(15,187568,'Unu\'pe, Borean Tundra and Moa\'ki Harbor, Dragonblight (Turtle, Neutral (\"Walker of Waves\"))',''),
(16,188511,'Kamagua, Howling Fjord and Moa\'ki Harbor, Dragonblight (Turtle, Neutral (\"Green Island\"))',''),
(17,190536,'Valiance Keep, Borean Tundra and Stormwind Harbor (Boat, Alliance (\"The Kraken\"))',''),
(18,192241,'Icecrown (Gunship, Horde (\"Orgrim\'s Hammer\"))',''),
(19,192242,'Icecrown (Gunship, Alliance (\"The Skybreaker\"))',''),
(20,190549,'Orgrimmar, Durotar and Thunder Bluff, Mulgore (Zeppelin, Horde (\"The Zephyr\"))','');
/*!40000 ALTER TABLE `transports` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:31
