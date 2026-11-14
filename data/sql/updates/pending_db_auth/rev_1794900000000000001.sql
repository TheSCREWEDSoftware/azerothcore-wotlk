-- Realm 1 for the 4.3.4 client: build and a name that shows the version
UPDATE `realmlist` SET `gamebuild` = 15595 WHERE `id` = 1;
UPDATE `realmlist` SET `name` = 'AzerothCore 4.3.4' WHERE `id` = 1 AND `name` = 'AzerothCore';
