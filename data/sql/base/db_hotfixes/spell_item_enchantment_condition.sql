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
-- Table structure for table `spell_item_enchantment_condition`
--

DROP TABLE IF EXISTS `spell_item_enchantment_condition`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `spell_item_enchantment_condition` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `LTOperandType1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `LTOperandType2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `LTOperandType3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `LTOperandType4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `LTOperandType5` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `LTOperand1` int(10) unsigned NOT NULL DEFAULT 0,
  `LTOperand2` int(10) unsigned NOT NULL DEFAULT 0,
  `LTOperand3` int(10) unsigned NOT NULL DEFAULT 0,
  `LTOperand4` int(10) unsigned NOT NULL DEFAULT 0,
  `LTOperand5` int(10) unsigned NOT NULL DEFAULT 0,
  `Operator1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Operator2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Operator3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Operator4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Operator5` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperandType1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperandType2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperandType3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperandType4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperandType5` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `RTOperand1` int(10) unsigned NOT NULL DEFAULT 0,
  `RTOperand2` int(10) unsigned NOT NULL DEFAULT 0,
  `RTOperand3` int(10) unsigned NOT NULL DEFAULT 0,
  `RTOperand4` int(10) unsigned NOT NULL DEFAULT 0,
  `RTOperand5` int(10) unsigned NOT NULL DEFAULT 0,
  `Logic1` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Logic2` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Logic3` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Logic4` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `Logic5` tinyint(3) unsigned NOT NULL DEFAULT 0,
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
