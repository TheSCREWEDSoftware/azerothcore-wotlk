-- 6.x item instance data: transmogrification, item upgrades, enchant illusions, caged battle pets and item bonuses
ALTER TABLE `item_instance`
    ADD COLUMN `transmogrification` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `text`,
    ADD COLUMN `upgradeId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `transmogrification`,
    ADD COLUMN `enchantIllusion` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `upgradeId`,
    ADD COLUMN `battlePetSpeciesId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `enchantIllusion`,
    ADD COLUMN `battlePetBreedData` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `battlePetSpeciesId`,
    ADD COLUMN `battlePetLevel` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `battlePetBreedData`,
    ADD COLUMN `battlePetDisplayId` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `battlePetLevel`,
    ADD COLUMN `bonusListIDs` TEXT AFTER `battlePetDisplayId`;
