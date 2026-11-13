-- Guild news log
CREATE TABLE IF NOT EXISTS `guild_newslog` (
  `guildid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Guild Identificator',
  `LogGuid` int unsigned NOT NULL DEFAULT '0' COMMENT 'Log record identificator - auxiliary column',
  `EventType` tinyint unsigned NOT NULL DEFAULT '0' COMMENT 'Event type',
  `PlayerGuid` int unsigned NOT NULL DEFAULT '0',
  `Flags` int unsigned NOT NULL DEFAULT '0',
  `Value` int unsigned NOT NULL DEFAULT '0',
  `TimeStamp` int unsigned NOT NULL DEFAULT '0' COMMENT 'Event UNIX time',
  PRIMARY KEY (`guildid`, `LogGuid`),
  KEY `Idx_PlayerGuid` (`PlayerGuid`),
  KEY `Idx_TimeStamp` (`TimeStamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Guild News';
