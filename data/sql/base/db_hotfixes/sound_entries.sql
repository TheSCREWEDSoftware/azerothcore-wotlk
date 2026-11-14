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
-- Table structure for table `sound_entries`
--

DROP TABLE IF EXISTS `sound_entries`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sound_entries` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `SoundType` int(10) unsigned NOT NULL DEFAULT 0,
  `Name` text DEFAULT NULL,
  `FileDataID1` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID2` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID3` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID4` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID5` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID6` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID7` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID8` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID9` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID10` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID11` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID12` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID13` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID14` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID15` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID16` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID17` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID18` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID19` int(10) unsigned NOT NULL DEFAULT 0,
  `FileDataID20` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq1` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq2` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq3` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq4` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq5` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq6` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq7` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq8` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq9` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq10` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq11` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq12` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq13` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq14` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq15` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq16` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq17` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq18` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq19` int(10) unsigned NOT NULL DEFAULT 0,
  `Freq20` int(10) unsigned NOT NULL DEFAULT 0,
  `VolumeFloat` float NOT NULL DEFAULT 0,
  `Flags` int(10) unsigned NOT NULL DEFAULT 0,
  `MinDistance` float NOT NULL DEFAULT 0,
  `DistanceCutoff` float NOT NULL DEFAULT 0,
  `EAXDef` int(10) unsigned NOT NULL DEFAULT 0,
  `SoundEntriesAdvancedID` int(10) unsigned NOT NULL DEFAULT 0,
  `VolumeVariationPlus` float NOT NULL DEFAULT 0,
  `VolumeVariationMinus` float NOT NULL DEFAULT 0,
  `PitchVariationPlus` float NOT NULL DEFAULT 0,
  `PitchVariationMinus` float NOT NULL DEFAULT 0,
  `PitchAdjust` float NOT NULL DEFAULT 0,
  `DialogType` int(10) unsigned NOT NULL DEFAULT 0,
  `BusOverwriteID` int(10) unsigned NOT NULL DEFAULT 0,
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
