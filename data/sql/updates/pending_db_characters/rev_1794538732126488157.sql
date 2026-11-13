-- 4.3.4 guild banks have 8 tabs (tabs 7 and 8 come from guild vault vouchers); daily withdraw
-- limits are tracked per tab.
ALTER TABLE `guild_member_withdraw`
    ADD COLUMN `tab6` int unsigned NOT NULL DEFAULT '0' AFTER `tab5`,
    ADD COLUMN `tab7` int unsigned NOT NULL DEFAULT '0' AFTER `tab6`;
