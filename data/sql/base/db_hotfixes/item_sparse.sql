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
-- Table structure for table `item_sparse`
--

DROP TABLE IF EXISTS `item_sparse`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `item_sparse` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `Quality` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags1` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags2` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags3` int(10) unsigned NOT NULL DEFAULT 0,
  `Unk1` float NOT NULL DEFAULT 0,
  `Unk2` float NOT NULL DEFAULT 0,
  `BuyCount` int(10) unsigned NOT NULL DEFAULT 0,
  `BuyPrice` int(10) unsigned NOT NULL DEFAULT 0,
  `SellPrice` int(10) unsigned NOT NULL DEFAULT 0,
  `InventoryType` int(10) unsigned NOT NULL DEFAULT 0,
  `AllowableClass` int(11) NOT NULL DEFAULT 0,
  `AllowableRace` int(11) NOT NULL DEFAULT 0,
  `ItemLevel` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredLevel` int(11) NOT NULL DEFAULT 0,
  `RequiredSkill` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredSkillRank` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredSpell` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredHonorRank` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredCityRank` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredReputationFaction` int(10) unsigned NOT NULL DEFAULT 0,
  `RequiredReputationRank` int(10) unsigned NOT NULL DEFAULT 0,
  `MaxCount` int(10) unsigned NOT NULL DEFAULT 0,
  `Stackable` int(10) unsigned NOT NULL DEFAULT 0,
  `ContainerSlots` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemStatType1` int(11) NOT NULL DEFAULT 0,
  `ItemStatType2` int(11) NOT NULL DEFAULT 0,
  `ItemStatType3` int(11) NOT NULL DEFAULT 0,
  `ItemStatType4` int(11) NOT NULL DEFAULT 0,
  `ItemStatType5` int(11) NOT NULL DEFAULT 0,
  `ItemStatType6` int(11) NOT NULL DEFAULT 0,
  `ItemStatType7` int(11) NOT NULL DEFAULT 0,
  `ItemStatType8` int(11) NOT NULL DEFAULT 0,
  `ItemStatType9` int(11) NOT NULL DEFAULT 0,
  `ItemStatType10` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue1` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue2` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue3` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue4` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue5` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue6` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue7` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue8` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue9` int(11) NOT NULL DEFAULT 0,
  `ItemStatValue10` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation1` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation2` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation3` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation4` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation5` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation6` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation7` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation8` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation9` int(11) NOT NULL DEFAULT 0,
  `ItemStatAllocation10` int(11) NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier1` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier2` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier3` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier4` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier5` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier6` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier7` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier8` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier9` float NOT NULL DEFAULT 0,
  `ItemStatSocketCostMultiplier10` float NOT NULL DEFAULT 0,
  `ScalingStatDistribution` int(10) unsigned NOT NULL DEFAULT 0,
  `DamageType` int(10) unsigned NOT NULL DEFAULT 0,
  `Delay` int(10) unsigned NOT NULL DEFAULT 0,
  `RangedModRange` float NOT NULL DEFAULT 0,
  `Bonding` int(10) unsigned NOT NULL DEFAULT 0,
  `Name` text DEFAULT NULL,
  `Name2` text DEFAULT NULL,
  `Name3` text DEFAULT NULL,
  `Name4` text DEFAULT NULL,
  `Description` text DEFAULT NULL,
  `PageText` int(10) unsigned NOT NULL DEFAULT 0,
  `LanguageID` int(10) unsigned NOT NULL DEFAULT 0,
  `PageMaterial` int(10) unsigned NOT NULL DEFAULT 0,
  `StartQuest` int(10) unsigned NOT NULL DEFAULT 0,
  `LockID` int(10) unsigned NOT NULL DEFAULT 0,
  `Material` int(11) NOT NULL DEFAULT 0,
  `Sheath` int(10) unsigned NOT NULL DEFAULT 0,
  `RandomProperty` int(10) unsigned NOT NULL DEFAULT 0,
  `RandomSuffix` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemSet` int(10) unsigned NOT NULL DEFAULT 0,
  `Area` int(10) unsigned NOT NULL DEFAULT 0,
  `Map` int(10) unsigned NOT NULL DEFAULT 0,
  `BagFamily` int(10) unsigned NOT NULL DEFAULT 0,
  `TotemCategory` int(10) unsigned NOT NULL DEFAULT 0,
  `SocketColor1` int(10) unsigned NOT NULL DEFAULT 0,
  `SocketColor2` int(10) unsigned NOT NULL DEFAULT 0,
  `SocketColor3` int(10) unsigned NOT NULL DEFAULT 0,
  `SocketBonus` int(10) unsigned NOT NULL DEFAULT 0,
  `GemProperties` int(10) unsigned NOT NULL DEFAULT 0,
  `ArmorDamageModifier` float NOT NULL DEFAULT 0,
  `Duration` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemLimitCategory` int(10) unsigned NOT NULL DEFAULT 0,
  `HolidayID` int(10) unsigned NOT NULL DEFAULT 0,
  `StatScalingFactor` float NOT NULL DEFAULT 0,
  `CurrencySubstitutionID` int(10) unsigned NOT NULL DEFAULT 0,
  `CurrencySubstitutionCount` int(10) unsigned NOT NULL DEFAULT 0,
  `ItemNameDescriptionID` int(10) unsigned NOT NULL DEFAULT 0,
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
