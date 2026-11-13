-- Weekly earned, tracked (season) quantity and flags per character currency
ALTER TABLE `character_currency`
    ADD COLUMN `WeeklyQuantity` int unsigned NOT NULL DEFAULT '0' AFTER `Quantity`,
    ADD COLUMN `TrackedQuantity` int unsigned NOT NULL DEFAULT '0' AFTER `WeeklyQuantity`,
    ADD COLUMN `Flags` tinyint unsigned NOT NULL DEFAULT '0' AFTER `TrackedQuantity`;
