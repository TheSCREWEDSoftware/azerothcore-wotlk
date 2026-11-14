-- 4.3.4 Item-sparse.db2 price factors the 3.3.5a schema lacked. Item::GetSellPrice multiplies them with the
-- ImportPrice*/ItemPriceBase factors to get the price the client shows for reforging and transmogrification.
-- apps/cata/convert_items_from_db2.py fills both columns; rows it has not converted keep 0 and fall back
-- to the minimum price.
ALTER TABLE `item_template`
    ADD COLUMN `PriceRandomValue` float NOT NULL DEFAULT 0 AFTER `SellPrice`,
    ADD COLUMN `PriceVariance` float NOT NULL DEFAULT 0 AFTER `PriceRandomValue`;
