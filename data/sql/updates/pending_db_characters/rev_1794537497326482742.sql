-- 4.3.4 void storage: up to 80 items per character, kept outside the inventory. itemId is the
-- server-wide void storage item id (ObjectMgr::GenerateVoidStorageItemId); randomPropertyId follows
-- AC's item_instance convention (< 0: ItemRandomSuffix.dbc, > 0: ItemRandomProperties.dbc).
CREATE TABLE IF NOT EXISTS `character_void_storage` (
  `itemId` bigint unsigned NOT NULL,
  `playerGuid` int unsigned NOT NULL,
  `itemEntry` int unsigned NOT NULL,
  `slot` tinyint unsigned NOT NULL,
  `creatorGuid` int unsigned NOT NULL DEFAULT '0',
  `randomPropertyId` int NOT NULL DEFAULT '0',
  `suffixFactor` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`itemId`),
  UNIQUE KEY `idx_player_slot` (`playerGuid`,`slot`),
  KEY `idx_player` (`playerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
