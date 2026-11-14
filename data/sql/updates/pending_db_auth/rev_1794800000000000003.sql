-- RBAC permissions of the Battle.net account commands (TrinityCore 6.x ids 207-216)
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 207 AND 216;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(207, 'Command: bnetaccount'),
(208, 'Command: bnetaccount create'),
(209, 'Command: bnetaccount lock country'),
(210, 'Command: bnetaccount lock ip'),
(211, 'Command: bnetaccount password'),
(212, 'Command: bnetaccount set'),
(213, 'Command: bnetaccount set password'),
(214, 'Command: bnetaccount link'),
(215, 'Command: bnetaccount unlink'),
(216, 'Command: bnetaccount gameaccountcreate');

-- players manage their own Battle.net account, administrators everything
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 207 AND 216;
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(199, 207),
(199, 209),
(199, 210),
(199, 211),
(196, 208),
(196, 212),
(196, 213),
(196, 214),
(196, 215),
(196, 216);
