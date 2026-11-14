-- 6.x: criteria data is keyed by Criteria.db2 and lives in `criteria_data` (TC 6.x).
-- Data types that TC 6.x dropped (they are ModifierTree conditions in 6.x) are not carried over:
-- 4 T_PLAYER_DEAD, 6 S_AREA, 12 MAP_DIFFICULTY, 22 NTH_BIRTHDAY, 24 BG_TEAMS_SCORES (24 is GAME_EVENT now), 25 S_ITEM_QUALITY
DROP TABLE IF EXISTS `criteria_data`;
CREATE TABLE `criteria_data` (
  `criteria_id` int NOT NULL,
  `type` tinyint unsigned NOT NULL DEFAULT '0',
  `value1` int unsigned NOT NULL DEFAULT '0',
  `value2` int unsigned NOT NULL DEFAULT '0',
  `ScriptName` char(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`criteria_id`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Achievment system';

DELETE FROM `criteria_data`;
INSERT INTO `criteria_data` (`criteria_id`, `type`, `value1`, `value2`, `ScriptName`)
SELECT `criteria_id`, `type`, `value1`, `value2`, `ScriptName` FROM `achievement_criteria_data` WHERE `type` NOT IN (4, 6, 12, 22, 24, 25);

DROP TABLE IF EXISTS `achievement_criteria_data`;
