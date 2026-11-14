-- Realm 1 for the 5.4.8 client: build and a name that shows the version
UPDATE `realmlist` SET `gamebuild` = 18414 WHERE `id` = 1;
UPDATE `realmlist` SET `name` = 'AzerothCore 5.4.8' WHERE `id` = 1 AND `name` = 'AzerothCore';
