-- Archaeology: per-character dig sites, current projects and completed artifacts
CREATE TABLE IF NOT EXISTS `character_archaeology_sites` (
  `guid` int unsigned NOT NULL,
  `site` tinyint unsigned NOT NULL,
  `type` smallint unsigned NOT NULL DEFAULT '0',
  `finds` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`site`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `character_archaeology_projects` (
  `guid` int unsigned NOT NULL,
  `branch` tinyint unsigned NOT NULL,
  `project` smallint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`branch`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `character_archaeology_completed` (
  `guid` int unsigned NOT NULL,
  `project` smallint unsigned NOT NULL,
  `time` int unsigned NOT NULL DEFAULT '0',
  `count` int unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`guid`,`project`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
