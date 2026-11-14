-- 6.x Black Market templates use the TrinityCore 6.x layout (keyed by market id, with item bonus lists)
DROP TABLE IF EXISTS `blackmarket_template_6x`;
CREATE TABLE `blackmarket_template_6x` (
  `marketId` int NOT NULL DEFAULT '0',
  `sellerNpc` int NOT NULL DEFAULT '0',
  `itemEntry` int NOT NULL DEFAULT '0',
  `quantity` int NOT NULL DEFAULT '1',
  `minBid` bigint DEFAULT '0',
  `duration` int NOT NULL DEFAULT '0',
  `chance` float NOT NULL DEFAULT '0',
  `bonusListIDs` text,
  PRIMARY KEY (`marketId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `blackmarket_template_6x`;
-- each 5.4.8 template row becomes its own market (5.4.8 ids are unique, its MarketId was a group)
INSERT INTO `blackmarket_template_6x` (`marketId`, `sellerNpc`, `itemEntry`, `quantity`, `minBid`, `duration`, `chance`, `bonusListIDs`)
SELECT `Id`, `SellerId`, `ItemEntry`, `Quantity`, `MinBid`, `Duration`, `Chance`, NULL FROM `blackmarket_template`;

DROP TABLE `blackmarket_template`;
RENAME TABLE `blackmarket_template_6x` TO `blackmarket_template`;
