-- 6.x garrisons (TC 6.x character tables; AzerothCore keeps 32-bit character guids and no foreign keys)
DROP TABLE IF EXISTS `character_garrison`;
CREATE TABLE `character_garrison` (
  `guid` int unsigned NOT NULL,
  `siteLevelId` int unsigned NOT NULL DEFAULT '0',
  `followerActivationsRemainingToday` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `character_garrison_blueprints`;
CREATE TABLE `character_garrison_blueprints` (
  `guid` int unsigned NOT NULL,
  `buildingId` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`buildingId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `character_garrison_buildings`;
CREATE TABLE `character_garrison_buildings` (
  `guid` int unsigned NOT NULL,
  `plotInstanceId` int unsigned NOT NULL DEFAULT '0',
  `buildingId` int unsigned NOT NULL DEFAULT '0',
  `timeBuilt` bigint unsigned NOT NULL,
  `active` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`plotInstanceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `character_garrison_follower_abilities`;
DROP TABLE IF EXISTS `character_garrison_followers`;
CREATE TABLE `character_garrison_followers` (
  `dbId` bigint unsigned NOT NULL,
  `guid` int unsigned NOT NULL,
  `followerId` int unsigned NOT NULL,
  `quality` int unsigned NOT NULL DEFAULT '2',
  `level` int unsigned NOT NULL DEFAULT '90',
  `itemLevelWeapon` int unsigned NOT NULL DEFAULT '600',
  `itemLevelArmor` int unsigned NOT NULL DEFAULT '600',
  `xp` int unsigned NOT NULL DEFAULT '0',
  `currentBuilding` int unsigned NOT NULL DEFAULT '0',
  `currentMission` int unsigned NOT NULL DEFAULT '0',
  `status` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`dbId`),
  UNIQUE KEY `idx_guid_id` (`guid`,`followerId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `character_garrison_follower_abilities` (
  `dbId` bigint unsigned NOT NULL,
  `abilityId` int unsigned NOT NULL,
  `slot` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`dbId`,`abilityId`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
