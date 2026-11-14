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
-- Table structure for table `spell_cata_dbc`
--

DROP TABLE IF EXISTS `spell_cata_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `spell_cata_dbc` (
  `ID` int(10) unsigned NOT NULL,
  `Attributes` int(10) unsigned NOT NULL,
  `AttributesEx` int(10) unsigned NOT NULL,
  `AttributesEx2` int(10) unsigned NOT NULL,
  `AttributesEx3` int(10) unsigned NOT NULL,
  `AttributesEx4` int(10) unsigned NOT NULL,
  `AttributesEx5` int(10) unsigned NOT NULL,
  `AttributesEx6` int(10) unsigned NOT NULL,
  `AttributesEx7` int(10) unsigned NOT NULL,
  `AttributesEx8` int(10) unsigned NOT NULL,
  `AttributesEx9` int(10) unsigned NOT NULL,
  `AttributesEx10` int(10) unsigned NOT NULL,
  `CastingTimeIndex` int(10) unsigned NOT NULL,
  `DurationIndex` int(10) unsigned NOT NULL,
  `PowerType` int(10) unsigned NOT NULL,
  `RangeIndex` int(10) unsigned NOT NULL,
  `Speed` float NOT NULL,
  `SpellVisualID1` int(10) unsigned NOT NULL,
  `SpellVisualID2` int(10) unsigned NOT NULL,
  `SpellIconID` int(10) unsigned NOT NULL,
  `ActiveIconID` int(10) unsigned NOT NULL,
  `Name` text NOT NULL,
  `NameSubtext` text NOT NULL,
  `Description` text NOT NULL,
  `AuraDescription` text NOT NULL,
  `SchoolMask` int(10) unsigned NOT NULL,
  `RuneCostID` int(10) unsigned NOT NULL,
  `SpellMissileID` int(10) unsigned NOT NULL,
  `DescriptionVariablesID` int(10) unsigned NOT NULL,
  `Difficulty` int(10) unsigned NOT NULL,
  `BonusCoefficient` float NOT NULL,
  `ScalingID` int(10) unsigned NOT NULL,
  `AuraOptionsID` int(10) unsigned NOT NULL,
  `AuraRestrictionsID` int(10) unsigned NOT NULL,
  `CastingRequirementsID` int(10) unsigned NOT NULL,
  `CategoriesID` int(10) unsigned NOT NULL,
  `ClassOptionsID` int(10) unsigned NOT NULL,
  `CooldownsID` int(10) unsigned NOT NULL,
  `Unknown38` int(10) unsigned NOT NULL,
  `EquippedItemsID` int(10) unsigned NOT NULL,
  `InterruptsID` int(10) unsigned NOT NULL,
  `LevelsID` int(10) unsigned NOT NULL,
  `PowerDisplayID` int(10) unsigned NOT NULL,
  `ReagentsID` int(10) unsigned NOT NULL,
  `ShapeshiftID` int(10) unsigned NOT NULL,
  `TargetRestrictionsID` int(10) unsigned NOT NULL,
  `TotemsID` int(10) unsigned NOT NULL,
  `RequiredProjectID` int(10) unsigned NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `spell_cata_dbc`
--

LOCK TABLES `spell_cata_dbc` WRITE;
/*!40000 ALTER TABLE `spell_cata_dbc` DISABLE KEYS */;
INSERT INTO `spell_cata_dbc` VALUES
(5302,536871312,1024,0,0,0,0,0,0,0,0,0,1,28,0,1,0,0,0,0,0,'','','','',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
/*!40000 ALTER TABLE `spell_cata_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:31
