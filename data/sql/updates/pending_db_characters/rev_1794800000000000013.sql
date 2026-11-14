-- 6.x: up to 32 spell effects per aura; effect amounts move to character_aura_effect / pet_aura_effect (TC 6.x)
DROP TABLE IF EXISTS `character_aura_effect`;
CREATE TABLE `character_aura_effect` (
  `guid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `casterGuid` binary(16) NOT NULL COMMENT 'Full Global Unique Identifier',
  `itemGuid` binary(16) NOT NULL,
  `spell` int unsigned NOT NULL,
  `effectMask` int unsigned NOT NULL,
  `effectIndex` tinyint unsigned NOT NULL,
  `amount` int NOT NULL DEFAULT '0',
  `baseAmount` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`casterGuid`,`itemGuid`,`spell`,`effectMask`,`effectIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Player System';

DROP TABLE IF EXISTS `pet_aura_effect`;
CREATE TABLE `pet_aura_effect` (
  `guid` int unsigned NOT NULL COMMENT 'Global Unique Identifier',
  `casterGuid` binary(16) NOT NULL COMMENT 'Full Global Unique Identifier',
  `spell` int unsigned NOT NULL,
  `effectMask` int unsigned NOT NULL,
  `effectIndex` tinyint unsigned NOT NULL,
  `amount` int NOT NULL DEFAULT '0',
  `baseAmount` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`casterGuid`,`spell`,`effectMask`,`effectIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Pet System';

DELETE FROM `character_aura_effect`;
INSERT INTO `character_aura_effect` (`guid`, `casterGuid`, `itemGuid`, `spell`, `effectMask`, `effectIndex`, `amount`, `baseAmount`)
SELECT `guid`, `casterGuid`, `itemGuid`, `spell`, `effectMask`, 0, `amount0`, `base_amount0` FROM `character_aura` WHERE `effectMask` & 1
UNION ALL
SELECT `guid`, `casterGuid`, `itemGuid`, `spell`, `effectMask`, 1, `amount1`, `base_amount1` FROM `character_aura` WHERE `effectMask` & 2
UNION ALL
SELECT `guid`, `casterGuid`, `itemGuid`, `spell`, `effectMask`, 2, `amount2`, `base_amount2` FROM `character_aura` WHERE `effectMask` & 4;

DELETE FROM `pet_aura_effect`;
INSERT INTO `pet_aura_effect` (`guid`, `casterGuid`, `spell`, `effectMask`, `effectIndex`, `amount`, `baseAmount`)
SELECT `guid`, `casterGuid`, `spell`, `effectMask`, 0, IFNULL(`amount0`, 0), IFNULL(`base_amount0`, 0) FROM `pet_aura` WHERE `effectMask` & 1
UNION ALL
SELECT `guid`, `casterGuid`, `spell`, `effectMask`, 1, IFNULL(`amount1`, 0), IFNULL(`base_amount1`, 0) FROM `pet_aura` WHERE `effectMask` & 2
UNION ALL
SELECT `guid`, `casterGuid`, `spell`, `effectMask`, 2, IFNULL(`amount2`, 0), IFNULL(`base_amount2`, 0) FROM `pet_aura` WHERE `effectMask` & 4;

ALTER TABLE `character_aura`
    MODIFY COLUMN `effectMask` int unsigned NOT NULL DEFAULT '0',
    MODIFY COLUMN `recalculateMask` int unsigned NOT NULL DEFAULT '0',
    DROP COLUMN `amount0`,
    DROP COLUMN `amount1`,
    DROP COLUMN `amount2`,
    DROP COLUMN `base_amount0`,
    DROP COLUMN `base_amount1`,
    DROP COLUMN `base_amount2`;

ALTER TABLE `pet_aura`
    MODIFY COLUMN `effectMask` int unsigned NOT NULL DEFAULT '0',
    MODIFY COLUMN `recalculateMask` int unsigned NOT NULL DEFAULT '0',
    DROP COLUMN `amount0`,
    DROP COLUMN `amount1`,
    DROP COLUMN `amount2`,
    DROP COLUMN `base_amount0`,
    DROP COLUMN `base_amount1`,
    DROP COLUMN `base_amount2`;
