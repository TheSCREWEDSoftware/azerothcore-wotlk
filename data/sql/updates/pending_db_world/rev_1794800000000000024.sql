-- 6.x: creature_template female name and second type flags word (TC 6.x columns)
DROP PROCEDURE IF EXISTS `acore_creature_template_6x`;
DELIMITER //
CREATE PROCEDURE `acore_creature_template_6x`()
BEGIN
    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'creature_template' AND `COLUMN_NAME` = 'femaleName') THEN
        ALTER TABLE `creature_template` ADD COLUMN `femaleName` char(100) NOT NULL DEFAULT '' AFTER `name`;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'creature_template' AND `COLUMN_NAME` = 'type_flags2') THEN
        ALTER TABLE `creature_template` ADD COLUMN `type_flags2` int unsigned NOT NULL DEFAULT '0' AFTER `type_flags`;
    END IF;
END //
DELIMITER ;
CALL `acore_creature_template_6x`();
DROP PROCEDURE IF EXISTS `acore_creature_template_6x`;
