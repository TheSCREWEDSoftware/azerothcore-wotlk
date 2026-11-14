--
-- Table structure for table `guild_challenges`
--
-- Cata Guild Challenges port. One row per
-- GuildChallengeType (1=Dungeon, 2=Raid, 3=Rated Battleground); ChallengeCount is the weekly
-- completion cap, Experience/Gold the sub-max-level reward, GoldMaxLevel the reward once the
-- guild has reached Guild.MaxLevel. Values are real data recovered from TrinityCore's own git
-- history (sql/updates/world/custom/custom_2019_10_14_00_world.sql), not invented.

CREATE TABLE IF NOT EXISTS `guild_challenges` (
  `ChallengeType` tinyint unsigned NOT NULL,
  `ChallengeCount` smallint unsigned NOT NULL DEFAULT '1',
  `Experience` int unsigned NOT NULL DEFAULT '0',
  `Gold` int unsigned NOT NULL DEFAULT '0',
  `GoldMaxLevel` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ChallengeType`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `guild_challenges` WHERE `ChallengeType` IN (1, 2, 3);
INSERT INTO `guild_challenges` (`ChallengeType`, `ChallengeCount`, `Experience`, `Gold`, `GoldMaxLevel`) VALUES
(1, 7, 300000, 125, 250),
(2, 1, 3000000, 500, 1000),
(3, 3, 1500000, 250, 500);
