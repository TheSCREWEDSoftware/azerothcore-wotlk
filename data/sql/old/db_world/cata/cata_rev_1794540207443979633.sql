-- 4.3.4 guild leveling: XP needed to go from guild level `lvl` to `lvl` + 1 (TrinityCore TDB 434,
-- sql/old/4.3.4/TDB01_to_TDB02_updates/world/017_2012_09_09_00_world_guild_xp_for_level.sql).
CREATE TABLE IF NOT EXISTS `guild_xp_for_level` (
  `lvl` tinyint unsigned NOT NULL,
  `xp_for_next_level` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`lvl`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `guild_xp_for_level` WHERE `lvl` BETWEEN 1 AND 25;
INSERT INTO `guild_xp_for_level` (`lvl`, `xp_for_next_level`) VALUES
(1, 16580000),
(2, 18240000),
(3, 19900000),
(4, 21550000),
(5, 23220000),
(6, 24880000),
(7, 26530000),
(8, 28190000),
(9, 29850000),
(10, 31510000),
(11, 33170000),
(12, 34820000),
(13, 36490000),
(14, 38140000),
(15, 39800000),
(16, 41450000),
(17, 43120000),
(18, 44780000),
(19, 46430000),
(20, 48090000),
(21, 49750000),
(22, 51410000),
(23, 53060000),
(24, 54730000),
(25, 56390000);
