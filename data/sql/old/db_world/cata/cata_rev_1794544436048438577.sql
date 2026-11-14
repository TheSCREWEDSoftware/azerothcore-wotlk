-- Twin Peaks and Battle for Gilneas battleground templates
DELETE FROM `battleground_template` WHERE `ID` IN (108, 120);
INSERT INTO `battleground_template` (`ID`, `MinPlayersPerTeam`, `MaxPlayersPerTeam`, `MinLvl`, `MaxLvl`, `AllianceStartLoc`, `AllianceStartO`, `HordeStartLoc`, `HordeStartO`, `StartMaxDist`, `Weight`, `ScriptName`, `Comment`) VALUES
(108, 5, 10, 10, 80, 1729, 3.14159, 1728, 0.151581, 75, 1, '', 'Twin Peaks (battleground)'),
(120, 8, 15, 20, 80, 1740, 0, 1739, 0, 0, 1, '', 'Battle for Gilneas (battleground)');
