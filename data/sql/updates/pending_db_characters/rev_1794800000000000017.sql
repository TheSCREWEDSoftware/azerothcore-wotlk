-- 6.x: criteria progress counters are 64-bit (TC 6.x criteria handler)
ALTER TABLE `character_achievement_progress` MODIFY `counter` bigint unsigned NOT NULL;
