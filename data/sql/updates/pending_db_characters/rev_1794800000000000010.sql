-- 6.x pet specializations (ChrSpecialization.dbc, PetTalentType replaced by specs)
ALTER TABLE `character_pet` ADD COLUMN `specialization` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `CreatedBySpell`;
