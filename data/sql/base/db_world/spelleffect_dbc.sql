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
-- Table structure for table `spelleffect_dbc`
--

DROP TABLE IF EXISTS `spelleffect_dbc`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `spelleffect_dbc` (
  `ID` int(10) unsigned NOT NULL,
  `Effect` int(10) unsigned NOT NULL,
  `EffectAmplitude` float NOT NULL,
  `EffectAura` int(10) unsigned NOT NULL,
  `EffectAuraPeriod` int(10) unsigned NOT NULL,
  `EffectBasePoints` int(11) NOT NULL,
  `EffectBonusCoefficient` float NOT NULL,
  `EffectChainAmplitude` float NOT NULL,
  `EffectChainTargets` int(10) unsigned NOT NULL,
  `EffectDieSides` int(11) NOT NULL,
  `EffectItemType` int(10) unsigned NOT NULL,
  `EffectMechanic` int(10) unsigned NOT NULL,
  `EffectMiscValue` int(11) NOT NULL,
  `EffectMiscValueB` int(11) NOT NULL,
  `EffectPointsPerResource` float NOT NULL,
  `EffectRadiusIndex` int(10) unsigned NOT NULL,
  `EffectRadiusMaxIndex` int(10) unsigned NOT NULL,
  `EffectRealPointsPerLevel` float NOT NULL,
  `EffectSpellClassMask1` int(10) unsigned NOT NULL,
  `EffectSpellClassMask2` int(10) unsigned NOT NULL,
  `EffectSpellClassMask3` int(10) unsigned NOT NULL,
  `EffectTriggerSpell` int(10) unsigned NOT NULL,
  `EffectImplicitTargetA` int(10) unsigned NOT NULL,
  `EffectImplicitTargetB` int(10) unsigned NOT NULL,
  `SpellID` int(10) unsigned NOT NULL,
  `EffectIndex` int(10) unsigned NOT NULL,
  `EffectAttributes` int(10) unsigned NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `spelleffect_dbc`
--

LOCK TABLES `spelleffect_dbc` WRITE;
/*!40000 ALTER TABLE `spelleffect_dbc` DISABLE KEYS */;
INSERT INTO `spelleffect_dbc` VALUES
(153050,6,0,4,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,5302,0,0);
/*!40000 ALTER TABLE `spelleffect_dbc` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:31
