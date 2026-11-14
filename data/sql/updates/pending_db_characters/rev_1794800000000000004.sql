-- 6.x 128 bit ObjectGuid: full guids are stored as binary(16) (low half then high half, little endian)
-- Old 64 bit values cannot be converted, auras and raid target icons are transient so they are dropped
DELETE FROM `character_aura`;
ALTER TABLE `character_aura`
    MODIFY COLUMN `casterGuid` binary(16) NOT NULL COMMENT 'Full Global Unique Identifier',
    MODIFY COLUMN `itemGuid` binary(16) NOT NULL;

DELETE FROM `pet_aura`;
ALTER TABLE `pet_aura`
    MODIFY COLUMN `casterGuid` binary(16) NOT NULL COMMENT 'Full Global Unique Identifier';

ALTER TABLE `groups`
    MODIFY COLUMN `icon1` binary(16) NOT NULL,
    MODIFY COLUMN `icon2` binary(16) NOT NULL,
    MODIFY COLUMN `icon3` binary(16) NOT NULL,
    MODIFY COLUMN `icon4` binary(16) NOT NULL,
    MODIFY COLUMN `icon5` binary(16) NOT NULL,
    MODIFY COLUMN `icon6` binary(16) NOT NULL,
    MODIFY COLUMN `icon7` binary(16) NOT NULL,
    MODIFY COLUMN `icon8` binary(16) NOT NULL;
UPDATE `groups` SET `icon1` = 0x00000000000000000000000000000000, `icon2` = 0x00000000000000000000000000000000, `icon3` = 0x00000000000000000000000000000000, `icon4` = 0x00000000000000000000000000000000,
    `icon5` = 0x00000000000000000000000000000000, `icon6` = 0x00000000000000000000000000000000, `icon7` = 0x00000000000000000000000000000000, `icon8` = 0x00000000000000000000000000000000;
