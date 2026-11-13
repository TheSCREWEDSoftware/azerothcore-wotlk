-- Archaeology: dig sites and artifact projects (data comes from the TDB world database)
CREATE TABLE IF NOT EXISTS `archaeology_sites` (
  `Id` smallint unsigned NOT NULL,
  `MapId` smallint unsigned NOT NULL,
  `QuestPOI` smallint unsigned NOT NULL DEFAULT '0',
  `ReqSkill` smallint unsigned NOT NULL DEFAULT '0',
  `MinLevel` tinyint unsigned NOT NULL DEFAULT '0',
  `GameobjectEntry` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `archaeology_projects` (
  `Entry` smallint unsigned NOT NULL,
  `Branch` tinyint unsigned NOT NULL,
  `Rarity` tinyint unsigned NOT NULL DEFAULT '0',
  `ReqSkill` smallint unsigned NOT NULL DEFAULT '0',
  `Fragments` smallint unsigned NOT NULL DEFAULT '0',
  `Keystone` tinyint unsigned NOT NULL DEFAULT '0',
  `Spell` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`Entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
