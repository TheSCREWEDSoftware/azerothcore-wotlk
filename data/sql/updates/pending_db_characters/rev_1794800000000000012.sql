-- 6.x difficulty model (TrinityCore 6.x): dungeon, raid (normal/heroic/mythic) and legacy raid (10/25) difficulties
-- the old instance_mode_mask kept the dungeon difficulty in the low nibble and the 10/25 raid difficulty in the high one
ALTER TABLE `characters`
    ADD COLUMN `dungeonDifficulty` tinyint unsigned NOT NULL DEFAULT '1' AFTER `instance_mode_mask`,
    ADD COLUMN `raidDifficulty` tinyint unsigned NOT NULL DEFAULT '14' AFTER `dungeonDifficulty`,
    ADD COLUMN `legacyRaidDifficulty` tinyint unsigned NOT NULL DEFAULT '3' AFTER `raidDifficulty`;

UPDATE `characters` SET
    `dungeonDifficulty` = IF((`instance_mode_mask` & 0x0F) IN (1, 2), `instance_mode_mask` & 0x0F, 1),
    `legacyRaidDifficulty` = IF(((`instance_mode_mask` >> 4) & 0x0F) BETWEEN 3 AND 6, (`instance_mode_mask` >> 4) & 0x0F, 3),
    `raidDifficulty` = 14;

ALTER TABLE `characters` DROP COLUMN `instance_mode_mask`;

ALTER TABLE `groups`
    ADD COLUMN `legacyRaidDifficulty` tinyint unsigned NOT NULL DEFAULT '3' AFTER `raidDifficulty`;

-- the old raidDifficulty was the 10/25 one
UPDATE `groups` SET
    `legacyRaidDifficulty` = IF(`raidDifficulty` BETWEEN 3 AND 6, `raidDifficulty`, 3),
    `raidDifficulty` = 14,
    `difficulty` = IF(`difficulty` IN (1, 2), `difficulty`, 1);
