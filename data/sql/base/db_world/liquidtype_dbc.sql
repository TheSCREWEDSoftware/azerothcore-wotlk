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
-- Table structure for table `liquidtype_dbc`
--

DROP TABLE IF EXISTS `liquidtype_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `liquidtype_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `Name` varchar(100) DEFAULT NULL,
  `Flags` int(11) NOT NULL DEFAULT 0,
  `Type` int(11) NOT NULL DEFAULT 0,
  `SoundID` int(11) NOT NULL DEFAULT 0,
  `SpellID` int(11) NOT NULL DEFAULT 0,
  `MaxDarkenDepth` float NOT NULL DEFAULT 0,
  `FogDarkenintensity` float NOT NULL DEFAULT 0,
  `AmbDarkenintensity` float NOT NULL DEFAULT 0,
  `DirDarkenintensity` float NOT NULL DEFAULT 0,
  `LightID` int(11) NOT NULL DEFAULT 0,
  `ParticleScale` float NOT NULL DEFAULT 0,
  `ParticleMovement` int(11) NOT NULL DEFAULT 0,
  `ParticleTexSlots` int(11) NOT NULL DEFAULT 0,
  `MaterialID` int(11) NOT NULL DEFAULT 0,
  `Texture_1` varchar(100) DEFAULT NULL,
  `Texture_2` varchar(100) DEFAULT NULL,
  `Texture_3` varchar(100) DEFAULT NULL,
  `Texture_4` varchar(100) DEFAULT NULL,
  `Texture_5` varchar(100) DEFAULT NULL,
  `Texture_6` varchar(100) DEFAULT NULL,
  `Color_1` int(11) NOT NULL DEFAULT 0,
  `Color_2` int(11) NOT NULL DEFAULT 0,
  `Float_1` float NOT NULL DEFAULT 0,
  `Float_2` float NOT NULL DEFAULT 0,
  `Float_3` float NOT NULL DEFAULT 0,
  `Float_4` float NOT NULL DEFAULT 0,
  `Float_5` float NOT NULL DEFAULT 0,
  `Float_6` float NOT NULL DEFAULT 0,
  `Float_7` float NOT NULL DEFAULT 0,
  `Float_8` float NOT NULL DEFAULT 0,
  `Float_9` float NOT NULL DEFAULT 0,
  `Float_10` float NOT NULL DEFAULT 0,
  `Float_11` float NOT NULL DEFAULT 0,
  `Float_12` float NOT NULL DEFAULT 0,
  `Float_13` float NOT NULL DEFAULT 0,
  `Float_14` float NOT NULL DEFAULT 0,
  `Float_15` float NOT NULL DEFAULT 0,
  `Float_16` float NOT NULL DEFAULT 0,
  `Float_17` float NOT NULL DEFAULT 0,
  `Float_18` float NOT NULL DEFAULT 0,
  `Int_1` int(11) NOT NULL DEFAULT 0,
  `Int_2` int(11) NOT NULL DEFAULT 0,
  `Int_3` int(11) NOT NULL DEFAULT 0,
  `Int_4` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `liquidtype_dbc`
--

LOCK TABLES `liquidtype_dbc` WRITE;
/*!40000 ALTER TABLE `liquidtype_dbc` DISABLE KEYS */;
/*!40000 ALTER TABLE `liquidtype_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:29
