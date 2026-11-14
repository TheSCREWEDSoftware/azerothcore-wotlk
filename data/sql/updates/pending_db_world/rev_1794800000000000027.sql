-- CONDITION_PLAYER_QUEUED_RANDOM_DUNGEON (105) is AzerothCore only; its difficulty (ConditionValue2)
-- was the legacy dungeon mode (0 normal, 1 heroic). 6.x maps use the Difficulty.dbc ids (1 normal, 2 heroic).
-- CONDITION_DIFFICULTY_ID (49) is left alone: TDB 6.x rows already carry 6.x ids.
UPDATE `conditions` SET `ConditionValue2` = `ConditionValue2` + 1
WHERE `ConditionTypeOrReference` = 105 AND `ConditionValue1` <> 0 AND `ConditionValue2` IN (0, 1);
