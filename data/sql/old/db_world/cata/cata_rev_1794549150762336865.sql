-- Battleground holidays store the battleground id instead of a 32-bit mask (TP 108 and BfG 120 do not fit)
ALTER TABLE `game_event_battleground_holiday`
    CHANGE `eventEntry` `EventEntry` TINYINT UNSIGNED NOT NULL COMMENT 'game_event EventEntry identifier',
    CHANGE `bgflag` `BattlegroundID` INT UNSIGNED NOT NULL DEFAULT 0;

-- Call to Arms: Battle for Gilneas / Twin Peaks, the rotation grows from 6 to 8 weeks
DELETE FROM `game_event` WHERE `eventEntry` IN (191, 192);
INSERT INTO `game_event` (`eventEntry`, `start_time`, `end_time`, `occurence`, `length`, `holiday`, `holidayStage`, `description`, `world_event`, `announce`) VALUES
(191, '2010-05-14 13:00:00', '2030-12-31 12:00:00', 80640, 6240, 435, 1, 'Call to Arms: Battle for Gilneas!', 0, 2),
(192, '2010-05-21 13:00:00', '2030-12-31 12:00:00', 80640, 6240, 436, 1, 'Call to Arms: Twin Peaks!', 0, 2);

UPDATE `game_event` SET `occurence` = 80640 WHERE `eventEntry` IN (18, 19, 20, 21, 53, 54);

DELETE FROM `game_event_battleground_holiday` WHERE `EventEntry` IN (18, 19, 20, 21, 53, 54, 191, 192);
INSERT INTO `game_event_battleground_holiday` (`EventEntry`, `BattlegroundID`) VALUES
(18, 1),
(19, 2),
(20, 3),
(21, 7),
(53, 9),
(54, 30),
(191, 120),
(192, 108);
