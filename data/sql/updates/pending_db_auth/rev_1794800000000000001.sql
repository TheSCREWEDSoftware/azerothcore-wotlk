-- 6.2.4 21742 client and the Battle.net login (bnetserver), from TrinityCore 6.x sql/base/auth_database.sql
DELETE FROM `build_info` WHERE `build` = 21742;
INSERT INTO `build_info`
    (`build`, `majorVersion`, `minorVersion`, `bugfixVersion`, `hotfixVersion`, `winAuthSeed`, `win64AuthSeed`, `mac64AuthSeed`, `winChecksumSeed`, `macChecksumSeed`)
VALUES
    (21742, 6, 2, 4, NULL, NULL, NULL, NULL, NULL, NULL);

ALTER TABLE `realmlist` ALTER COLUMN `gamebuild` SET DEFAULT 21742;
UPDATE `realmlist` SET `gamebuild` = 21742 WHERE `gamebuild` = 18414;

-- Battle.net realm address: region << 24 | battlegroup << 16 | id
ALTER TABLE `realmlist`
    ADD COLUMN `Region` tinyint unsigned NOT NULL DEFAULT '1' AFTER `gamebuild`,
    ADD COLUMN `Battlegroup` tinyint unsigned NOT NULL DEFAULT '1' AFTER `Region`;

CREATE TABLE IF NOT EXISTS `battlenet_accounts` (
  `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `email` varchar(320) NOT NULL,
  `sha_pass_hash` varchar(64) NOT NULL DEFAULT '',
  `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` varchar(15) NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` int unsigned NOT NULL DEFAULT '0',
  `locked` tinyint unsigned NOT NULL DEFAULT '0',
  `lock_country` varchar(2) NOT NULL DEFAULT '00',
  `last_login` timestamp NULL DEFAULT NULL,
  `online` tinyint unsigned NOT NULL DEFAULT '0',
  `locale` tinyint unsigned NOT NULL DEFAULT '0',
  `os` varchar(4) NOT NULL DEFAULT '',
  `LastCharacterUndelete` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Battle.net accounts';

CREATE TABLE IF NOT EXISTS `battlenet_account_bans` (
  `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Battle.net account Id',
  `bandate` int unsigned NOT NULL DEFAULT '0',
  `unbandate` int unsigned NOT NULL DEFAULT '0',
  `bannedby` varchar(50) NOT NULL,
  `banreason` varchar(255) NOT NULL,
  PRIMARY KEY (`id`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Battle.net account bans';

CREATE TABLE IF NOT EXISTS `account_last_played_character` (
  `accountId` int unsigned NOT NULL,
  `region` tinyint unsigned NOT NULL,
  `battlegroup` tinyint unsigned NOT NULL,
  `realmId` int unsigned DEFAULT NULL,
  `characterName` varchar(12) DEFAULT NULL,
  `characterGUID` bigint unsigned DEFAULT NULL,
  `lastPlayedTime` int unsigned DEFAULT NULL,
  PRIMARY KEY (`accountId`,`region`,`battlegroup`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- game accounts belong to a Battle.net account; session_key_bnet is the 6.x world session key (JoinRealm);
-- the 6.x client reports its OS as Win, Wn64 or Mc64
ALTER TABLE `account`
    MODIFY COLUMN `os` varchar(4) NOT NULL DEFAULT '',
    ADD COLUMN `session_key_bnet` varbinary(64) DEFAULT NULL AFTER `session_key`,
    ADD COLUMN `battlenet_account` int unsigned DEFAULT NULL AFTER `totaltime`,
    ADD COLUMN `battlenet_index` tinyint unsigned DEFAULT NULL AFTER `battlenet_account`,
    ADD UNIQUE KEY `uk_bnet_acc` (`battlenet_account`, `battlenet_index`);
