-- 6.x keeps transmogrification in the item modifiers (`item_instance`.`transmogrification`).
-- A 5.4.8 database kept the display item entry in enchantment slot 9 (the 28th number of `enchantments`);
-- move it over. Reforges (slot 8) are ignored by the 6.x core (reforging was removed).
UPDATE `item_instance`
SET `transmogrification` = CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(`enchantments`, ' ', 28), ' ', -1) AS UNSIGNED)
WHERE `transmogrification` = 0
  AND (LENGTH(`enchantments`) - LENGTH(REPLACE(`enchantments`, ' ', ''))) >= 27
  AND SUBSTRING_INDEX(SUBSTRING_INDEX(`enchantments`, ' ', 28), ' ', -1) REGEXP '^[0-9]+$';
