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
-- Table structure for table `player_race_stats`
--

DROP TABLE IF EXISTS `player_race_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player_race_stats` (
  `Race` tinyint(3) unsigned NOT NULL,
  `Strength` int(11) NOT NULL DEFAULT 0,
  `Agility` int(11) NOT NULL DEFAULT 0,
  `Stamina` int(11) NOT NULL DEFAULT 0,
  `Intellect` int(11) NOT NULL DEFAULT 0,
  `Spirit` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`Race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci PACK_KEYS=0 COMMENT='Stores race stats.';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `player_race_stats`
--

LOCK TABLES `player_race_stats` WRITE;
/*!40000 ALTER TABLE `player_race_stats` DISABLE KEYS */;
INSERT INTO `player_race_stats` VALUES
(1,0,0,0,0,0),
(2,3,-3,1,-3,2),
(3,5,-4,1,-1,-1),
(4,-4,4,0,0,0),
(5,-1,-2,0,-2,5),
(6,5,-4,1,-4,3),
(7,-5,2,0,3,0),
(8,1,2,0,-4,1),
(9,-3,2,0,3,-2),
(10,-3,2,0,3,-2),
(11,1,-3,0,0,2),
(22,3,2,0,-4,-1);
/*!40000 ALTER TABLE `player_race_stats` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:29
