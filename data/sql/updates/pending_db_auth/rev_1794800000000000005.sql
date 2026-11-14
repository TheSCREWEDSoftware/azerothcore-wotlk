-- 6.x account wide toys and heirlooms (CollectionMgr)
DROP TABLE IF EXISTS `battlenet_account_heirlooms`;
CREATE TABLE `battlenet_account_heirlooms` (
    `accountId` INT UNSIGNED NOT NULL,
    `itemId` INT UNSIGNED NOT NULL DEFAULT 0,
    `flags` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`accountId`, `itemId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `battlenet_account_toys`;
CREATE TABLE `battlenet_account_toys` (
    `accountId` INT UNSIGNED NOT NULL,
    `itemId` INT NOT NULL DEFAULT 0,
    `isFavourite` TINYINT DEFAULT 0,
    PRIMARY KEY (`accountId`, `itemId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
