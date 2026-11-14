-- 6.x: SkillTiers.dbc is gone; skill caps per tier (from TrinityCore 6.x)
DROP TABLE IF EXISTS `skill_tiers`;
CREATE TABLE `skill_tiers` (
  `ID` int(10) unsigned NOT NULL DEFAULT '0',
  `Value1` int(10) unsigned NOT NULL DEFAULT '0',
  `Value2` int(10) unsigned NOT NULL DEFAULT '0',
  `Value3` int(10) unsigned NOT NULL DEFAULT '0',
  `Value4` int(10) unsigned NOT NULL DEFAULT '0',
  `Value5` int(10) unsigned NOT NULL DEFAULT '0',
  `Value6` int(10) unsigned NOT NULL DEFAULT '0',
  `Value7` int(10) unsigned NOT NULL DEFAULT '0',
  `Value8` int(10) unsigned NOT NULL DEFAULT '0',
  `Value9` int(10) unsigned NOT NULL DEFAULT '0',
  `Value10` int(10) unsigned NOT NULL DEFAULT '0',
  `Value11` int(10) unsigned NOT NULL DEFAULT '0',
  `Value12` int(10) unsigned NOT NULL DEFAULT '0',
  `Value13` int(10) unsigned NOT NULL DEFAULT '0',
  `Value14` int(10) unsigned NOT NULL DEFAULT '0',
  `Value15` int(10) unsigned NOT NULL DEFAULT '0',
  `Value16` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `skill_tiers`;
INSERT INTO `skill_tiers` (`ID`,`Value1`,`Value2`,`Value3`,`Value4`,`Value5`,`Value6`,`Value7`,`Value8`,`Value9`,`Value10`,`Value11`,`Value12`,`Value13`,`Value14`,`Value15`,`Value16`) VALUES
(2,75,150,225,300,375,450,525,600,700,800,900,1000,0,0,0,0),
(21,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(22,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(23,75,150,225,300,375,450,525,600,700,750,825,900,0,0,0,0),
(24,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(41,75,150,225,300,375,450,525,600,700,800,900,1000,1100,1200,1300,1400),
(61,75,150,225,300,375,450,525,600,700,750,825,900,0,0,0,0),
(62,75,150,225,300,375,450,525,600,700,800,900,1000,1100,1200,1300,1400),
(63,75,150,225,300,375,450,525,600,700,750,825,900,0,0,0,0),
(81,75,150,225,0,0,0,0,0,0,0,0,0,0,0,0,0),
(121,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(122,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(123,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(124,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(125,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(126,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(127,1000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(141,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(142,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(143,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(161,75,150,225,300,375,450,525,600,700,800,900,1000,1100,1200,1300,1400),
(181,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(182,300,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(221,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(222,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(223,75,150,225,300,375,0,0,0,0,0,0,0,0,0,0,0),
(224,75,150,225,300,375,450,525,600,700,750,825,900,0,0,0,0),
(230,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(233,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(234,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(235,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(236,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(237,600,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(295,30,0,0,300,375,450,525,600,700,800,900,1000,0,0,0,0);

-- 6.x: item templates come from Item.db2/Item-sparse.db2; the world database keeps only server data
DROP TABLE IF EXISTS `item_template_addon`;
CREATE TABLE `item_template_addon` (
  `Id` int unsigned NOT NULL,
  `FlagsCu` int unsigned NOT NULL DEFAULT '0',
  `FoodType` tinyint unsigned NOT NULL DEFAULT '0',
  `MinMoneyLoot` int unsigned NOT NULL DEFAULT '0',
  `MaxMoneyLoot` int unsigned NOT NULL DEFAULT '0',
  `SpellPPMChance` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `item_script_names`;
CREATE TABLE `item_script_names` (
  `Id` int unsigned NOT NULL,
  `ScriptName` varchar(64) NOT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELETE FROM `item_template_addon`;
INSERT INTO `item_template_addon` (`Id`, `FlagsCu`, `FoodType`, `MinMoneyLoot`, `MaxMoneyLoot`, `SpellPPMChance`)
SELECT `entry`, `flagsCustom`, `FoodType`, `minMoneyLoot`, `maxMoneyLoot`,
    GREATEST(`spellppmRate_1`, `spellppmRate_2`, `spellppmRate_3`, `spellppmRate_4`, `spellppmRate_5`)
FROM `item_template`
WHERE `flagsCustom` <> 0 OR `FoodType` <> 0 OR `minMoneyLoot` <> 0 OR `maxMoneyLoot` <> 0
    OR GREATEST(`spellppmRate_1`, `spellppmRate_2`, `spellppmRate_3`, `spellppmRate_4`, `spellppmRate_5`) > 0;

DELETE FROM `item_script_names`;
INSERT INTO `item_script_names` (`Id`, `ScriptName`)
SELECT `entry`, `ScriptName` FROM `item_template` WHERE `ScriptName` <> '';

DROP TABLE `item_template`;
