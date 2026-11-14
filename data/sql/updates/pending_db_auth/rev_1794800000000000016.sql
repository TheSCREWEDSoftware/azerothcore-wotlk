-- 6.x battle pet journal, account wide (TrinityCore 6.x keeps it per battle.net account)
CREATE TABLE IF NOT EXISTS `battle_pets` (
  `guid` bigint NOT NULL,
  `battlenetAccountId` int NOT NULL,
  `species` int NOT NULL,
  `breed` smallint NOT NULL,
  `level` smallint NOT NULL DEFAULT '1',
  `exp` smallint NOT NULL DEFAULT '0',
  `health` int NOT NULL DEFAULT '1',
  `quality` tinyint NOT NULL DEFAULT '0',
  `flags` smallint NOT NULL DEFAULT '0',
  `name` varchar(12) NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `battle_pet_slots` (
  `id` tinyint NOT NULL,
  `battlenetAccountId` int NOT NULL,
  `battlePetGuid` bigint NOT NULL,
  `locked` tinyint NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`battlenetAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
