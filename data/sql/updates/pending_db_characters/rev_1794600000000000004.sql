-- 4.3.4 player money is 64 bit (gold cap 999,999)
ALTER TABLE `characters`
  MODIFY COLUMN `money` bigint unsigned NOT NULL DEFAULT '0';
