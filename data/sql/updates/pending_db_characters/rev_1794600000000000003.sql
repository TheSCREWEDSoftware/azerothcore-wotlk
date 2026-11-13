-- 4.3.4 hunters keep up to five active pets, one per Call Pet spell
ALTER TABLE `character_pet`
  ADD COLUMN `CallSlot` tinyint unsigned NOT NULL DEFAULT '255' AFTER `abdata`;

-- the single WotLK active hunter pet (current or dismissed) becomes Call Pet 1
UPDATE `character_pet` SET `CallSlot` = 0 WHERE `PetType` = 1 AND (`slot` = 0 OR `slot` = 100);
