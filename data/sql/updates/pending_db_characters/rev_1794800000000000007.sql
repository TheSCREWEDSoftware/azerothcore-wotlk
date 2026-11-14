-- 6.x void storage keeps the item upgrade and bonus lists of stored items
ALTER TABLE `character_void_storage`
    ADD COLUMN `upgradeId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `suffixFactor`,
    ADD COLUMN `bonusListIDs` TEXT NULL AFTER `upgradeId`;
