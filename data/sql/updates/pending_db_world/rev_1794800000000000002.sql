-- Battle.net account commands (.bnetaccount), 6.x login
DELETE FROM `acore_string` WHERE `entry` BETWEEN 35490 AND 35495;
INSERT INTO `acore_string` (`entry`, `content_default`) VALUES
(35490, 'A Battle.net account name must be an e-mail address.'),
(35491, 'Game account {} is now linked to Battle.net account {}.'),
(35492, 'Battle.net account {} or game account {} does not exist.'),
(35493, 'Game account {} is already linked to a Battle.net account.'),
(35494, 'Game account {} is no longer linked to a Battle.net account.'),
(35495, 'Game account {} is not linked to a Battle.net account.');

DELETE FROM `command` WHERE `name` LIKE 'bnetaccount%';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('bnetaccount create', 3, 'Syntax: .bnetaccount create $email $password [$createGameAccount]\r\nCreates a Battle.net account and, unless $createGameAccount is 0, its first game account ($id#1).'),
('bnetaccount gameaccountcreate', 3, 'Syntax: .bnetaccount gameaccountcreate $email\r\nAdds another game account ($id#$index) to the Battle.net account.'),
('bnetaccount lock country', 0, 'Syntax: .bnetaccount lock country [on|off]\r\nLocks the own Battle.net account to the current country.'),
('bnetaccount lock ip', 0, 'Syntax: .bnetaccount lock ip [on|off]\r\nLocks the own Battle.net account to the current IP.'),
('bnetaccount password', 0, 'Syntax: .bnetaccount password $old_password $new_password $new_password\r\nChanges the password of the own Battle.net account.'),
('bnetaccount set password', 3, 'Syntax: .bnetaccount set password $email $password $password\r\nSets the password of a Battle.net account.'),
('bnetaccount link', 3, 'Syntax: .bnetaccount link $email $game_account\r\nLinks a game account to a Battle.net account.'),
('bnetaccount unlink', 3, 'Syntax: .bnetaccount unlink $game_account\r\nRemoves the Battle.net account link of a game account.');
