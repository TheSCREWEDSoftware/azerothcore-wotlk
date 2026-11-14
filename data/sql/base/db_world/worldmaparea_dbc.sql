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
-- Table structure for table `worldmaparea_dbc`
--

DROP TABLE IF EXISTS `worldmaparea_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `worldmaparea_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `MapID` int(11) NOT NULL DEFAULT 0,
  `AreaID` int(11) NOT NULL DEFAULT 0,
  `AreaName` varchar(100) DEFAULT NULL,
  `LocLeft` float NOT NULL DEFAULT 0,
  `LocRight` float NOT NULL DEFAULT 0,
  `LocTop` float NOT NULL DEFAULT 0,
  `LocBottom` float NOT NULL DEFAULT 0,
  `DisplayMapID` int(11) NOT NULL DEFAULT 0,
  `DefaultDungeonFloor` int(11) NOT NULL DEFAULT 0,
  `ParentWorldMapID` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `worldmaparea_dbc`
--

LOCK TABLES `worldmaparea_dbc` WRITE;
/*!40000 ALTER TABLE `worldmaparea_dbc` DISABLE KEYS */;
INSERT INTO `worldmaparea_dbc` VALUES
(1000,530,3479,'TheVeiledSea',-10075,-14570.8,-758.333,-5508.33,1,0,0),
(1001,530,3455,'TheNorthSea',-4487.5,-9412.5,13568.8,6066.67,0,0,0);
/*!40000 ALTER TABLE `worldmaparea_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:31
