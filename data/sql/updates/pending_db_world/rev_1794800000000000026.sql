-- 6.x: `npcflag` is 64-bit; the high half is UNIT_NPC_FLAGS + 1 (NPCFlags2: upgrade master,
-- garrison architect, shipment crafter, garrison mission npc, black market, ...), as in TDB 6.x
ALTER TABLE `creature_template` MODIFY COLUMN `npcflag` BIGINT UNSIGNED NOT NULL DEFAULT 0;
