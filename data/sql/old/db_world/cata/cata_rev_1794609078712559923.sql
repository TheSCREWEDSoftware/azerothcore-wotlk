-- 4.3.4 anim kits on creature spawns and templates (TrinityCore 4.3.4 creature_addon / creature_template_addon)
ALTER TABLE `creature_addon`
    ADD COLUMN `aiAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `auras`,
    ADD COLUMN `movementAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `aiAnimKit`,
    ADD COLUMN `meleeAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `movementAnimKit`;

ALTER TABLE `creature_template_addon`
    ADD COLUMN `aiAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `auras`,
    ADD COLUMN `movementAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `aiAnimKit`,
    ADD COLUMN `meleeAnimKit` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `movementAnimKit`;
