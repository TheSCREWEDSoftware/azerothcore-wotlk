-- 6.x character undelete: time of the account's last restored character (TC 6.x keeps it on the battle.net account)
SET @col := (SELECT COUNT(*) FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'account' AND `COLUMN_NAME` = 'last_character_undelete');
SET @sql := IF(@col = 0, 'ALTER TABLE `account` ADD COLUMN `last_character_undelete` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `last_login`', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
