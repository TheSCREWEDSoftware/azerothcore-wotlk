-- Draenor content that TrinityCore master added after 6.x
-- (TC master 2022_08_22_00_world_quest_34586, 2025_02_01_02_world, 2025_08_31_03_world)

-- Garrison intro: Baros Alexston (Shadowmoon Valley, Alliance) and the Master Surveyor (Frostfire Ridge, Horde)
UPDATE `creature_template` SET `ScriptName` = 'npc_baros_alexston' WHERE `entry` = 79243;
UPDATE `gameobject_template` SET `ScriptName` = 'go_master_surveyor' WHERE `entry` = 233664;
UPDATE `gossip_menu_option` SET `OptionType` = 1, `OptionNpcFlag` = 1 WHERE `MenuID` = 16871 AND `OptionID` = 0;

DELETE FROM `conditions` WHERE (`SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 16871 AND `SourceEntry` = 0 AND `SourceId` = 0) OR (`SourceTypeOrReferenceId` = 13 AND `SourceGroup` = 1 AND `SourceEntry` = 160938);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 16871, 0, 0, 0, 47, 0, 34586, 8, 0, 0, 0, 0, '', 'Show gossip option if player has Establish Your Garrison (34586) in state incomplete'),
(13, 1, 160938, 0, 0, 31, 0, 3, 79655, 0, 0, 0, 0, '', 'Spell Despawn All Summons (Garrison Intro Only) can hit Vindicator Maraad'),
(13, 1, 160938, 0, 1, 31, 0, 3, 79635, 0, 0, 0, 0, '', 'Spell Despawn All Summons (Garrison Intro Only) can hit Prophet Velen'),
(13, 1, 160938, 0, 2, 31, 0, 3, 79656, 0, 0, 0, 0, '', 'Spell Despawn All Summons (Garrison Intro Only) can hit Yrel');

DELETE FROM `creature_text` WHERE `CreatureID` = 79243;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(79243, 0, 0, 'Let me know when you are ready to start construction, commander.', 12, 0, 100, 1, 0, 0, 81887, 0, 'Baros Alexston to Player - On Quest Accept 34586');

-- Auchindoun
DELETE FROM `instance_template` WHERE `map` = 1182;
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`) VALUES
(1182, 0, 'instance_auchindoun', 0);

UPDATE `creature_template` SET `ScriptName` = 'npc_auchindoun_soulbinder_tuulani' WHERE `entry` = 79248;
UPDATE `creature_template` SET `ScriptName` = 'npc_auchindoun_auchenai_defender' WHERE `entry` = 77693;

DELETE FROM `creature_text` WHERE `CreatureID` IN (77693, 79248);
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(77693, 0, 0, 'Nyami awaits you in the western ritual chamber, champions.', 12, 0, 100, 0, 0, 0, 83111, 0, 'Auchenai Defender to Player'),
(79248, 0, 0, 'Good! We must have arrived before Teron''gor and Gul''dan!', 12, 0, 100, 3, 0, 44644, 81528, 0, 'Soulbinder Tuulani to Player'),
(79248, 1, 0, 'Nyami is working to bolster the defenses against their assault - follow me, we will report to her.', 12, 0, 100, 1, 0, 44654, 81529, 0, 'Soulbinder Tuulani to Player'),
(79248, 2, 0, 'Beautiful, isn''t it? Just think - you''re one of the first outsiders to see inside Auchindoun.', 12, 0, 100, 6, 0, 44655, 81530, 0, 'Soulbinder Tuulani to Player'),
(79248, 3, 0, 'Strange... why is the door sealed? No matter...', 12, 0, 100, 6, 0, 44656, 81531, 0, 'Soulbinder Tuulani to Player'),
(79248, 4, 0, 'Let us proceed.', 12, 0, 100, 25, 0, 44657, 81532, 0, 'Soulbinder Tuulani'),
(79248, 5, 0, 'The Vindicators and Exarchs of the past. Heroes all - their souls still remain with us here.', 12, 0, 100, 25, 0, 44658, 81533, 0, 'Soulbinder Tuulani');

-- Soulbinder Tuulani paths; the points keep master's 0 based node ids, which the script uses
DELETE FROM `waypoint_data` WHERE `id` IN (7924800, 7924801);
INSERT INTO `waypoint_data` (`id`, `point`, `position_x`, `position_y`, `position_z`, `orientation`, `delay`, `move_type`) VALUES
(7924800, 0, 1581.05, 2953.418, 35.2387, NULL, 0, 1),
(7924800, 1, 1599.741, 2952.971, 35.23116, NULL, 0, 1),
(7924800, 2, 1617.514, 2953.21, 35.22284, NULL, 6486, 1),
(7924800, 3, 1649.21, 2953.6, 34.80952, NULL, 0, 1),
(7924800, 4, 1654.767, 2960.552, 34.27824, NULL, 0, 1),
(7924800, 5, 1658.876, 2968.664, 35.31583, NULL, 0, 1),
(7924800, 6, 1660.462, 2981.102, 34.7626, NULL, 0, 1),
(7924800, 7, 1663.101, 2997.4, 34.76278, NULL, 0, 1),
(7924800, 8, 1664.323, 3002.953, 34.89674, NULL, 0, 1),
(7924801, 0, 1666.299, 3032.361, 35.15017, NULL, 0, 1),
(7924801, 1, 1677.693, 3045.896, 35.1377, NULL, 0, 1),
(7924801, 2, 1681.016, 3064.814, 35.13763, NULL, 0, 1),
(7924801, 3, 1683.851, 3078.6, 35.13764, NULL, 0, 1),
(7924801, 4, 1694.418, 3081.378, 35.13764, NULL, 0, 1),
(7924801, 5, 1697.734, 3086.925, 35.13764, NULL, 7960, 1),
(7924801, 6, 1718.639, 3116.199, 35.13764, NULL, 0, 1),
(7924801, 7, 1728.587, 3128.338, 35.13763, NULL, 0, 1),
(7924801, 8, 1747.014, 3144.337, 35.13762, NULL, 0, 1),
(7924801, 9, 1775.142, 3162.625, 35.13763, NULL, 0, 1),
(7924801, 10, 1789.036, 3174.038, 35.13765, NULL, 0, 1),
(7924801, 11, 1798.358, 3179.212, 35.13763, NULL, 0, 1),
(7924801, 12, 1806.295, 3182.313, 35.11731, 3.70009, 0, 1);

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 160415;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 160415, 0, 2, 31, 0, 3, 79417, 0, 0, 0, 0, '', 'Spell Tuulani Unlock Visual can only hit Invisible Stalker');

DELETE FROM `spell_script_names` WHERE `spell_id` = 157762;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(157762, 'spell_auchindoun_halo');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (9973, 10072, 9974, 10280);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(9973, 'at_auchindoun_entrance'),
(10072, 'at_auchindoun_auchenai_defender_intro'),
(9974, 'at_auchindoun_barrier'),
(10280, 'at_auchindoun_soulbinder_nyami_scene');
