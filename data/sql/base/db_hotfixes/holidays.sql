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
-- Table structure for table `holidays`
--

DROP TABLE IF EXISTS `holidays`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `holidays` (
  `ID` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration1` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration2` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration3` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration4` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration5` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration6` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration7` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration8` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration9` int(10) unsigned NOT NULL DEFAULT 0,
  `Duration10` int(10) unsigned NOT NULL DEFAULT 0,
  `Date1` int(10) unsigned NOT NULL DEFAULT 0,
  `Date2` int(10) unsigned NOT NULL DEFAULT 0,
  `Date3` int(10) unsigned NOT NULL DEFAULT 0,
  `Date4` int(10) unsigned NOT NULL DEFAULT 0,
  `Date5` int(10) unsigned NOT NULL DEFAULT 0,
  `Date6` int(10) unsigned NOT NULL DEFAULT 0,
  `Date7` int(10) unsigned NOT NULL DEFAULT 0,
  `Date8` int(10) unsigned NOT NULL DEFAULT 0,
  `Date9` int(10) unsigned NOT NULL DEFAULT 0,
  `Date10` int(10) unsigned NOT NULL DEFAULT 0,
  `Date11` int(10) unsigned NOT NULL DEFAULT 0,
  `Date12` int(10) unsigned NOT NULL DEFAULT 0,
  `Date13` int(10) unsigned NOT NULL DEFAULT 0,
  `Date14` int(10) unsigned NOT NULL DEFAULT 0,
  `Date15` int(10) unsigned NOT NULL DEFAULT 0,
  `Date16` int(10) unsigned NOT NULL DEFAULT 0,
  `Region` int(10) unsigned NOT NULL DEFAULT 0,
  `Looping` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags1` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags2` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags3` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags4` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags5` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags6` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags7` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags8` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags9` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFlags10` int(10) unsigned NOT NULL DEFAULT 0,
  `HolidayNameID` int(10) unsigned NOT NULL DEFAULT 0,
  `HolidayDescriptionID` int(10) unsigned NOT NULL DEFAULT 0,
  `TextureFilename` text DEFAULT NULL,
  `Priority` int(10) unsigned NOT NULL DEFAULT 0,
  `CalendarFilterType` int(10) unsigned NOT NULL DEFAULT 0,
  `Flags` int(10) unsigned NOT NULL DEFAULT 0,
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
