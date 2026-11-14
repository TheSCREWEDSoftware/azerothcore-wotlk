/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19  Distrib 10.11.14-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: acore_world_548
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
-- Table structure for table `playercreateinfo_spell_custom`
--

DROP TABLE IF EXISTS `playercreateinfo_spell_custom`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `playercreateinfo_spell_custom` (
  `racemask` int(10) unsigned NOT NULL DEFAULT 0,
  `classmask` int(10) unsigned NOT NULL DEFAULT 0,
  `Spell` int(10) unsigned NOT NULL DEFAULT 0,
  `Note` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`racemask`,`classmask`,`Spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `playercreateinfo_spell_custom`
--

LOCK TABLES `playercreateinfo_spell_custom` WRITE;
/*!40000 ALTER TABLE `playercreateinfo_spell_custom` DISABLE KEYS */;
INSERT INTO `playercreateinfo_spell_custom` VALUES
(0,512,100780,'Jab'),
(0,623,196,'One-Handed Axes'),
(0,943,201,'One-Handed Swords'),
(0,1575,200,'Polearms'),
(0,1613,15590,'Fist Weapons'),
(0,1647,9077,'Leather'),
(0,1659,198,'One-Handed Maces'),
(0,2005,227,'Staves'),
(1,959,20597,'Sword Specialization'),
(1,959,20598,'The Human Spirit'),
(1,959,20599,'Diplomacy'),
(1,959,20864,'Mace Specialization'),
(1,959,59752,'Every Man for Himself'),
(1,959,79738,'Languages'),
(2,576,33697,'Blood Fury'),
(2,1005,20573,'Hardiness'),
(2,1005,20574,'Axe Specialization'),
(2,1005,79743,'Languages'),
(4,1023,672,'Language Dwarven'),
(4,1023,20594,'Stoneform'),
(4,1023,20595,'Gun Specialization'),
(4,1023,20596,'Frost Resistance'),
(4,1023,59224,'Mace Specialization'),
(4,1023,79739,'Languages (Racial Passive)'),
(4,1023,92682,'Explorer'),
(8,1725,671,'Language Darnassian'),
(8,1725,20582,'Quickness'),
(8,1725,20583,'Nature Resistance'),
(8,1725,20585,'Wisp Spirit'),
(8,1725,21009,'Elusiveness'),
(8,1725,58984,'Shadowmeld'),
(8,1725,76252,'Languages (Racial Passive)'),
(16,573,17737,'Language Gutterspeak'),
(16,573,79747,'Languages (Racial Passive)'),
(16,957,5227,'Underwater Breathing'),
(16,957,7744,'Will of the Forsaken'),
(16,957,20577,'Cannibalize'),
(16,957,20579,'Shadow Resistance'),
(32,1655,670,'Language Taurahe'),
(32,1655,20549,'War Stomp'),
(32,1655,20550,'Endurance'),
(32,1655,20551,'Nature Resistance'),
(32,1655,20552,'Cultivation'),
(32,1655,79746,'Languages (Racial Passive)'),
(64,953,7340,'Language Gnomish'),
(64,953,20589,'Escape Artist'),
(64,953,20591,'Expansive Mind'),
(64,953,20592,'Arcane Resistance'),
(64,953,20593,'Engineering Specialization'),
(64,953,79740,'Languages (Racial Passive)'),
(64,953,92680,'Shortblade Specialization'),
(128,2045,7341,'Language Troll'),
(128,2045,20555,'Regeneration'),
(128,2045,20557,'Beast Slaying'),
(128,2045,26290,'Bow Specialization'),
(128,2045,26297,'Berserking'),
(128,2045,58943,'Da Voodoo Shuffle'),
(128,2045,79744,'Languages (Racial Passive)'),
(256,1021,69041,'Rocket Barrage'),
(256,1021,69042,'Time is Money'),
(256,1021,69044,'Best Deals Anywhere'),
(256,1021,69045,'Better Living Through Chemistry'),
(256,1021,69046,'Pack Hobgoblin'),
(256,1021,69070,'Rocket Jump'),
(256,1021,69269,'Language Goblin'),
(256,1021,79749,'Languages (Racial Passive)'),
(512,512,129597,'Arcane Torrent'),
(512,959,813,'Language Thalassian'),
(512,959,822,'Arcane Resistance'),
(512,959,28877,'Arcane Affinity'),
(512,959,79748,'Languages (Racial Passive)'),
(1024,512,121093,'Gift of the Naaru'),
(1024,512,132295,'Shadow Resistance'),
(1024,551,6562,'Heroic Presence'),
(1024,759,28875,'Gemcutting'),
(1024,759,29932,'Language Draenei'),
(1024,759,79741,'Languages (Racial Passive)'),
(8388608,4,982,'Revive Pet'),
(8388608,4,3044,'Arcane Shot'),
(8388608,8,1752,'Sinister Strike'),
(8388608,16,585,'Smite'),
(8388608,64,403,'Lightning Bolt'),
(8388608,733,107072,'Epicurean'),
(8388608,733,107073,'Gourmand Gourmand'),
(8388608,733,107074,'Inner Peace'),
(8388608,733,107076,'Bouncy'),
(8388608,733,107079,'Bouncy'),
(8388608,733,108127,'Language Pandaren Neutral'),
(8388608,733,131701,'Languages (Racial Passive)'),
(10487775,128,44614,'Frostfire Bolt'),
(16777216,733,108130,'Language Pandaren Alliance'),
(16777216,733,143368,'Languages (Racial Passive)'),
(18875469,2047,668,'Language Common'),
(33554432,733,108131,'Language Pandaren Horde'),
(33554432,733,143369,'Languages (Racial Passive)'),
(33555378,2047,669,'Language Orcish');
/*!40000 ALTER TABLE `playercreateinfo_spell_custom` ENABLE KEYS */;
UNLOCK TABLES;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-11-14  1:24:19
