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
UPDATE `groups` SET `icon1` = UNHEX(REPEAT('00', 16)), `icon2` = UNHEX(REPEAT('00', 16)), `icon3` = UNHEX(REPEAT('00', 16)), `icon4` = UNHEX(REPEAT('00', 16)),
    `icon5` = UNHEX(REPEAT('00', 16)), `icon6` = UNHEX(REPEAT('00', 16)), `icon7` = UNHEX(REPEAT('00', 16)), `icon8` = UNHEX(REPEAT('00', 16));
