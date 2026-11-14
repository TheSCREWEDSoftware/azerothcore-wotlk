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
-- Table structure for table `achievement_criteria_dbc`
--

DROP TABLE IF EXISTS `achievement_criteria_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `achievement_criteria_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `Achievement_Id` int(11) NOT NULL DEFAULT 0,
  `Type` int(11) NOT NULL DEFAULT 0,
  `Asset_Id` int(11) NOT NULL DEFAULT 0,
  `Quantity` int(11) NOT NULL DEFAULT 0,
  `Start_Event` int(11) NOT NULL DEFAULT 0,
  `Start_Asset` int(11) NOT NULL DEFAULT 0,
  `Fail_Event` int(11) NOT NULL DEFAULT 0,
  `Fail_Asset` int(11) NOT NULL DEFAULT 0,
  `Description_Lang_enUS` varchar(100) DEFAULT NULL,
  `Description_Lang_enGB` varchar(100) DEFAULT NULL,
  `Description_Lang_koKR` varchar(100) DEFAULT NULL,
  `Description_Lang_frFR` varchar(100) DEFAULT NULL,
  `Description_Lang_deDE` varchar(100) DEFAULT NULL,
  `Description_Lang_enCN` varchar(100) DEFAULT NULL,
  `Description_Lang_zhCN` varchar(100) DEFAULT NULL,
  `Description_Lang_enTW` varchar(100) DEFAULT NULL,
  `Description_Lang_zhTW` varchar(100) DEFAULT NULL,
  `Description_Lang_esES` varchar(100) DEFAULT NULL,
  `Description_Lang_esMX` varchar(100) DEFAULT NULL,
  `Description_Lang_ruRU` varchar(100) DEFAULT NULL,
  `Description_Lang_ptPT` varchar(100) DEFAULT NULL,
  `Description_Lang_ptBR` varchar(100) DEFAULT NULL,
  `Description_Lang_itIT` varchar(100) DEFAULT NULL,
  `Description_Lang_Unk` varchar(100) DEFAULT NULL,
  `Description_Lang_Mask` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags` int(11) NOT NULL DEFAULT 0,
  `Timer_Start_Event` int(11) NOT NULL DEFAULT 0,
  `Timer_Asset_Id` int(11) NOT NULL DEFAULT 0,
  `Timer_Time` int(11) NOT NULL DEFAULT 0,
  `Ui_Order` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `achievement_criteria_dbc`
--

LOCK TABLES `achievement_criteria_dbc` WRITE;
/*!40000 ALTER TABLE `achievement_criteria_dbc` DISABLE KEYS */;
/*!40000 ALTER TABLE `achievement_criteria_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:24
