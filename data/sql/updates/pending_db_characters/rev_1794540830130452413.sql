-- Cata Guild Reputation port .
-- Adds per-member weekly guild reputation tracking to `guild_member`, backing
-- Guild::Member::AddReputation's weekly cap (Guild.WeeklyReputationCap). Lifetime guild
-- reputation is not tracked as a separate column - it lives in the player's own
-- PLAYER_GUILD_REPUTATION (1168) faction standing in `character_reputation`, same as
-- TrinityCore.

ALTER TABLE `guild_member`
  ADD COLUMN `weekReputation` INT UNSIGNED NOT NULL DEFAULT '0' AFTER `weekActivity`;
