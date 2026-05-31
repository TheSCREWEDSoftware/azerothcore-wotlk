DELETE FROM `rbac_permissions` WHERE `id` IN (928, 929, 930);
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(928, 'Command: character inactive list'),
(929, 'Command: character inactive add'),
(930, 'Command: character inactive remove');

DELETE FROM `rbac_linked_permissions` WHERE `linkedId` IN (928, 929, 930);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 928),
(195, 929),
(195, 930);
