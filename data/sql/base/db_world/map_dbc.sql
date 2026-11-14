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
-- Table structure for table `map_dbc`
--

DROP TABLE IF EXISTS `map_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `map_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `Directory` varchar(100) DEFAULT NULL,
  `InstanceType` int(11) NOT NULL DEFAULT 0,
  `Flags` int(11) NOT NULL DEFAULT 0,
  `PVP` int(11) NOT NULL DEFAULT 0,
  `MapName_Lang_enUS` varchar(100) DEFAULT NULL,
  `MapName_Lang_enGB` varchar(100) DEFAULT NULL,
  `MapName_Lang_koKR` varchar(100) DEFAULT NULL,
  `MapName_Lang_frFR` varchar(100) DEFAULT NULL,
  `MapName_Lang_deDE` varchar(100) DEFAULT NULL,
  `MapName_Lang_enCN` varchar(100) DEFAULT NULL,
  `MapName_Lang_zhCN` varchar(100) DEFAULT NULL,
  `MapName_Lang_enTW` varchar(100) DEFAULT NULL,
  `MapName_Lang_zhTW` varchar(100) DEFAULT NULL,
  `MapName_Lang_esES` varchar(100) DEFAULT NULL,
  `MapName_Lang_esMX` varchar(100) DEFAULT NULL,
  `MapName_Lang_ruRU` varchar(100) DEFAULT NULL,
  `MapName_Lang_ptPT` varchar(100) DEFAULT NULL,
  `MapName_Lang_ptBR` varchar(100) DEFAULT NULL,
  `MapName_Lang_itIT` varchar(100) DEFAULT NULL,
  `MapName_Lang_Unk` varchar(100) DEFAULT NULL,
  `MapName_Lang_Mask` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaTableID` int(11) NOT NULL DEFAULT 0,
  `MapDescription0_Lang_enUS` text DEFAULT NULL,
  `MapDescription0_Lang_enGB` text DEFAULT NULL,
  `MapDescription0_Lang_koKR` text DEFAULT NULL,
  `MapDescription0_Lang_frFR` text DEFAULT NULL,
  `MapDescription0_Lang_deDE` text DEFAULT NULL,
  `MapDescription0_Lang_enCN` text DEFAULT NULL,
  `MapDescription0_Lang_zhCN` text DEFAULT NULL,
  `MapDescription0_Lang_enTW` text DEFAULT NULL,
  `MapDescription0_Lang_zhTW` text DEFAULT NULL,
  `MapDescription0_Lang_esES` text DEFAULT NULL,
  `MapDescription0_Lang_esMX` text DEFAULT NULL,
  `MapDescription0_Lang_ruRU` text DEFAULT NULL,
  `MapDescription0_Lang_ptPT` text DEFAULT NULL,
  `MapDescription0_Lang_ptBR` text DEFAULT NULL,
  `MapDescription0_Lang_itIT` text DEFAULT NULL,
  `MapDescription0_Lang_Unk` varchar(100) DEFAULT NULL,
  `MapDescription0_Lang_Mask` int(10) unsigned NOT NULL DEFAULT 0,
  `MapDescription1_Lang_enUS` text DEFAULT NULL,
  `MapDescription1_Lang_enGB` text DEFAULT NULL,
  `MapDescription1_Lang_koKR` text DEFAULT NULL,
  `MapDescription1_Lang_frFR` text DEFAULT NULL,
  `MapDescription1_Lang_deDE` text DEFAULT NULL,
  `MapDescription1_Lang_enCN` text DEFAULT NULL,
  `MapDescription1_Lang_zhCN` text DEFAULT NULL,
  `MapDescription1_Lang_enTW` text DEFAULT NULL,
  `MapDescription1_Lang_zhTW` text DEFAULT NULL,
  `MapDescription1_Lang_esES` text DEFAULT NULL,
  `MapDescription1_Lang_esMX` text DEFAULT NULL,
  `MapDescription1_Lang_ruRU` text DEFAULT NULL,
  `MapDescription1_Lang_ptPT` text DEFAULT NULL,
  `MapDescription1_Lang_ptBR` text DEFAULT NULL,
  `MapDescription1_Lang_itIT` text DEFAULT NULL,
  `MapDescription1_Lang_Unk` varchar(100) DEFAULT NULL,
  `MapDescription1_Lang_Mask` int(10) unsigned NOT NULL DEFAULT 0,
  `LoadingScreenID` int(11) NOT NULL DEFAULT 0,
  `MinimapIconScale` float NOT NULL DEFAULT 0,
  `CorpseMapID` int(11) NOT NULL DEFAULT 0,
  `CorpseX` float NOT NULL DEFAULT 0,
  `CorpseY` float NOT NULL DEFAULT 0,
  `TimeOfDayOverride` int(11) NOT NULL DEFAULT 0,
  `ExpansionID` int(11) NOT NULL DEFAULT 0,
  `RaidOffset` int(11) NOT NULL DEFAULT 0,
  `MaxPlayers` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `map_dbc`
--

LOCK TABLES `map_dbc` WRITE;
/*!40000 ALTER TABLE `map_dbc` DISABLE KEYS */;
/*!40000 ALTER TABLE `map_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:29
