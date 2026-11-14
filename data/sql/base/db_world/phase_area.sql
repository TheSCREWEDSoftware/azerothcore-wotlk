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
-- Table structure for table `phase_area`
--

DROP TABLE IF EXISTS `phase_area`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `phase_area` (
  `AreaId` int(10) unsigned NOT NULL,
  `PhaseId` int(10) unsigned NOT NULL,
  `PhaseGroup` int(10) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`AreaId`,`PhaseId`,`PhaseGroup`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `phase_area`
--

LOCK TABLES `phase_area` WRITE;
/*!40000 ALTER TABLE `phase_area` DISABLE KEYS */;
INSERT INTO `phase_area` VALUES
(108,226,0),
(4714,105,0),
(4714,181,0),
(4755,170,0),
(4755,171,0),
(4756,170,0),
(4756,171,0),
(4757,170,0),
(4757,171,0),
(4757,172,0),
(4757,179,0),
(4758,172,0),
(4758,179,0),
(4759,172,0),
(4759,179,0),
(4761,172,0),
(4761,179,0),
(4861,194,0),
(4979,195,0),
(4989,194,0),
(4990,194,0),
(5013,194,0),
(5014,194,0),
(5016,194,0),
(5018,194,0),
(5019,194,0),
(5020,194,0),
(5022,194,0),
(5140,169,0),
(5140,361,0),
(5424,169,0),
(5424,361,0),
(5602,312,0),
(5602,313,0),
(5602,315,0),
(5602,324,0),
(5602,325,0),
(5700,313,0),
(5700,315,0),
(5700,324,0);
/*!40000 ALTER TABLE `phase_area` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:29
