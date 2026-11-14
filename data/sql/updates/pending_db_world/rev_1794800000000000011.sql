-- 6.x: quest_poi uses the TrinityCore 6.x layout (blob index, objective and object ids, world effect, player condition)
ALTER TABLE `quest_poi`
    CHANGE COLUMN `id` `Idx1` int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN `BlobIndex` int(11) NOT NULL DEFAULT 0 AFTER `QuestID`,
    ADD COLUMN `QuestObjectiveID` int(11) NOT NULL DEFAULT 0 AFTER `ObjectiveIndex`,
    ADD COLUMN `QuestObjectID` int(11) NOT NULL DEFAULT 0 AFTER `QuestObjectiveID`,
    ADD COLUMN `WorldEffectID` int(11) NOT NULL DEFAULT 0 AFTER `Flags`,
    ADD COLUMN `PlayerConditionID` int(11) NOT NULL DEFAULT 0 AFTER `WorldEffectID`,
    ADD COLUMN `WoDUnk1` int(11) NOT NULL DEFAULT 0 AFTER `PlayerConditionID`,
    DROP PRIMARY KEY,
    DROP KEY `idx`;

-- the 5.4.8 rows had one blob per row, the old id was both the blob and the points index
UPDATE `quest_poi` SET `BlobIndex` = `Idx1`;

ALTER TABLE `quest_poi`
    ADD PRIMARY KEY (`QuestID`, `BlobIndex`, `Idx1`),
    ADD KEY `idx` (`QuestID`, `BlobIndex`);
