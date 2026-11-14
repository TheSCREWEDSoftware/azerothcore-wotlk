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
-- Table structure for table `itemdisplayinfo_dbc`
--

DROP TABLE IF EXISTS `itemdisplayinfo_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `itemdisplayinfo_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `ModelName_1` varchar(100) DEFAULT NULL,
  `ModelName_2` varchar(100) DEFAULT NULL,
  `ModelTexture_1` varchar(100) DEFAULT NULL,
  `ModelTexture_2` varchar(100) DEFAULT NULL,
  `InventoryIcon_1` varchar(100) DEFAULT NULL,
  `InventoryIcon_2` varchar(100) DEFAULT NULL,
  `GeosetGroup_1` int(11) NOT NULL DEFAULT 0,
  `GeosetGroup_2` int(11) NOT NULL DEFAULT 0,
  `GeosetGroup_3` int(11) NOT NULL DEFAULT 0,
  `Flags` int(11) NOT NULL DEFAULT 0,
  `SpellVisualID` int(11) NOT NULL DEFAULT 0,
  `GroupSoundIndex` int(11) NOT NULL DEFAULT 0,
  `HelmetGeosetVis_1` int(11) NOT NULL DEFAULT 0,
  `HelmetGeosetVis_2` int(11) NOT NULL DEFAULT 0,
  `Texture_1` varchar(100) DEFAULT NULL,
  `Texture_2` varchar(100) DEFAULT NULL,
  `Texture_3` varchar(100) DEFAULT NULL,
  `Texture_4` varchar(100) DEFAULT NULL,
  `Texture_5` varchar(100) DEFAULT NULL,
  `Texture_6` varchar(100) DEFAULT NULL,
  `Texture_7` varchar(100) DEFAULT NULL,
  `Texture_8` varchar(100) DEFAULT NULL,
  `ItemVisual` int(11) NOT NULL DEFAULT 0,
  `ParticleColorID` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `itemdisplayinfo_dbc`
--

LOCK TABLES `itemdisplayinfo_dbc` WRITE;
/*!40000 ALTER TABLE `itemdisplayinfo_dbc` DISABLE KEYS */;
/*!40000 ALTER TABLE `itemdisplayinfo_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:28
