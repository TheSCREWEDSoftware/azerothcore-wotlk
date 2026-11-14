-- TC master string ids: script tags on creature templates and spawns (HasStringId,
-- FindNearestCreatureWithOptions). TDB 6.x has none; scripts that need them do nothing until set.
ALTER TABLE `creature_template` ADD COLUMN `StringId` VARCHAR(64) DEFAULT NULL AFTER `type_flags2`;
ALTER TABLE `creature` ADD COLUMN `StringId` VARCHAR(64) DEFAULT NULL AFTER `PhaseGroup`;
