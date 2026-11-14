-- 6.x: gameobject_template has 33 data fields and RequiredLevel (unkInt32) as in TC 6.x
DROP PROCEDURE IF EXISTS `acore_go_template_6x`;
DELIMITER //
CREATE PROCEDURE `acore_go_template_6x`()
BEGIN
    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'gameobject_template' AND `COLUMN_NAME` = 'Data24') THEN
        ALTER TABLE `gameobject_template`
            ADD COLUMN `Data24` int NOT NULL DEFAULT '0' AFTER `Data23`,
            ADD COLUMN `Data25` int NOT NULL DEFAULT '0' AFTER `Data24`,
            ADD COLUMN `Data26` int NOT NULL DEFAULT '0' AFTER `Data25`,
            ADD COLUMN `Data27` int NOT NULL DEFAULT '0' AFTER `Data26`,
            ADD COLUMN `Data28` int NOT NULL DEFAULT '0' AFTER `Data27`,
            ADD COLUMN `Data29` int NOT NULL DEFAULT '0' AFTER `Data28`,
            ADD COLUMN `Data30` int NOT NULL DEFAULT '0' AFTER `Data29`,
            ADD COLUMN `Data31` int NOT NULL DEFAULT '0' AFTER `Data30`,
            ADD COLUMN `Data32` int NOT NULL DEFAULT '0' AFTER `Data31`;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'gameobject_template' AND `COLUMN_NAME` = 'unkInt32') THEN
        ALTER TABLE `gameobject_template` ADD COLUMN `unkInt32` int NOT NULL DEFAULT '0' AFTER `Data32`;
    END IF;
END //
DELIMITER ;
CALL `acore_go_template_6x`();
DROP PROCEDURE IF EXISTS `acore_go_template_6x`;

-- destructible building health (TC master layout), referenced by gameobject_template Data2 (HealthRec) of type 33
DROP TABLE IF EXISTS `destructible_hitpoint`;
CREATE TABLE `destructible_hitpoint` (
  `Id` int unsigned NOT NULL,
  `IntactNumHits` int unsigned NOT NULL DEFAULT '0',
  `DamagedNumHits` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
