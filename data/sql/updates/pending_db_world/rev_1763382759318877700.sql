-- Disables the Must Die Quests from Archmage Lan'dalock
DELETE FROM `disables` WHERE `sourceType` = 1 AND `entry` IN (24579, 24580, 24581, 24582, 24583, 24584, 24585, 24586, 24587, 24588, 24589, 24590);
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(1, 24579, 0, '', '', "Sartharion Must Die!"),
(1, 24580, 0, '', '', "Anub'Rekhan Must Die!"),
(1, 24581, 0, '', '', "Noth the Plaguebringer Must Die!"),
(1, 24582, 0, '', '', "Instructor Razuvious Must Die!"),
(1, 24583, 0, '', '', "Patchwerk Must Die!"),
(1, 24584, 0, '', '', "Malygos Must Die!"),
(1, 24585, 0, '', '', "Flame Leviathan Must Die!"),
(1, 24586, 0, '', '', "Razorscale Must Die!"),
(1, 24587, 0, '', '', "Ignis the Furnace Master Must Die!"),
(1, 24588, 0, '', '', "XT-002 Deconstructor Must Die!"),
(1, 24589, 0, '', '', "Lord Jaraxxus Must Die!"),
(1, 24590, 0, '', '', "Lord Marrowgar Must Die!");

-- Removes Gossip (10061), Gossip and Quest Flags and Removes the Script (npc_archmage_landalock)
UPDATE `creature_template` SET `gossip_menu_id` = 0, `npcflag` = `npcflag` &~ 3, `ScriptName` = "" WHERE `entry` = 20735;
