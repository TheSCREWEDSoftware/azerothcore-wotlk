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
-- Table structure for table `reputation_reward_rate`
--

DROP TABLE IF EXISTS `reputation_reward_rate`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `reputation_reward_rate` (
  `faction` int(10) unsigned NOT NULL DEFAULT 0,
  `quest_rate` float NOT NULL DEFAULT 1,
  `quest_daily_rate` float NOT NULL DEFAULT 1,
  `quest_weekly_rate` float NOT NULL DEFAULT 1,
  `quest_monthly_rate` float NOT NULL DEFAULT 1,
  `quest_repeatable_rate` float NOT NULL DEFAULT 1,
  `creature_rate` float NOT NULL DEFAULT 1,
  `spell_rate` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `reputation_reward_rate`
--

LOCK TABLES `reputation_reward_rate` WRITE;
/*!40000 ALTER TABLE `reputation_reward_rate` DISABLE KEYS */;
INSERT INTO `reputation_reward_rate` VALUES
(529,1,1,1,1,1,1,2),
(576,4,4,1,1,4,1,1),
(609,1,1,1,1,1,1,2),
(941,1,1,1,1,1,1,1),
(970,1,1,1,1,1,1,3),
(978,1,1,1,1,1,1,1),
(1037,1,1,1,1,1,1.3,1.3),
(1052,1,1,1,1,1,1.3,1.3),
(1073,1,2,1,1,1,2,2),
(1090,1.3,1.3,1.3,1.3,1.3,1.3,1.3),
(1091,1.3,1.3,1.3,1.3,1.3,1.3,1.3),
(1098,1.3,1.3,1.3,1.3,1.3,1.3,1.3),
(1104,1,2,1,1,1,2,2),
(1105,1,2,1,1,1,2,2),
(1106,1.3,1.3,1.3,1.3,1.3,1.3,1.3),
(1119,1.3,1.3,1.3,1.3,1.3,1.3,1.3);
/*!40000 ALTER TABLE `reputation_reward_rate` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:20:30
