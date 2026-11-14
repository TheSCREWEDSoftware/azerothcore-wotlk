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
-- Table structure for table `creature_movement_override`
--

DROP TABLE IF EXISTS `creature_movement_override`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `creature_movement_override` (
  `SpawnId` int(10) unsigned NOT NULL DEFAULT 0,
  `Ground` tinyint(3) unsigned DEFAULT NULL,
  `Swim` tinyint(3) unsigned DEFAULT NULL,
  `Flight` tinyint(3) unsigned DEFAULT NULL,
  `Rooted` tinyint(3) unsigned DEFAULT NULL,
  `Chase` tinyint(3) unsigned DEFAULT NULL,
  `Random` tinyint(3) unsigned DEFAULT NULL,
  `InteractionPauseTimer` int(10) unsigned DEFAULT NULL COMMENT 'Time (in milliseconds) during which creature will not move after interaction with player',
  PRIMARY KEY (`SpawnId`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `creature_movement_override`
--

LOCK TABLES `creature_movement_override` WRITE;
/*!40000 ALTER TABLE `creature_movement_override` DISABLE KEYS */;
INSERT INTO `creature_movement_override` VALUES
(74081,NULL,NULL,1,NULL,NULL,NULL,NULL),
(74082,NULL,NULL,1,NULL,NULL,NULL,NULL),
(106339,1,1,2,0,0,0,NULL),
(106340,1,1,2,0,0,0,NULL),
(117664,1,0,1,0,0,0,NULL),
(117670,1,0,1,0,0,0,NULL),
(117671,1,0,1,0,0,0,NULL),
(117672,1,0,1,0,0,0,NULL),
(117677,1,0,1,0,0,0,NULL),
(125724,1,1,2,0,0,0,NULL),
(130896,1,0,1,0,0,0,NULL),
(1971380,1,1,2,0,0,0,NULL);
/*!40000 ALTER TABLE `creature_movement_override` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:26
