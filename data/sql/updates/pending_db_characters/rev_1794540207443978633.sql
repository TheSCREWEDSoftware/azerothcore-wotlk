-- 4.3.4 guild leveling: guild level/XP and per-member activity (lifetime and this week).

ALTER TABLE `guild`
  ADD COLUMN `level` TINYINT UNSIGNED NOT NULL DEFAULT '1' AFTER `BankMoney`,
  ADD COLUMN `experience` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `level`,
  ADD COLUMN `todayExperience` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `experience`;

ALTER TABLE `guild_member`
  ADD COLUMN `activity` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `offnote`,
  ADD COLUMN `weekActivity` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `activity`;
