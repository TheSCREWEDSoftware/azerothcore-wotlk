-- Compact unit frame (raid frame) profiles saved by the client
CREATE TABLE IF NOT EXISTS `character_cuf_profiles` (
  `guid` int unsigned NOT NULL COMMENT 'Character Guid',
  `id` tinyint unsigned NOT NULL COMMENT 'Profile Id (0-4)',
  `name` varchar(12) NOT NULL COMMENT 'Profile Name',
  `frameHeight` smallint unsigned NOT NULL DEFAULT '0' COMMENT 'Profile Frame Height',
  `frameWidth` smallint unsigned NOT NULL DEFAULT '0' COMMENT 'Profile Frame Width',
  `sortBy` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Frame Sort By',
  `healthText` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Frame Health Text',
  `boolOptions` int unsigned NOT NULL DEFAULT '0' COMMENT 'Many Configurable Bool Options',
  `unk146` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Top Point',
  `unk147` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Bottom Point',
  `unk148` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Left Point',
  `unk150` smallint unsigned NOT NULL DEFAULT '0' COMMENT 'Top Offset',
  `unk152` smallint unsigned NOT NULL DEFAULT '0' COMMENT 'Bottom Offset',
  `unk154` smallint unsigned NOT NULL DEFAULT '0' COMMENT 'Left Offset',
  PRIMARY KEY (`guid`, `id`),
  KEY `index` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
