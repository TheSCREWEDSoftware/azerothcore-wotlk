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
-- Table structure for table `destructible_model_data`
--

DROP TABLE IF EXISTS `destructible_model_data`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `destructible_model_data` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDamagedDisplayID` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDamagedImpactEffectDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDamagedAmbientDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDamagedNameSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDestroyedDisplayID` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDestroyedDestructionDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDestroyedImpactEffectDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDestroyedAmbientDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateDestroyedNameSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateRebuildingDisplayID` int(10) unsigned NOT NULL DEFAULT 0,
  `StateRebuildingDestructionDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateRebuildingImpactEffectDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateRebuildingAmbientDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateRebuildingNameSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateSmokeDisplayID` int(10) unsigned NOT NULL DEFAULT 0,
  `StateSmokeInitDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateSmokeAmbientDoodadSet` int(10) unsigned NOT NULL DEFAULT 0,
  `StateSmokeNameSet` int(10) unsigned NOT NULL DEFAULT 0,
  `EjectDirection` int(10) unsigned NOT NULL DEFAULT 0,
  `RepairGroundFx` int(10) unsigned NOT NULL DEFAULT 0,
  `DoNotHighlight` int(10) unsigned NOT NULL DEFAULT 0,
  `HealEffect` int(10) unsigned NOT NULL DEFAULT 0,
  `HealEffectSpeed` int(10) unsigned NOT NULL DEFAULT 0,
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
