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
-- Table structure for table `player_condition`
--

DROP TABLE IF EXISTS `player_condition`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `player_condition` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags` int(10) unsigned NOT NULL DEFAULT 0,
  `MinLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `RaceMask` int(10) unsigned NOT NULL DEFAULT 0,
  `ClassMask` int(10) unsigned NOT NULL DEFAULT 0,
  `Gender` int(11) NOT NULL DEFAULT 0,
  `NativeGender` int(11) NOT NULL DEFAULT 0,
  `SkillID1` int(10) unsigned NOT NULL DEFAULT 0,
  `SkillID2` int(10) unsigned NOT NULL DEFAULT 0,
  `SkillID3` int(10) unsigned NOT NULL DEFAULT 0,
  `SkillID4` int(10) unsigned NOT NULL DEFAULT 0,
  `MinSkill1` int(11) NOT NULL DEFAULT 0,
  `MinSkill2` int(11) NOT NULL DEFAULT 0,
  `MinSkill3` int(11) NOT NULL DEFAULT 0,
  `MinSkill4` int(11) NOT NULL DEFAULT 0,
  `MaxSkill1` int(11) NOT NULL DEFAULT 0,
  `MaxSkill2` int(11) NOT NULL DEFAULT 0,
  `MaxSkill3` int(11) NOT NULL DEFAULT 0,
  `MaxSkill4` int(11) NOT NULL DEFAULT 0,
  `SkillLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `LanguageID` int(10) unsigned NOT NULL DEFAULT 0,
  `MinLanguage` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxLanguage` int(10) unsigned NOT NULL DEFAULT 0,
  `MinFactionID1` int(10) unsigned NOT NULL DEFAULT 0,
  `MinFactionID2` int(10) unsigned NOT NULL DEFAULT 0,
  `MinFactionID3` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxFactionID` int(10) unsigned NOT NULL DEFAULT 0,
  `MinReputation1` int(10) unsigned NOT NULL DEFAULT 0,
  `MinReputation2` int(10) unsigned NOT NULL DEFAULT 0,
  `MinReputation3` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxReputation` int(10) unsigned NOT NULL DEFAULT 0,
  `ReputationLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `Unknown1` int(10) unsigned NOT NULL DEFAULT 0,
  `MinPVPRank` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxPVPRank` int(10) unsigned NOT NULL DEFAULT 0,
  `PvpMedal` int(10) unsigned NOT NULL DEFAULT 0,
  `PrevQuestLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `PrevQuestID1` int(10) unsigned NOT NULL DEFAULT 0,
  `PrevQuestID2` int(10) unsigned NOT NULL DEFAULT 0,
  `PrevQuestID3` int(10) unsigned NOT NULL DEFAULT 0,
  `PrevQuestID4` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrQuestLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrQuestID1` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrQuestID2` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrQuestID3` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrQuestID4` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrentCompletedQuestLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrentCompletedQuestID1` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrentCompletedQuestID2` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrentCompletedQuestID3` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrentCompletedQuestID4` int(10) unsigned NOT NULL DEFAULT 0,
  `SpellLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `SpellID1` int(10) unsigned NOT NULL DEFAULT 0,
  `SpellID2` int(10) unsigned NOT NULL DEFAULT 0,
  `SpellID3` int(10) unsigned NOT NULL DEFAULT 0,
  `SpellID4` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemID1` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemID2` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemID3` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemID4` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemCount1` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemCount2` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemCount3` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemCount4` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemFlags` int(10) unsigned NOT NULL DEFAULT 0,
  `Explored1` int(10) unsigned NOT NULL DEFAULT 0,
  `Explored2` int(10) unsigned NOT NULL DEFAULT 0,
  `Time1` int(10) unsigned NOT NULL DEFAULT 0,
  `Time2` int(10) unsigned NOT NULL DEFAULT 0,
  `AuraSpellLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `AuraSpellID1` int(10) unsigned NOT NULL DEFAULT 0,
  `AuraSpellID2` int(10) unsigned NOT NULL DEFAULT 0,
  `AuraSpellID3` int(10) unsigned NOT NULL DEFAULT 0,
  `AuraSpellID4` int(10) unsigned NOT NULL DEFAULT 0,
  `WorldStateExpressionID` int(10) unsigned NOT NULL DEFAULT 0,
  `WeatherID` int(10) unsigned NOT NULL DEFAULT 0,
  `PartyStatus` int(10) unsigned NOT NULL DEFAULT 0,
  `LifetimeMaxPVPRank` int(10) unsigned NOT NULL DEFAULT 0,
  `AchievementLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `Achievement1` int(10) unsigned NOT NULL DEFAULT 0,
  `Achievement2` int(10) unsigned NOT NULL DEFAULT 0,
  `Achievement3` int(10) unsigned NOT NULL DEFAULT 0,
  `Achievement4` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgStatus1` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgStatus2` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgStatus3` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgStatus4` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgCompare1` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgCompare2` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgCompare3` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgCompare4` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgValue1` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgValue2` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgValue3` int(10) unsigned NOT NULL DEFAULT 0,
  `LfgValue4` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaID1` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaID2` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaID3` int(10) unsigned NOT NULL DEFAULT 0,
  `AreaID4` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyID1` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyID2` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyID3` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyID4` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyCount1` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyCount2` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyCount3` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencyCount4` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillID` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillLogic` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillMonster1` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillMonster2` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillMonster3` int(10) unsigned NOT NULL DEFAULT 0,
  `QuestKillMonster4` int(10) unsigned NOT NULL DEFAULT 0,
  `MinExpansionLevel` int(11) NOT NULL DEFAULT 0,
  `MaxExpansionLevel` int(11) NOT NULL DEFAULT 0,
  `MinExpansionTier` int(11) NOT NULL DEFAULT 0,
  `MaxExpansionTier` int(11) NOT NULL DEFAULT 0,
  `MinGuildLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxGuildLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `PhaseUseFlags` int(10) unsigned NOT NULL DEFAULT 0,
  `PhaseID` int(10) unsigned NOT NULL DEFAULT 0,
  `PhaseGroupID` int(10) unsigned NOT NULL DEFAULT 0,
  `MinAvgItemLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxAvgItemLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `MinAvgEquippedItemLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxAvgEquippedItemLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `ChrSpecializationIndex` int(11) NOT NULL DEFAULT 0,
  `ChrSpecializationRole` int(11) NOT NULL DEFAULT 0,
  `FailureDescription` text DEFAULT NULL,
  `PowerType` int(11) NOT NULL DEFAULT 0,
  `PowerTypeComp` int(11) NOT NULL DEFAULT 0,
  `PowerTypeValue` int(11) NOT NULL DEFAULT 0,
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
