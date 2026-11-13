-- Cata Guild Challenges port.
-- Weekly-capped per-guild completion counters, matching TrinityCore's real schema
-- (sql/updates/characters/4.3.4/custom_2017_12_21_00_characters.sql) exactly, including column
-- names and position (appended after guild-leveling's `todayExperience`).

ALTER TABLE `guild`
  ADD COLUMN `completedDungeonChallenges` INT UNSIGNED NOT NULL DEFAULT '0' AFTER `todayExperience`,
  ADD COLUMN `completedRaidChallenges` INT UNSIGNED NOT NULL DEFAULT '0' AFTER `completedDungeonChallenges`,
  ADD COLUMN `completedRatedBattlegroundChallenges` INT UNSIGNED NOT NULL DEFAULT '0' AFTER `completedRaidChallenges`;
