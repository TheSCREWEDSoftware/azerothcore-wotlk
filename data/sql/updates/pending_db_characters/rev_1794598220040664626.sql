-- 5.4.8 Black Market Auction House auctions, from Project SkyFire
CREATE TABLE IF NOT EXISTS `blackmarket_auctions` (
  `auctionId` int unsigned NOT NULL DEFAULT '0',
  `templateId` int unsigned NOT NULL DEFAULT '0',
  `startTime` int unsigned NOT NULL DEFAULT '0',
  `currentBidder` int unsigned NOT NULL DEFAULT '0',
  `currentBid` bigint unsigned NOT NULL DEFAULT '0',
  `minIncrement` bigint unsigned NOT NULL DEFAULT '0',
  `numBids` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`auctionId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
