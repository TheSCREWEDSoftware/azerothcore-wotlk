-- Realm 1 for the 6.2.4 client: build and a name that shows the version
UPDATE `realmlist` SET `gamebuild` = 21742 WHERE `id` = 1;
UPDATE `realmlist` SET `name` = 'AzerothCore 6.2.4' WHERE `id` = 1 AND `name` = 'AzerothCore';
