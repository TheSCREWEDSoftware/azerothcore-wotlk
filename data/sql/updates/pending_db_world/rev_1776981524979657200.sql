-- Do not touch the NPC flag after rewarding the quest only play RP
-- Apothecary Lysander - On Data Set 2 2 - Set Npc Flags Gossip & Questgiver
DELETE FROM `smart_scripts` WHERE (`entryorguid` = 24126) AND (`source_type` = 0) AND (`id` IN (2));
-- Apothecary Lysander - Actionlist - Set Npc Flag
DELETE FROM `smart_scripts` WHERE (`entryorguid` = 2412600) AND (`source_type` = 9) AND (`id` IN (1));
