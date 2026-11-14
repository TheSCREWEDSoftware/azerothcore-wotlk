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
-- Table structure for table `spellvisual_dbc`
--

DROP TABLE IF EXISTS `spellvisual_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `spellvisual_dbc` (
  `ID` int(11) NOT NULL DEFAULT 0,
  `PrecastKit` int(11) NOT NULL DEFAULT 0,
  `CastKit` int(11) NOT NULL DEFAULT 0,
  `ImpactKit` int(11) NOT NULL DEFAULT 0,
  `StateKit` int(11) NOT NULL DEFAULT 0,
  `StateDoneKit` int(11) NOT NULL DEFAULT 0,
  `ChannelKit` int(11) NOT NULL DEFAULT 0,
  `HasMissile` int(11) NOT NULL DEFAULT 0,
  `MissileModel` int(11) NOT NULL DEFAULT 0,
  `MissilePathType` int(11) NOT NULL DEFAULT 0,
  `MissileDestinationAttachment` int(11) NOT NULL DEFAULT 0,
  `MissileSound` int(11) NOT NULL DEFAULT 0,
  `AnimEventSoundID` int(11) NOT NULL DEFAULT 0,
  `Flags` int(11) NOT NULL DEFAULT 0,
  `CasterImpactKit` int(11) NOT NULL DEFAULT 0,
  `TargetImpactKit` int(11) NOT NULL DEFAULT 0,
  `MissileAttachment` int(11) NOT NULL DEFAULT 0,
  `MissileFollowGroundHeight` int(11) NOT NULL DEFAULT 0,
  `MissileFollowGroundDropSpeed` int(11) NOT NULL DEFAULT 0,
  `MissileFollowGroundApproach` int(11) NOT NULL DEFAULT 0,
  `MissileFollowGroundFlags` int(11) NOT NULL DEFAULT 0,
  `MissileMotion` int(11) NOT NULL DEFAULT 0,
  `MissileTargetingKit` int(11) NOT NULL DEFAULT 0,
  `InstantAreaKit` int(11) NOT NULL DEFAULT 0,
  `ImpactAreaKit` int(11) NOT NULL DEFAULT 0,
  `PersistentAreaKit` int(11) NOT NULL DEFAULT 0,
  `MissileCastOffsetX` float NOT NULL DEFAULT 0,
  `MissileCastOffsetY` float NOT NULL DEFAULT 0,
  `MissileCastOffsetZ` float NOT NULL DEFAULT 0,
  `MissileImpactOffsetX` float NOT NULL DEFAULT 0,
  `MissileImpactOffsetY` float NOT NULL DEFAULT 0,
  `MissileImpactOffsetZ` float NOT NULL DEFAULT 0,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `spellvisual_dbc`
--

LOCK TABLES `spellvisual_dbc` WRITE;
/*!40000 ALTER TABLE `spellvisual_dbc` DISABLE KEYS */;
/*!40000 ALTER TABLE `spellvisual_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:31
