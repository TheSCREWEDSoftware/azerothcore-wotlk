-- 6.x quest objectives: progress is stored per objective StorageIndex (TrinityCore 6.x layout).
-- The mobcount/itemcount/playercount columns stay so the core can convert old progress
-- on the first login; character_queststatus rows saved afterwards no longer fill them.
DROP TABLE IF EXISTS `character_queststatus_objectives`;
CREATE TABLE `character_queststatus_objectives` (
  `guid` int unsigned NOT NULL DEFAULT '0',
  `quest` int unsigned NOT NULL DEFAULT '0',
  `objective` tinyint unsigned NOT NULL DEFAULT '0',
  `data` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`quest`,`objective`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Player System';
