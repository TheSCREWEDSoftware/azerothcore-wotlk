-- 447 gameobject templates carry the creature AI name 'SmartAI'; gameobjects only know 'SmartGameObjectAI', and the
-- unregistered name made the AI factory lookup return null (worldserver crash in FactorySelector::SelectGameObjectAI
-- as soon as such an object was loaded). Objects with gameobject SmartAI scripts get SmartGameObjectAI, the rest none.
UPDATE `gameobject_template` AS `gt`
SET `gt`.`AIName` = IF(EXISTS (SELECT 1 FROM `smart_scripts` AS `ss` WHERE `ss`.`entryorguid` = `gt`.`entry` AND `ss`.`source_type` = 1), 'SmartGameObjectAI', '')
WHERE `gt`.`AIName` = 'SmartAI';
