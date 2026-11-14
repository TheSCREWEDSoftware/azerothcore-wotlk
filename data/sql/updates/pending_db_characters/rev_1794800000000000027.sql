-- 6.x PLAYER_FLAGS_EX (reagent bank unlock), as in TC 7.3.5 `characters`.`playerFlagsEx`
ALTER TABLE `characters` ADD COLUMN `playerFlagsEx` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `playerFlags`;
