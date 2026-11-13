-- Primary talent tree (TalentTab.dbc) per talent spec
ALTER TABLE `characters` ADD COLUMN `talentTree` varchar(10) NOT NULL DEFAULT '0 0' AFTER `resettalents_time`;
