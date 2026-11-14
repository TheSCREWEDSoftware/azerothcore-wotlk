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
-- Table structure for table `garr_follower`
--

DROP TABLE IF EXISTS `garr_follower`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `garr_follower` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `FollowerTypeID` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeCreatureID` int(10) unsigned NOT NULL DEFAULT 0,
  `AllianceCreatureID` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeUiAnimRaceInfoID` int(10) unsigned NOT NULL DEFAULT 0,
  `AllianceUiAnimRaceInfoID` int(10) unsigned NOT NULL DEFAULT 0,
  `Quality` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeGarrClassSpecID` int(10) unsigned NOT NULL DEFAULT 0,
  `AllianceGarrClassSpecID` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeGarrFollItemSetID` int(10) unsigned NOT NULL DEFAULT 0,
  `AllianceGarrFollItemSetID` int(10) unsigned NOT NULL DEFAULT 0,
  `Level` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemLevelWeapon` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemLevelArmor` int(10) unsigned NOT NULL DEFAULT 0,
  `Unknown1` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeSourceText` text DEFAULT NULL,
  `AllianceSourceText` text DEFAULT NULL,
  `Unknown2` int(11) NOT NULL DEFAULT 0,
  `Unknown3` int(11) NOT NULL DEFAULT 0,
  `HordePortraitIconID` int(10) unsigned NOT NULL DEFAULT 0,
  `AlliancePortraitIconID` int(10) unsigned NOT NULL DEFAULT 0,
  `HordeListPortraitTextureKitID` int(10) unsigned NOT NULL DEFAULT 0,
  `AllianceListPortraitTextureKitID` int(10) unsigned NOT NULL DEFAULT 0,
  `VerifiedBuild` smallint(6) NOT NULL DEFAULT 0,
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
