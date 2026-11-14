-- MySQL dump 10.13  Distrib 8.4.3, for Win64 (x86_64)
--
-- Host: localhost    Database: acore_hotfixes
-- ------------------------------------------------------
-- Server version	8.4.3

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `gameobjects`
--

DROP TABLE IF EXISTS `gameobjects`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `gameobjects` (
  `ID` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `MapID` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `DisplayID` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `PositionX` float NOT NULL DEFAULT 0,
  `PositionY` float NOT NULL DEFAULT 0,
  `PositionZ` float NOT NULL DEFAULT 0,
  `RotationX` float NOT NULL DEFAULT 0,
  `RotationY` float NOT NULL DEFAULT 0,
  `RotationZ` float NOT NULL DEFAULT 0,
  `RotationW` float NOT NULL DEFAULT 0,
  `Size` float NOT NULL DEFAULT 0,
  `PhaseUseFlags` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `PhaseID` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `PhaseGroupID` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `Type` mediumint(8) unsigned NOT NULL DEFAULT 0,
  `Data1` int(10) NOT NULL DEFAULT 0,
  `Data2` int(10) NOT NULL DEFAULT 0,
  `Data3` int(10) NOT NULL DEFAULT 0,
  `Data4` int(10) NOT NULL DEFAULT 0,
  `Data5` int(10) NOT NULL DEFAULT 0,
  `Data6` int(10) NOT NULL DEFAULT 0,
  `Data7` int(10) NOT NULL DEFAULT 0,
  `Data8` int(10) NOT NULL DEFAULT 0,
  `Name` text NOT NULL,
  `VerifiedBuild` smallint(5) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-06-01 22:41:25
