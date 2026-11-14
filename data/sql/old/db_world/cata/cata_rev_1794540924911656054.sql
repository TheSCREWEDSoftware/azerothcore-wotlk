--
-- Table structure for table `guild_rewards`
--
-- Cata Guild Rewards port. Rep/gold-gated
-- mount/item rewards; `standing` is a ReputationRank (4=Friendly..7=Exalted) on the player's
-- own PLAYER_GUILD_REPUTATION faction, `racemask` -1 means all races, `price` is in copper,
-- `achievement` 0 means no achievement is required. Eligibility is computed client-side from
-- this catalog plus the player's own state - the server does not gate purchases on these
-- columns in this pass (see the porting doc for the redemption-flow scope cut). Values are
-- real Cata 4.3.4 launch data recovered from TrinityCore's own git history
-- (sql/old/4.3.4/TDB01_to_TDB02_updates/world/027_2012_09_09_05_world_guild_rewards_434.sql),
-- not invented.

CREATE TABLE IF NOT EXISTS `guild_rewards` (
  `entry` int unsigned NOT NULL DEFAULT '0',
  `standing` tinyint unsigned NOT NULL DEFAULT '0',
  `racemask` int NOT NULL DEFAULT '0',
  `price` bigint unsigned NOT NULL DEFAULT '0',
  `achievement` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `guild_rewards`;
INSERT INTO `guild_rewards` (`entry`, `standing`, `racemask`, `price`, `achievement`) VALUES
(69209, 4, -1, 1250000, 0),
(69210, 5, -1, 2500000, 0),
(63353, 5, 946, 1500000, 4989),
(63352, 5, 2098253, 1500000, 4989),
(63207, 5, 946, 3000000, 4945),
(63206, 5, 2098253, 3000000, 4945),
(65274, 6, 946, 5000000, 5035),
(65360, 6, 2098253, 5000000, 5035),
(65362, 5, 946, 3000000, 5179),
(65364, 6, 946, 5000000, 5201),
(65361, 5, 2098253, 3000000, 5031),
(65363, 6, 2098253, 5000000, 5201),
(63138, 7, -1, 3000000, 5812),
(63398, 6, 0, 3000000, 5144),
(63359, 5, 2098253, 1500000, 4860),
(64398, 5, 2098253, 2000000, 5143),
(64399, 5, 2098253, 3000000, 5422),
(64400, 5, 946, 1500000, 4860),
(64401, 5, 946, 2000000, 5143),
(64402, 5, 946, 3000000, 5422),
(62800, 5, 0, 1500000, 5036),
(62799, 5, 0, 1500000, 5467),
(65435, 5, 0, 1500000, 5465),
(65498, 5, 0, 1500000, 5024),
(62286, 4, 0, 100000000, 4943),
(68136, 6, 2098253, 200000000, 5152),
(62287, 6, 946, 200000000, 5158),
(69892, 5, 0, 12000000, 4944),
(62038, 5, 0, 12000000, 4944),
(62039, 5, 0, 12000000, 4944),
(62040, 5, 0, 12000000, 4944),
(69887, 5, 0, 15000000, 4946),
(61958, 5, 0, 15000000, 4946),
(61942, 5, 0, 15000000, 4946),
(61937, 5, 0, 15000000, 4946),
(61936, 5, 0, 15000000, 4946),
(61935, 5, 0, 15000000, 4946),
(61931, 5, 0, 15000000, 4946),
(67107, 7, 946, 15000000, 5492),
(62298, 7, 2098253, 15000000, 4912),
(63125, 7, 0, 30000000, 4988),
(71033, 7, 0, 15000000, 5840);
